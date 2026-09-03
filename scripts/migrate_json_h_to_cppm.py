#!/usr/bin/env python3
"""Batch-migrate silicon.json's vendored nlohmann/json .h headers to C++20
module partitions.

Strategy
--------
* Every non-macro .h becomes a partition `silicon.json:<relpath-with-dots>`.
* Macro-only headers (abi_macros / macro_scope / macro_unscope / hedley) stay
  as textual #include in the GLOBAL MODULE FRAGMENT, because C++20 modules do
  NOT export macros across translation-unit boundaries.
* Cross-header `#include <silicon/json/...>` lines become `import :<partition>;`
  (relative partition import inside the same module).
* Standard-library / other `#include` lines stay as GMF textual includes.
* The body is wrapped with `export { ... }` so every declaration the partition
  provides is re-exported; json.cppm then `export import`s each partition to
  forward the public API to importers.
* Conditional-compilation blocks (`#if`/`#ifdef`/`#ifndef` ... `#endif`) that
  contain an `#include` are SPLIT: the `#if`/`#elif`/`#else`/`#endif` wrapper is
  emitted TWICE -- once in the GMF carrying only the `#include` lines, and once
  in the purview carrying only the declaration lines.  This preserves the
  conditional semantics (e.g. `<experimental/filesystem>` must not be included
  on platforms where JSON_HAS_EXPERIMENTAL_FILESYSTEM is false) while keeping the
  `#include` validly placed inside the global module fragment.
* `#pragma`-only blocks are emitted OUTSIDE `export { }` (a `#pragma` is not a
  declaration and must not sit inside an export-declaration-seq), but in their
  original textual position so they still bracket the declarations they guard.

Pure-mechanical: include-guard stripping, module declaration, include->import
rewrite and `export {}` wrapping. Per-declaration export fine-tuning (if any)
is left to the compiler-driven fixup pass.
"""
import os
import re
import sys

ROOT = "core/include/silicon/json"

# Pure-preprocessor headers: keep as textual #include, never a partition.
# Stored with the `silicon/json/` prefix so classify_include can match directly.
MACRO_HEADERS = {
    "silicon/json/detail/abi_macros.h",
    "silicon/json/detail/macro_scope.h",
    "silicon/json/thirdparty/hedley/hedley.h",
}
# Macro-undef headers: not needed in a per-partition build (each partition is
# its own translation unit, so the JSON_* feature macros from its own GMF never
# leak elsewhere) and must NOT be #included inside the purview.  Dropped.
UNSCOPE_HEADERS = {
    "silicon/json/detail/macro_unscope.h",
    "silicon/json/thirdparty/hedley/hedley_undef.h",
}

PREFIX = "silicon/json/"

# Hand-maintained partitions (not auto-generated from a .h): json_fwd.cppm is
# crafted to export ONLY the two public aliases and keep the forward
# declarations unexported, which the generic transform cannot replicate.
SKIP_SOURCES = {"json_fwd.h"}

# --- preprocessor line classifiers (operate on raw, unstripped lines) ---
RE_IF = re.compile(r"^\s*#\s*(if|ifdef|ifndef)\b")
RE_ENDIF = re.compile(r"^\s*#\s*endif\b")
RE_ELIF_ELSE = re.compile(r"^\s*#\s*(elif|else)\b")
RE_INCLUDE = re.compile(r"^\s*#\s*include\s+<([^>]+)>")
RE_GUARD_IFNDEF = re.compile(r"^#ifndef\s+(INCLUDE_\w+)")
RE_GUARD_DEFINE = re.compile(r"^#define\s+(INCLUDE_\w+)")


def partition_name(inc_path: str) -> str:
    """'silicon/json/detail/conversions/from_json.h' -> 'detail.conversions.from_json'."""
    rel = inc_path
    if rel.startswith(PREFIX):
        rel = rel[len(PREFIX):]
    if rel.endswith(".h"):
        rel = rel[:-2]
    return rel.replace("/", ".")


def classify_include(inc_path: str):
    """Return ('macro'|'unscope'|'partition'|'std', original_path)."""
    if inc_path in MACRO_HEADERS:
        return ("macro", inc_path)
    if inc_path in UNSCOPE_HEADERS:
        return ("unscope", inc_path)
    if inc_path.startswith(PREFIX):
        return ("partition", inc_path)
    return ("std", inc_path)


