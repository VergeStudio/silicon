// proxy 模块测试：覆盖类型擦除基础设施 facade_builder / proxy / proxy_view 的
// 约定分派、生命周期语义与 rtti / as_view / weak_dispatch 技能。
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

// 宏不随模块导出：PRO_DEF_MEM_DISPATCH 等由 proxy_macros.h 提供，消费方须自行
// 包含（与 silicon.proxy 模块接口内的同一份定义互不冲突，各 TU 各有一份）。
#include <silicon/proxy/proxy_macros.h>

#include <silicon/test/test.h>

import silicon.proxy;

namespace sp = silicon::proxy;

namespace {

// 追加写：append 追加到尾部。
struct text_buffer {
    std::string value;

    text_buffer() = default;
    explicit text_buffer(std::string v) noexcept: value(std::move(v)) {}

    std::size_t size() const noexcept { return value.size(); }
    const char *data() const noexcept { return value.c_str(); }
    void append(std::string_view more) { value.append(more); }
    std::string str() const { return value; }
};

// 前插写：与 text_buffer 同样的约定、相反的实现，用于验证分派按目标类型走。
struct prefix_buffer {
    std::string value;

    prefix_buffer() = default;
    explicit prefix_buffer(std::string v) noexcept: value(std::move(v)) {}

    std::size_t size() const noexcept { return value.size(); }
    const char *data() const noexcept { return value.c_str(); }
    void append(std::string_view more) { value.insert(0, more); }
    std::string str() const { return value; }
};

// 就地构造目标：小于默认 facade 容量（两个指针）。
struct small_counter {
    int value;

    explicit small_counter(int v) noexcept: value(v) {}

    std::size_t size() const noexcept { return static_cast<std::size_t>(value); }
    const char *data() const noexcept { return ""; }
    void append(std::string_view) noexcept { ++value; }
    std::string str() const { return std::to_string(value); }
};

// 超出默认 facade 容量的目标：make_proxy 自动退化为堆分配，无法满足就地约束。
struct big_buffer {
    char storage[64]{};
    std::size_t length{0};

    std::size_t size() const noexcept { return length; }
    const char *data() const noexcept { return storage; }
    void append(std::string_view more) {
        for(char c: more) {
            if(length + 1u < sizeof(storage)) {
                storage[length++] = c;
            }
        }
        storage[length] = '\0';
    }
    std::string str() const { return std::string(storage, length); }
};

// 只实现部分约定：用于 weak_dispatch 的未实现约定回退路径。
struct partial_buffer {
    std::size_t size() const noexcept { return 0u; }
    std::string str() const { return "partial"; }
};

PRO_DEF_MEM_DISPATCH(MemSize, size);
PRO_DEF_MEM_DISPATCH(MemData, data);
PRO_DEF_MEM_DISPATCH(MemAppend, append);
PRO_DEF_MEM_DISPATCH(MemStr, str);
PRO_DEF_MEM_DISPATCH(MemClear, clear);

// 基础 facade：四个约定，默认约束（不可拷贝 / 平凡重定位 / noexcept 析构）。
using text_facade = sp::facade_builder
        ::add_convention<MemSize, std::size_t() const>
        ::add_convention<MemData, const char *() const>
        ::add_convention<MemAppend, void(std::string_view)>
        ::add_convention<MemStr, std::string() const>
        ::build;

// 可拷贝 facade：非平凡拷贝，用于验证 proxy 拷贝的是目标值本身。
using copyable_facade = sp::facade_builder
        ::add_convention<MemSize, std::size_t() const>
        ::add_convention<MemAppend, void(std::string_view)>
        ::add_convention<MemStr, std::string() const>
        ::support_copy<sp::constraint_level::kNontrivial>
        ::build;

#if defined(__cpp_rtti) && __cpp_rtti >= 199711L
using rtti_facade = sp::facade_builder
        ::add_convention<MemSize, std::size_t() const>
        ::add_convention<MemStr, std::string() const>
        ::add_skill<sp::skills::rtti>
        ::build;
#endif // __cpp_rtti

using viewable_facade = sp::facade_builder
        ::add_convention<MemSize, std::size_t() const>
        ::add_convention<MemStr, std::string() const>
        ::add_skill<sp::skills::as_view>
        ::build;

// 未实现的 clear 采用 weak_dispatch 兜底：目标缺该方法时抛 std::logic_error。
using weak_facade = sp::facade_builder
        ::add_convention<MemSize, std::size_t() const>
        ::add_convention<MemStr, std::string() const>
        ::add_convention<sp::weak_dispatch<MemClear>, void()>
        ::build;

} // namespace

TEST_CASE("facade 契约：默认约束与容量") {
    static_assert(sp::facade<text_facade>);
    static_assert(text_facade::copyability == sp::constraint_level::kNone);
    static_assert(text_facade::relocatability == sp::constraint_level::kTrivial);
    static_assert(text_facade::destructibility == sp::constraint_level::kNothrow);
    static_assert(text_facade::max_size == 2u * sizeof(void *));
    static_assert(copyable_facade::copyability == sp::constraint_level::kNontrivial);
}

