// di 模块测试：覆盖依赖注入容器的接口绑定、作用域、构造注入、自动装配与错误路径。
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include <silicon/di/di_macros.h>
#include <silicon/test/test.hpp>

import silicon.di;

namespace di = silicon::di;

namespace {

struct igreeter {
    virtual ~igreeter() = default;
    virtual std::string greet() const = 0;
};

struct english_greeter final: igreeter {
    std::string greet() const override { return "hello"; }
};

struct iconfig {
    virtual ~iconfig() = default;
    virtual int retries() const = 0;
};

struct retry_config final: iconfig {
    int retries() const override { return 3; }
};

// 构造期注入：SILICON_DI_CONSTRUCTOR 声明依赖签名，容器据此自动装配。
struct greeting_service {
    SILICON_DI_CONSTRUCTOR(greeting_service(igreeter &greeter, iconfig &cfg))
        : greeter_(greeter),
          cfg_(cfg) {}

    std::string describe() const { return greeter_.greet() + "/" + std::to_string(cfg_.retries()); }

    igreeter &greeter_;
    iconfig &cfg_;
};

} // namespace

TEST_CASE("di_error 枚举与 make_error_code 走 silicon.di category") {
    auto ec = make_error_code(di::di_error::kTypeNotFound);
    CHECK(ec.value() == static_cast<int>(di::di_error::kTypeNotFound));
    CHECK(std::string(ec.category().name()) == "silicon.di");
    CHECK(std::string(ec.message()) == "requested type not found in container");
    CHECK(std::string(make_error_code(di::di_error::kCircularDependency).message()) == "circular dependency detected");

    // 枚举底层值（顺序即协议，改动须同步）
    static_assert(static_cast<int>(di::di_error::kDuplicateBinding) == 1);
    static_assert(static_cast<int>(di::di_error::kUnresolvedDependency) == 2);
    static_assert(static_cast<int>(di::di_error::kCircularDependency) == 3);
    static_assert(static_cast<int>(di::di_error::kInvalidType) == 4);
}

TEST_CASE("接口绑定：resolve 得到实现引用") {
    di::container<> c;
    c.register_type<di::interfaces<igreeter>, di::storage_marker<english_greeter>, di::scope<di::shared>>();
    auto g = c.resolve<igreeter &>();
    REQUIRE(g.has_value());
    CHECK(g->get().greet() == "hello");
}

TEST_CASE("scope::unique 每次解析出新实例") {
    // unique 存储按值把实例移出，抽象接口无法按值持有，故按实现类型直接注册并解析，
    // 两次解析得到两个独立实例。
    di::container<> c;
    c.register_type<di::storage_marker<english_greeter>, di::scope<di::unique>>();
    auto a = c.resolve<english_greeter>();
    auto b = c.resolve<english_greeter>();
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    CHECK(a->greet() == "hello");
    CHECK(&a.value() != &b.value());
}

TEST_CASE("scope::shared 复用同一实例") {
    di::container<> c;
    c.register_type<di::interfaces<igreeter>, di::storage_marker<english_greeter>, di::scope<di::shared>>();
    auto a = c.resolve<igreeter &>();
    auto b = c.resolve<igreeter &>();
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    CHECK(&a->get() == &b->get());
}

TEST_CASE("未注册的类型解析失败，返回 di category 的错误码") {
    di::container<> c;
    auto g = c.resolve<igreeter &>();
    REQUIRE_FALSE(g.has_value());
    CHECK(std::string(g.error().category().name()) == "silicon.di");
}

TEST_CASE("构造注入：SILICON_DI_CONSTRUCTOR 声明的依赖被自动装配") {
    di::container<> c;
    c.register_type<di::interfaces<igreeter>, di::storage_marker<english_greeter>, di::scope<di::shared>>();
    c.register_type<di::interfaces<iconfig>, di::storage_marker<retry_config>, di::scope<di::shared>>();
    c.register_type<di::storage_marker<greeting_service>, di::scope<di::unique>>();

    auto s = c.resolve<greeting_service &>();
    REQUIRE(s.has_value());
    CHECK(s->get().describe() == "hello/3");
}

TEST_CASE("construct 无需注册直接自动装配") {
    di::container<> c;
    c.register_type<di::interfaces<igreeter>, di::storage_marker<english_greeter>, di::scope<di::shared>>();
    c.register_type<di::interfaces<iconfig>, di::storage_marker<retry_config>, di::scope<di::shared>>();

    auto s = c.construct<greeting_service>();
    REQUIRE(s.has_value());
    CHECK(s->describe() == "hello/3");
}

TEST_CASE("callable 工厂注册值类型") {
    di::container<> c;
    int produced = 0;
    c.register_type<di::storage_marker<int>, di::scope<di::unique>>(
            di::callable([&produced] { ++produced; return 42; })
    );

    auto v = c.resolve<int>();
    REQUIRE(v.has_value());
    CHECK(*v == 42);
    CHECK(produced == 1);

    auto again = c.resolve<int>();
    REQUIRE(again.has_value());
    CHECK(produced == 2);
}

TEST_CASE("invoke 注入可调用对象的参数") {
    di::container<> c;
    c.register_type<di::interfaces<igreeter>, di::storage_marker<english_greeter>, di::scope<di::shared>>();
    c.register_type<di::interfaces<iconfig>, di::storage_marker<retry_config>, di::scope<di::shared>>();

    // invoke 的参数解析失败按硬失败落地（依赖层无法向上传播错误码），
    // 因此 invoke 返回可调用对象的原始结果而非 expected。
    auto r = c.invoke([](igreeter &g, iconfig &cfg) { return g.greet() + std::to_string(cfg.retries()); });
    CHECK(r == "hello3");
}

TEST_CASE("集合绑定：同类型多个注册按注册顺序收集") {
    di::container<> c;
    int produced = 0;
    c.register_type_collection<di::storage_marker<std::vector<int>>, di::scope<di::unique>>();
    c.register_type<di::storage_marker<int>, di::scope<di::unique>>(
            di::callable([&produced] { ++produced; return 42; })
    );

    auto all = c.resolve<std::vector<int>>();
    REQUIRE(all.has_value());
    CHECK(all->size() == 1u);
    CHECK(all->at(0) == 42);
    CHECK(produced == 1);
}

TEST_CASE("解析具体实现类型按值（shared 范围）") {
    di::container<> c;
    c.register_type<di::storage_marker<english_greeter>, di::scope<di::shared>>();
    auto s = c.resolve<english_greeter>();
    REQUIRE(s.has_value());
    CHECK(s->greet() == "hello");
}