def is_pure_comment_line(line: str) -> bool:
    return line.strip().startswith("//")


def strip_guard_and_pragma_once(lines):
    """Remove an INCLUDE_-prefixed include guard (#ifndef/#define + matching
    #endif) and any `#pragma once`.  Returns the remaining body lines."""
    guard_ifndef = None
    guard_define = None
    for i, ln in enumerate(lines):
        m = RE_GUARD_IFNDEF.match(ln)
        if m:
            guard_ifndef = i
            for j in range(i + 1, min(i + 3, len(lines))):
                if RE_GUARD_DEFINE.match(lines[j]):
                    guard_define = j
                    break
            break

    body = list(lines)
    if guard_ifndef is not None:
        guard_name = RE_GUARD_IFNDEF.match(lines[guard_ifndef]).group(1)
        if guard_define is not None:
            body.pop(guard_define)
        body.pop(guard_ifndef)
        # remove the matching guard-close #endif (carries the guard name in its
        # comment, e.g. `#endif // INCLUDE_X_HPP_`); fall back to the last #endif.
        removed = False
        for k in range(len(body) - 1, -1, -1):
            m = re.match(r"^#endif\s*(//\s*(.*))?$", body[k])
            if m:
                comment = (m.group(2) or "").upper()
                if guard_name.upper() in comment:
                    body.pop(k)
                    removed = True
                    break
        if not removed:
            for k in range(len(body) - 1, -1, -1):
                if re.match(r"^#endif", body[k]):
                    body.pop(k)
                    break
    # strip any #pragma once
    body = [ln for ln in body if not re.match(r"^\s*#pragma\s+once", ln)]
    while body and body[-1].strip() == "":
        body.pop()
    while body and body[0].strip() == "":
        body.pop(0)
    return body


def parse_branches(block):
    """Split a ['#if' ... '#endif'] block into [(opener, inner_lines), ...] by
    its #elif / #else delimiters."""
    parts = []
    cur_opener = block[0]
    cur_inner = []
    for ln in block[1:-1]:
        if RE_ELIF_ELSE.match(ln):
            parts.append((cur_opener, cur_inner))
            cur_opener = ln
            cur_inner = []
        else:
            cur_inner.append(ln)
    parts.append((cur_opener, cur_inner))
    return parts


def split_cond(block, partition_imports):
    """Split a single conditional block into (gmf_lines, purview_lines).

    gmf_lines: the `#if`/`#elif`/`#else`/`#endif` wrapper carrying only the
        include lines (emitted into the global module fragment).  Empty if no
        branch contains any include.
    purview_lines: the same wrapper carrying only the declaration-side lines,
        in original order.
    """
    parts = parse_branches(block)
    gmf_lines = []
    purview_lines = []
    has_gmf = False
    for opener, inner in parts:
        bgmf, bpur = split_body(inner, partition_imports)
        if bgmf:
            gmf_lines.append(opener)
            gmf_lines.extend(bgmf)
            has_gmf = True
        if bpur:
            purview_lines.append(opener)
            purview_lines.extend(bpur)
    gmf_blk = gmf_lines + (["#endif"] if has_gmf else [])
    purview_blk = purview_lines + (["#endif"] if purview_lines else [])
    return gmf_blk, purview_blk


def split_body(lines, partition_imports):
    """Process a list of body lines.

    Returns (gmf_includes, purview_lines) where:
      gmf_includes: std/macro #include lines that belong in the GMF (ordered).
      purview_lines: every non-include line, in original order, for the module
        purview (declarations, namespaces, #pragma, #if/#endif wrappers, ...).
    Mutates partition_imports with `import :x;` lines.
    """
    gmf = []
    purview = []
    i = 0
    n = len(lines)
    while i < n:
        ln = lines[i]
        if RE_IF.match(ln):
            depth = 1
            j = i + 1
            while j < n:
                if RE_IF.match(lines[j]):
                    depth += 1
                elif RE_ENDIF.match(lines[j]):
                    depth -= 1
                    if depth == 0:
                        break
                j += 1
            gmf_blk, purview_blk = split_cond(lines[i:j + 1], partition_imports)
            gmf.extend(gmf_blk)
            purview.extend(purview_blk)
            i = j + 1
            continue
        m = RE_INCLUDE.match(ln)
        if m:
            path = m.group(1)
            kind, _ = classify_include(path)
            if kind == "partition":
                partition_imports.append("import :" + partition_name(path) + ";")
            elif kind == "unscope":
                pass  # dropped on purpose
            else:
                gmf.append(ln.rstrip())
            i += 1
            continue
        # any other line (declaration, namespace, #pragma, comment, blank):
        # stays in the purview in original order.
        purview.append(ln)
        i += 1
    return gmf, purview