TEST_CASE("proxiable_target / inplace_proxiable_target 按约定与容量判定") {
    static_assert(sp::proxiable_target<text_buffer, text_facade>);
    static_assert(sp::proxiable_target<prefix_buffer, text_facade>);
    static_assert(sp::proxiable_target<big_buffer, text_facade>);
    static_assert(!sp::proxiable_target<int, text_facade>);

    static_assert(sp::inplace_proxiable_target<small_counter, text_facade>);
    static_assert(!sp::inplace_proxiable_target<big_buffer, text_facade>);
}

TEST_CASE("make_proxy 构造并分派到目标实现") {
    auto p = sp::make_proxy<text_facade, text_buffer>("hello");
    REQUIRE(p.has_value());
    CHECK(p->size() == 5u);
    CHECK(std::string(p->data()) == "hello");
    p->append("!!");
    CHECK(p->str() == "hello!!");
}

TEST_CASE("同一 facade 下不同目标各自分派") {
    sp::proxy<text_facade> appended = sp::make_proxy<text_facade, text_buffer>("abc");
    sp::proxy<text_facade> prepended = sp::make_proxy<text_facade, prefix_buffer>("abc");
    appended->append("X");
    prepended->append("X");
    CHECK(appended->str() == "abcX");
    CHECK(prepended->str() == "Xabc");
}

TEST_CASE("超出就地容量的目标经堆分配仍可用") {
    auto p = sp::make_proxy<text_facade, big_buffer>();
    REQUIRE(p.has_value());
    p->append("heap");
    CHECK(p->str() == "heap");
    CHECK(p->size() == 4u);
}

TEST_CASE("make_proxy_inplace 就地构造小目标") {
    auto p = sp::make_proxy_inplace<text_facade, small_counter>(7);
    REQUIRE(p.has_value());
    CHECK(p->str() == "7");
    p->append("+");
    CHECK(p->str() == "8");
}

TEST_CASE("空 proxy：has_value / operator bool / 与 nullptr 比较") {
    sp::proxy<text_facade> p;
    CHECK_FALSE(p.has_value());
    CHECK_FALSE(static_cast<bool>(p));
    CHECK(p == nullptr);
}

TEST_CASE("reset 与 nullptr 赋值回到空状态") {
    auto p = sp::make_proxy<text_facade, text_buffer>("gone");
    REQUIRE(p.has_value());
    p = nullptr;
    CHECK_FALSE(p.has_value());
    CHECK(p == nullptr);
}

TEST_CASE("移动构造移交所有权，源回到空状态") {
    auto p = sp::make_proxy<text_facade, text_buffer>("move");
    auto q = std::move(p);
    REQUIRE(q.has_value());
    CHECK(q->str() == "move");
    CHECK_FALSE(p.has_value());
}

TEST_CASE("拷贝构造复制目标值，两者互不影响") {
    auto p = sp::make_proxy<copyable_facade, text_buffer>("orig");
    auto copy = p;
    REQUIRE(copy.has_value());
    CHECK(copy->str() == "orig");
    copy->append("+copy");
    CHECK(p->str() == "orig");
    CHECK(copy->str() == "orig+copy");
}

TEST_CASE("proxy_view 观察原对象，双向可见") {
    text_buffer buf{"view"};
    auto v = sp::make_proxy_view<text_facade>(buf);
    REQUIRE(v.has_value());
    CHECK(v->str() == "view");

    buf.append("+source");
    CHECK(v->str() == "view+source");

    v->append("+view");
    CHECK(buf.value == "view+source+view");
}

TEST_CASE("weak_dispatch 对未实现的约定抛 std::logic_error") {
    auto p = sp::make_proxy<weak_facade, partial_buffer>();
    REQUIRE(p.has_value());
    CHECK(p->str() == "partial");
    CHECK_THROWS_AS(p->clear(), std::logic_error);
}

TEST_CASE("skills::as_view 允许 proxy 隐式转为 proxy_view") {
    auto p = sp::make_proxy<viewable_facade, text_buffer>("asview");
    sp::proxy_view<viewable_facade> v = p;
    REQUIRE(v.has_value());
    CHECK(v->str() == "asview");
}

#if defined(__cpp_rtti) && __cpp_rtti >= 199711L
// proxy_typeid / proxy_cast 是间接访问器（*p）的隐藏友元，须经 *p 触发 ADL，
// 直接传 proxy 本体时其关联类不含该访问器分支。
TEST_CASE("skills::rtti 提供 typeid 反射与 proxy_cast") {
    auto p = sp::make_proxy<rtti_facade, text_buffer>("rtti");
    REQUIRE(p.has_value());
    CHECK(proxy_typeid(*p) == typeid(text_buffer));

    auto &ref = proxy_cast<text_buffer &>(*p);
    CHECK(ref.value == "rtti");
    ref.value += "!";
    CHECK(p->str() == "rtti!");
}

TEST_CASE("proxy_cast 类型不匹配抛 bad_proxy_cast") {
    auto p = sp::make_proxy<rtti_facade, prefix_buffer>("rtti");
    REQUIRE(proxy_typeid(*p) == typeid(prefix_buffer));
    CHECK_THROWS_AS(proxy_cast<text_buffer &>(*p), sp::bad_proxy_cast);
}

TEST_CASE("bad_proxy_cast::what 契约") {
    sp::bad_proxy_cast e;
    CHECK(std::string(e.what()) == "silicon::proxy::bad_proxy_cast");
    const std::exception &base = e;
    CHECK(std::string(base.what()) == "silicon::proxy::bad_proxy_cast");
}
#endif // __cpp_rtti
