#include <cstdint>
#include <cstring>
#include <silicon/test/test.h>
#include <string>

import silicon.ffi;

namespace ffi = silicon::ffi;

namespace {

int add2(int a, int b) { return a + b; }

double mix3(std::int32_t a, double b, float c) {
    return static_cast<double>(a) + b + static_cast<double>(c);
}

const char *greet() { return "hello-ffi"; }

void mul_closure(ffi::cif *, void *ret, void **args, void *user_data) {
    auto a = *static_cast<int *>(args[0]);
    auto b = *static_cast<int *>(args[1]);
    auto bias = *static_cast<int *>(user_data);
    *static_cast<ffi::sarg *>(ret) = static_cast<ffi::sarg>(a * b + bias);
}

}

TEST_CASE("ffi 版本信息") {
    CHECK(ffi::version_number() == 30701UL);
    CHECK(std::string(ffi::version()).find("3.7.1") != std::string::npos);
}

TEST_CASE("ffi 基础调用：int(int,int)") {
    ffi::cif c;
    ffi::type *args[] = {&ffi::type_sint32(), &ffi::type_sint32()};
    REQUIRE(ffi::prep_cif(&c, ffi::default_abi(), 2, &ffi::type_sint32(), args) == ffi::ok);

    int a = 40, b = 2, r = 0;
    void *argv[] = {&a, &b};
    ffi::call(&c, reinterpret_cast<void (*)()>(&add2), &r, argv);
    CHECK(r == 42);
}

TEST_CASE("ffi 混合标量参数：double(int32,double,float)") {
    ffi::cif c;
    ffi::type *args[] = {&ffi::type_sint32(), &ffi::type_double(), &ffi::type_float()};
    REQUIRE(ffi::prep_cif(&c, ffi::default_abi(), 3, &ffi::type_double(), args) == ffi::ok);

    std::int32_t a = 1;
    double b = 2.5;
    float f = 0.5F;
    double r = 0.0;
    void *argv[] = {&a, &b, &f};
    ffi::call(&c, reinterpret_cast<void (*)()>(&mix3), &r, argv);
    CHECK(r == doctest::Approx(4.0));
}

TEST_CASE("ffi 指针返回值：const char*(void)") {
    ffi::cif c;
    REQUIRE(ffi::prep_cif(&c, ffi::default_abi(), 0, &ffi::type_pointer(), nullptr) == ffi::ok);

    const char *r = nullptr;
    ffi::call(&c, reinterpret_cast<void (*)()>(&greet), &r, nullptr);
    REQUIRE(r != nullptr);
    CHECK(std::strcmp(r, "hello-ffi") == 0);
}

TEST_CASE("ffi 闭包：动态生成 int(int,int) 可调用入口") {
    ffi::cif c;
    ffi::type *args[] = {&ffi::type_sint32(), &ffi::type_sint32()};
    REQUIRE(ffi::prep_cif(&c, ffi::default_abi(), 2, &ffi::type_sint32(), args) == ffi::ok);

    void *code = nullptr;
    auto *cl = static_cast<ffi::closure *>(
            ffi::closure_alloc(sizeof(ffi::closure), &code)
    );
    REQUIRE(cl != nullptr);
    REQUIRE(code != nullptr);

    int bias = 7;
    REQUIRE(ffi::prep_closure_loc(cl, &c, &mul_closure, &bias, code) == ffi::ok);

    auto *fn = reinterpret_cast<int (*)(int, int)>(code);
    CHECK(fn(6, 7) == 6 * 7 + 7);

    ffi::closure_free(cl);
}
