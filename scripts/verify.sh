#!/usr/bin/env bash
# verify.sh — Silicon 项目验证闭环（macOS/clang 22 为主，兼容其它平台）
#
# 用法:
#   scripts/verify.sh            # 全量 clean 重建(release) + 测试 + dylib 符号检查 + warning 统计
#   scripts/verify.sh --inc      # 增量构建（不 clean -a），更快但不保证暴露被掩盖的 warning
#   scripts/verify.sh --no-build # 仅跑测试 + 符号检查（假定已构建）
#   scripts/verify.sh --debug    # 在 xmake debug 模式验证（断言生效，可暴露 release 下被 NDEBUG
#                                #   掩盖的编译/逻辑错误）；结束自动还原为 release 模式
#
# 退出码:
#   0  全部通过（构建成功、0 warning、9 测试全绿、dylib 无测试符号）
#   1  任一步骤失败
#
# 设计约束（来自跨工具链经验）:
#   - 增量构建会掩盖未重编模块的 warning，故默认走 clean 全量。
#   - xmake 3.1.0 在 clean -a 后首跑会因 config 模板生成顺序报一次错，第二次即通过，
#     故 build 跑两次、以第二次结果为准。
#   - macOS 用 brew LLVM（自带 std modules），须把其 bin 置于 PATH 最前；否则 Apple clang
#     无 std 模块会报 'map' file not found。
#   - 运行测试须 DYLD_LIBRARY_PATH 指向 dylib 所在目录；测试二进制位于
#     build/<plat>/<arch>/release/silicon/*.test（depth 3，find 须 maxdepth 1 从该目录起）。

set -uo pipefail

INC=0
NOBUILD=0
DEBUG=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --inc)      INC=1 ;;
    --no-build) NOBUILD=1 ;;
    --debug)    DEBUG=1 ;;
    -h|--help)  sed -n '2,20p' "$0"; exit 0 ;;
    *) echo "unknown arg: $1" >&2; exit 2 ;;
  esac
  shift
done

# --debug：验证 debug 模式（断言生效，可暴露 release 下被 NDEBUG 掩盖的问题）。
# 注册 EXIT trap，无论成功失败均还原为 release（项目默认模式），避免遗留配置。
if [[ $DEBUG -eq 1 ]]; then
  trap 'xmake config -m release -y >/dev/null 2>&1' EXIT
fi

# --- 平台相关环境（macOS + brew LLVM） ---
if [[ "$(uname -s)" == "Darwin" ]]; then
  BREW_LLVM=/opt/homebrew/opt/llvm/bin
  if [[ -d "$BREW_LLVM" ]]; then
    export PATH="$BREW_LLVM:$PATH"
  fi
fi

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT" || exit 1

echo "==> [verify] pwd=$ROOT  mode=$([[ $DEBUG -eq 1 ]] && echo debug || echo release)"

# --- 构建 ---
if [[ $NOBUILD -eq 0 ]]; then
  if [[ $INC -eq 0 ]]; then
    echo "==> [verify] xmake clean -a"
    xmake clean -a >>/tmp/silicon_verify_build1.log 2>&1 || { echo "!! clean failed"; tail -15 /tmp/silicon_verify_build1.log; exit 1; }
  fi
  # clean -a 会清空 xmake 配置缓存，须在之后重新设定模式（release 默认，--debug 为 debug）
  MODE=$( [[ $DEBUG -eq 1 ]] && echo debug || echo release )
  xmake config -m "$MODE" -y >/dev/null 2>&1 || { echo "!! cannot configure $MODE mode"; exit 1; }
  # 最终构建日志为准；clean 后首跑会因 config 模板生成顺序报一次良性 warning:
  #   "cannot match add_files(...config.cppm)" 与 xmake 通用尾注 "add -v for getting more warnings"
  # 二者在第二次（最终）构建中消失。故以第二次构建日志为准，并过滤这两类良性提示。
  xmake build -y >/tmp/silicon_verify_build1.log 2>&1
  xmake build -y >/tmp/silicon_verify_build2.log 2>&1
  BUILD_RC=$?
  if [[ $BUILD_RC -ne 0 ]]; then
    echo "!! build failed (rc=$BUILD_RC)"; tail -15 /tmp/silicon_verify_build2.log; exit 1
  fi
  WARN=$(grep -E "warning:" /tmp/silicon_verify_build2.log \
         | grep -vE "cannot match add_files.*config" \
         | grep -vE "add -v for getting more warnings" \
         | grep -vE "libc\+\+\.modules\.json not found" \
         | wc -l | tr -d ' ')
  echo "==> [verify] build ok | warnings=$WARN"
  if [[ $WARN -ne 0 ]]; then
    echo "!! warnings present:"
    grep -E "warning:" /tmp/silicon_verify_build2.log \
      | grep -vE "cannot match add_files.*config" \
      | grep -vE "add -v for getting more warnings" \
      | grep -vE "libc\+\+\.modules\.json not found"
    exit 1
  fi