_RE_STATIC = re.compile(r"^\s*static\s+")
_RE_ANON_NS = re.compile(r"^\s*namespace\s*\{")
_RE_NS_OPEN = re.compile(r"^\s*namespace\b")
_RE_FRIEND = re.compile(r"\bfriend\b")
_RE_STATIC_ASSERT = re.compile(r"^\s*static_assert\b")
_RE_TEMPLATE = re.compile(r"^\s*(?:inline\s+)?template\b")
# Macro invocations that expand to `namespace ... {` / `}` must stay RAW -- they
# are not declarations and must never receive an `export` prefix (exporting a
# closing `}` is a hard syntax error).
_RE_SILICON_NS = re.compile(r"SILICON_JSON_NAMESPACE_(BEGIN|END)")


def brace_delta(line: str, in_block: bool = False):
    """Count '{' - '}' in a line, ignoring those inside string/char literals,
    // comments and /* ... */ block comments.  `in_block` is the block-comment
    state at the start of the line; returns (net_delta, new_in_block_state) so
    callers that scan line-by-line can thread the state across lines (this is
    what lets a '}' sitting inside a doc-comment code example not close a
    surrounding class prematurely)."""
    res = 0
    in_str = None
    j = 0
    L = len(line)
    k = in_block
    while j < L:
        c = line[j]
        if k:  # inside a /* ... */ block comment
            if c == "*" and j + 1 < L and line[j + 1] == "/":
                k = False
                j += 2
                continue
            j += 1
            continue
        if in_str:
            if c == "\\":  # skip an escaped character (e.g. '\\', \")
                j += 2
                continue
            if c == in_str:
                in_str = None
            j += 1
            continue
        if c == '"' or c == "'":
            in_str = c
            j += 1
            continue
        if c == "/" and j + 1 < L and line[j + 1] == "/":
            break  # rest is a line comment
        if c == "/" and j + 1 < L and line[j + 1] == "*":
            k = True
            j += 2
            continue
        if c == "{":
            res += 1
        elif c == "}":
            res -= 1
        j += 1
    return res, k


def find_close_brace(lines, i):
    """Given line index `i` that starts a declaration (possibly with its '{'
    on a later line), return the index just past the declaration's closing
    '}'.  A single-line declaration (ending in ';', no '{') returns i+1."""
    n = len(lines)
    depth = 0
    started = False
    j = i
    in_block = False
    while j < n:
        line = lines[j]
        d, in_block = brace_delta(line, in_block)
        if not started:
            if "{" in line and d > 0:
                started = True
                depth = d
                if depth == 0:  # "{}" on one line
                    return j + 1
            elif line.rstrip().endswith(";"):
                return j + 1
        else:
            depth += d
            if depth == 0:
                return j + 1
        j += 1
    return j


def _is_blank_or_comment(ln: str) -> bool:
    s = ln.strip()
    return s == "" or s.startswith("//") or s.startswith("/*") or s.startswith("*")


def _is_preproc(ln: str) -> bool:
    return re.match(r"^\s*#", ln) is not None


def _is_template_prefix(ln: str) -> bool:
    return _RE_TEMPLATE.match(ln) is not None


