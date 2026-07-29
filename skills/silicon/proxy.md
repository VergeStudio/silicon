---
title: "silicon.proxy — C++ Proxy Library"
summary: "C++20 Proxy / Facade pattern library (Microsoft pro·xy v4 port). Enables type-erased polymorphism without inheritance."
read_when:
  - User wants to use proxy, facade, or type-erased polymorphism
  - User asks how to define a dispatch or facade
  - User needs to understand silicon.proxy API conventions
---

# silicon.proxy — Proxy Library

## Overview

A C++20 library for **type-erased polymorphism** without inheritance. Based on the [Microsoft pro·xy](https://github.com/microsoft/proxy) library (v4, MIT License).

Use it when you would otherwise reach for `std::function`, `std::any`, or virtual inheritance.

---

## Module Import

```cpp
import silicon.proxy;

// All types are in namespace silicon::proxy.
using namespace silicon::proxy;   // optional, for brevity
```

---

## Core Concepts

| Concept | Description |
|---------|-------------|
| **facade** | Describes a set of operations a type must support |
| **proxy**  | A type-erased wrapper holding any type matching a facade |
| **dispatch** | Named set of operations (the "interface") |
| **PRO_DEF_MEM_DISPATCH** | Macro to define a dispatch from a member function |
| **PRO_DEF_FREE_DISPATCH** | Macro to define a dispatch from a free function |

---

## Defining a Facade

```cpp
// Step 1: Define a dispatch — what operation(s) the facade requires
PRO_DEF_MEM_DISPATCH(Draw, draw, void());
PRO_DEF_MEM_DISPATCH(Area, area, double());

// Step 2: Define the facade — the interface contract
using Drawable = facade<Draw, Area>;

// Step 3: Use proxy — any type satisfying the facade works
struct Circle {
    void draw() const { /* ... */ }
    double area() const { return 3.14 * r_ * r_; }
    double r_ = 1.0;
};

proxy<Drawable> p{Circle{}};
p.draw();
double a = p.area();
```

---

## Dispatch Macros

### Member Function Dispatch

```cpp
// PRO_DEF_MEM_DISPATCH(name, member_func, signature)
PRO_DEF_MEM_DISPATCH(Serialize, serialize, void(std::ostream&));

using Serializable = facade<Serialize>;
proxy<Serializable> s{MyType{}};
s.serialize(std::cout);
```

### Free Function Dispatch

```cpp
// PRO_DEF_MEM_DISPATCH(name, free_func, signature)
PRO_DEF_MEM_DISPATCH(Hash, std::hash<std::string_view>{});

using Hashable = facade<Hash>;
proxy<Hashable> h{std::string{"hello"}};
auto h1 = h();   // calls std::hash
```

### Multiple Operations

A facade can combine any number of dispatches:

```cpp
using Streamable = facade<Serialize, Deserialize, Close>;
```

---

## Proxy Types

| Type | Semantics | Use Case |
|------|-----------|----------|
| `proxy<F>` | Owning, value semantics | General purpose |
| `proxy_view<F>` | Non-owning reference | Borrowed access |
| `weak_proxy<F>` | Nullable owning | Optional storage |
| `observer_facade<F>` | Read-only facade | Const access |

---

## Factory Functions

```cpp
auto p1 = make_proxy<Drawable>(Circle{});         // owning proxy
auto p2 = make_proxy_inplace<Drawable>(InPlaceType{});  // in-place construct
auto p3 = make_proxy_shared<Drawable>(Circle{});  // shared ownership
auto p4 = make_proxy_view<Drawable>(obj);          // non-owning view
```

---

## Skills (Optional Facade Extensions)

```cpp
using Drawable = facade<Draw, Area, skills::slim>;      // small buffer optimization
using Drawable = facade<Draw, Area, skills::as_view>;    // implicit view conversion
using Drawable = facade<Draw, Area, skills::as_weak>;    // nullable
```

---

## Version Info (at runtime)

```cpp
auto ver = silicon::proxy::GetVersion();           // "0.0.1"
auto maj  = silicon::proxy::GetVersionMajor();
auto min  = silicon::proxy::GetVersionMinor();
auto br   = silicon::proxy::GetVersionBranch();    // "dev"
auto cmt  = silicon::proxy::GetVersionCommit();    // commit hash
```

---

## Examples

### Callback / Strategy Pattern

```cpp
PRO_DEF_MEM_DISPATCH(Call, operator(), void(int));
using Callback = facade<Call>;

void process(std::span<int> data, proxy<Callback> cb) {
    for (int v : data) cb(v);
}

process(values, [](int x) { std::print("{}", x); });
```

### Ad-hoc Interface

```cpp
PRO_DEF_MEM_DISPATCH(GetName, name, std::string_view());
using Named = facade<GetName>;

std::vector<proxy<Named>> things;
things.emplace_back(MyClass{});
things.emplace_back([] { return std::string_view{"lambda"}; });

for (auto& t : things)
    std::print("{}\n", t.name());
```

---

## Background

- Based on [microsoft/proxy](https://github.com/microsoft/proxy) v4 (MIT)
- Ported by the Silicon project team
- 100% header-only at the implementation level; module-ized via C++23 modules