else
  echo "==> [verify] skip build (--no-build)"
fi

# --- 定位测试二进制与 dylib（按当前模式定位真实 *.test，取其所在目录，避免误匹配 .objs 或其它模式） ---
MODE_DIR=$( [[ $DEBUG -eq 1 ]] && echo debug || echo release )
TEST_BIN=$(find build -type f -name '*.test' -path "*/${MODE_DIR}/*" 2>/dev/null | head -1)
if [[ -z "$TEST_BIN" ]]; then
  echo "!! no *.test binaries found under build/ ($MODE_DIR)"; exit 1
fi
TEST_DIR=$(dirname "$TEST_BIN")
echo "==> [verify] test dir=$TEST_DIR (mode=$MODE_DIR)"

if [[ "$(uname -s)" == "Darwin" ]]; then
  export DYLD_LIBRARY_PATH="$TEST_DIR"
fi

# --- dylib 测试符号泄漏检查 ---
DYLIB=$(find "$TEST_DIR" -maxdepth 1 -name 'libsilicon_core*.dylib' 2>/dev/null | head -1)
if [[ -n "$DYLIB" ]]; then
  LEAK=$(nm -gU "$DYLIB" 2>/dev/null | grep -iE "test_main|doctest" || true)
  if [[ -n "$LEAK" ]]; then
    echo "!! test symbols leaked into dylib:"; echo "$LEAK"; exit 1
  fi
  echo "==> [verify] dylib clean (no test_main/doctest)"
else
  echo "==> [verify] (no libsilicon_core dylib found; skip symbol check)"
fi

# --- 运行全部测试（zsh 安全的按行读取） ---
TESTS=$(find "$TEST_DIR" -maxdepth 1 -type f -name '*.test' | sort)
if [[ -z "$TESTS" ]]; then
  echo "!! no *.test binaries found in $TEST_DIR"; exit 1
fi

FAIL=0
TOTAL_CASES=0
TOTAL_ASSERT=0
while IFS= read -r t; do
  [[ -z "$t" ]] && continue
  LOG=$(mktemp /tmp/silicon_test.XXXXXX.log)
  "$t" >"$LOG" 2>&1
  rc=$?
  name=$(basename "$t")
  if [[ $rc -ne 0 ]]; then
    echo "!! $name FAIL rc=$rc"; tail -10 "$LOG"; FAIL=1
  else
    c=$(grep -oE "test cases:[[:space:]]+[0-9]+" "$LOG" | grep -oE "[0-9]+$")
    a=$(grep -oE "assertions:[[:space:]]+[0-9]+" "$LOG" | grep -oE "[0-9]+$")
    TOTAL_CASES=$((TOTAL_CASES + ${c:-0}))
    TOTAL_ASSERT=$((TOTAL_ASSERT + ${a:-0}))
    echo "ok $name (cases=${c:-?} assert=${a:-?})"
  fi
  rm -f "$LOG"
done <<< "$TESTS"

if [[ $FAIL -ne 0 ]]; then
  echo "!! some tests failed"; exit 1
fi

echo "==> [verify] ALL OK | total cases=$TOTAL_CASES assertions=$TOTAL_ASSERT"
exit 0