def _is_template_cont(ln: str) -> bool:
    """A line that continues a multi-line ``template<...>`` parameter list.

    A continuation is identified purely by its *terminator*: it ends with `,`
    (another parameter follows) or with `>` (the parameter list closes) and it
    must NOT open a brace or end a statement.  We deliberately do NOT match a
    line merely because it *starts* with `class`/`typename` -- that would also
    catch real declarations such as `class foo {` or `typename bar`, shredding
    them into phantom template prefixes.
    """
    s = ln.strip()
    if s.endswith(","):
        return True
    if s.endswith(">") and "{" not in ln and ";" not in ln and "(" not in ln:
        return True
    return False


def _is_decl_head(ln: str) -> bool:
    """A line that begins a real declaration/definition (as opposed to a
    template-prefix, namespace open/close, comment, preprocessor or blank)."""
    if _is_template_prefix(ln) or _is_template_cont(ln):
        return False
    if _RE_NS_OPEN.match(ln):
        return False
    if _is_blank_or_comment(ln) or _is_preproc(ln):
        return False
    return True


def _anon_spans(lines):
    """Return [(open, close_exclusive), ...] index ranges of anonymous
    namespaces; nothing inside them may be exported (internal linkage)."""
    spans = []
    for k in range(len(lines)):
        if _RE_ANON_NS.match(lines[k]):
            e = find_close_brace(lines, k)
            spans.append((k, e))
    return spans


def emit_purview(lines):
    """Emit the purview with *per-declaration* ``export`` prefixes.

    C++20 lets a single declaration carry ``export`` (``export void f();``),
    which is far more robust than wrapping the whole body in one ``export { }``
    block: internal-linkage declarations (``static`` at namespace scope,
    anonymous namespaces, ``friend`` and ``static_assert``) simply get NO
    export, while namespace / class braces stay exactly where the source put
    them -- so brace balance and namespace scope are never disturbed.

    ``template<...>`` prefixes that precede a declaration are buffered and
    merged into the same exported unit, since ``export template<...> decl;`` is
    the only valid ordering.

    Returns the list of output lines.
    """
    out = []
    n = len(lines)
    prefix = []
    anon = _anon_spans(lines)
    in_block = False  # inside a `/* ... */` block comment

    def in_anon(idx):
        return any(a <= idx < b for a, b in anon)

    def is_comment_line(ln):
        nonlocal in_block
        s = ln.strip()
        if s.startswith("//"):
            return True
        if s.startswith("/*") and "*/" not in ln:
            in_block = True
            return True
        if in_block:
            if "*/" in ln:
                in_block = False
            return True
        if s.startswith("*/"):
            in_block = False
            return True
        return False

    i = 0
    while i < n:
        ln = lines[i]
        if is_comment_line(ln) or ln.strip() == "" or _is_preproc(ln) \
                or _RE_NS_OPEN.match(ln) or _RE_SILICON_NS.search(ln) \
                or re.match(r"^\s*\}", ln):
            out.extend(prefix)
            prefix = []
            out.append(ln)
            i += 1
            continue
        if _is_template_prefix(ln) or _is_template_cont(ln):
            prefix.append(ln)
            i += 1
            continue
        if not _is_decl_head(ln):
            out.extend(prefix)
            prefix = []
            out.append(ln)
            i += 1
            continue
        # declaration head: consume the whole declaration (incl. its body).
        end = find_close_brace(lines, i)
        unit = lines[i:end]
        decl_prefix = prefix          # template prefix buffer, valid until reset
        prefix = []
        head_line = unit[0].strip()
        exportable = True
        if _RE_STATIC.match(head_line):
            # A `static` TEMPLATE function is a helper that, under the
            # textual-include regime, was duplicated into every TU that
            # included the header. In a module partition it would carry
            # internal linkage and become invisible to other partitions that
            # legitimately use it (e.g. detail::unescape used by json_pointer).
            # Convert it to `inline` (external/module linkage) and export it.
            if decl_prefix and _RE_TEMPLATE.match(decl_prefix[0].lstrip()):
                unit[0] = re.sub(r"^\s*static\b",
                                 lambda mo: mo.group(0).replace("static", "inline"),
                                 unit[0])
            else:
                # Plain `static` free function: keep internal linkage (it is
                # used only within this partition, or it is one of the
                # deliberately-duplicated helper names such as create / value).
                exportable = False
        if _RE_FRIEND.search(head_line):
            exportable = False
        if _RE_STATIC_ASSERT.match(head_line):
            exportable = False
        if in_anon(i):
            exportable = False
        # Build the emitted unit AFTER `unit[0]` may have been rewritten above:
        # `decl_prefix + unit` copies element *references*, so mutating `unit[0]`
        # (rebinding it to a new string) would NOT be reflected in a previously
        # built `full`.  Constructing `full` here picks up the rewritten line.
        full = decl_prefix + unit
        if exportable and not full[0].lstrip().startswith("export "):
            full[0] = "export " + full[0]
        out.extend(full)
        i = end
    out.extend(prefix)
    return out


def transform(filepath: str):
    rel = os.path.relpath(filepath, ROOT)
    if rel in SKIP_SOURCES:
        return None  # hand-maintained, not regenerated
    if (PREFIX + rel) in MACRO_HEADERS or (PREFIX + rel) in UNSCOPE_HEADERS:
        return None  # not a partition

    with open(filepath, "r", encoding="utf-8") as fh:
        raw = fh.read()
    lines = raw.split("\n")

    # --- separate leading license comment block ---
    head = 0
    while head < len(lines) and is_pure_comment_line(lines[head]):
        head += 1
    rest = lines[head:]

    body = strip_guard_and_pragma_once(rest)
    partition_imports = []
    gmf_includes, purview = split_body(body, partition_imports)

    name = partition_name(PREFIX + rel)
    out = []
    out.append("//     __ _____ _____ _____")
    out.append("//  __|  |   __|     |   | |  silicon JSON")
    out.append("// |  |  |__   |  |  | | | |  version 3.11.3")
    out.append("// |_____|_____|_____|_|___|  https://github.com/VergeStudio/silicon")
    out.append("//")
    out.append("// SPDX-FileCopyrightText: silicon contributors")
    out.append("// SPDX-License-Identifier: MIT")
    out.append("")
    out.append("// Partition of the silicon.json module. Macros (JSON_* feature")
    out.append("// flags, SILICON_JSON_NAMESPACE_* ) are NOT exported by C++20")
    out.append("// modules, so the macro headers are textually included in the")
    out.append("// global module fragment of every partition that needs them.")
    out.append("")
    out.append("module;")
    out.append("")
    # macro headers always needed in GMF for their #defines
    out.append("#include <silicon/json/detail/abi_macros.h>")
    out.append("#include <silicon/json/detail/macro_scope.h>")
    # A partition that consumes `std_fs::path` needs the underlying <filesystem>
    # (or <experimental/filesystem>) header in ITS OWN global module fragment:
    # importing silicon.json:detail.meta.std_fs only provides the `std_fs`
    # namespace alias, it does not transitively pull in the std header's
    # definitions for this translation unit.
    if any("import :detail.meta.std_fs;" in imp for imp in partition_imports):
        out.append("#if JSON_HAS_EXPERIMENTAL_FILESYSTEM")
        out.append("#    include <experimental/filesystem>")
        out.append("#elif JSON_HAS_FILESYSTEM")
        out.append("#    include <filesystem>")
        out.append("#endif")
    for inc in gmf_includes:
        if "detail/abi_macros.h" in inc or "detail/macro_scope.h" in inc:
            continue
        out.append(inc)
    out.append("")
    out.append(f"export module silicon.json:{name};")
    out.append("")
    for imp in partition_imports:
        out.append(imp)
    if partition_imports:
        out.append("")
    # Emit the purview with per-declaration `export` prefixes: every
    # namespace-scope declaration that is NOT internal linkage (static,
    # anonymous namespace, friend, static_assert) is exported; namespace /
    # class braces and #pragma lines stay exactly where the source put them.
    out.extend(emit_purview(purview))
    out.append("")

    out_path = filepath[:-2] + ".cppm"
    with open(out_path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(out))
    return out_path


def main():
    count = 0
    created = []
    for cur, _dirs, files in os.walk(ROOT):
        for f in sorted(files):
            if not f.endswith(".h"):
                continue
            fp = os.path.join(cur, f)
            res = transform(fp)
            if res:
                count += 1
                created.append(os.path.relpath(res, ROOT))
    print(f"created {count} partitions:")
    for c in created:
        print("  ", c)


if __name__ == "__main__":
    main()
