module;

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

export module silicon.fs;

import silicon.exception;

export namespace silicon::fs {

template<typename T>
class result {
    struct Impl {
      public:
        bool ok_{false};
        T value_{};
        silicon::exception::fs_error error_{};
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    result(T v) {
        impl_->ok_ = true;
        impl_->value_ = std::move(v);
    }
    result(silicon::exception::fs_error e) {
        impl_->ok_ = false;
        impl_->error_ = std::move(e);
    }
    result(const result &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    result &operator=(const result &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    result(result &&) noexcept = default;
    result &operator=(result &&) noexcept = default;
    ~result() = default;

    bool has_value() const { return impl_->ok_; }
    explicit operator bool() const { return impl_->ok_; }
    T &value() { return impl_->value_; }
    const T &value() const { return impl_->value_; }
    silicon::exception::fs_error &error() { return impl_->error_; }
    const silicon::exception::fs_error &error() const { return impl_->error_; }
};

// 针对 void 的特化
template<>
class result<void> {
    struct Impl {
      public:
        bool ok_{false};
        silicon::exception::fs_error error_{};
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    result() { impl_->ok_ = true; }
    result(silicon::exception::fs_error e) {
        impl_->ok_ = false;
        impl_->error_ = std::move(e);
    }
    result(const result &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    result &operator=(const result &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    result(result &&) noexcept = default;
    result &operator=(result &&) noexcept = default;
    ~result() = default;

    bool has_value() const { return impl_->ok_; }
    explicit operator bool() const { return impl_->ok_; }
    silicon::exception::fs_error &error() { return impl_->error_; }
    const silicon::exception::fs_error &error() const { return impl_->error_; }
};

/// 文件系统抽象（统一接口）。
/// 文本 read/write 平台无关地以 UTF-8 表达；平台相关细节
/// （Windows 文本 CRLF 归一化、POSIX 目录默认权限）由具体实现处理。
class i_file_system {
  public:
    virtual ~i_file_system() = default;

    /// 文本读取，返回 UTF-8 内容
    virtual result<std::string> read(const std::string &path) const = 0;
    /// 文本写入（平台相关：Windows 归一化为 CRLF，POSIX 保持 LF）
    virtual result<void> write(const std::string &path, const std::string &content) const = 0;

    /// 二进制读取
    virtual result<std::vector<std::byte>> read_binary(const std::string &path) const = 0;
    /// 二进制写入
    virtual result<void> write_binary(const std::string &path, const std::vector<std::byte> &data) const = 0;

    virtual bool exists(const std::string &path) const = 0;
    virtual result<std::vector<std::string>> list_dir(const std::string &path) const = 0;
    virtual bool create_directories(const std::string &path) const = 0;
};

} // namespace silicon::fs

// ── 分平台实现（file_system_base / win32_file_system / posix_file_system） ──
// 已并入模块实现单元 fs/src/fs.cpp：按 SILICON_PLATFORM_* 宏在编译期选用
// 对应平台实现（宏由顶层 xmake.lua 定义），create_file_system() 亦在该单元定义。

export namespace silicon::fs {

/// 工厂：返回当前平台的文件系统实现（具体类型由上面选中的头文件提供）。
std::unique_ptr<i_file_system> create_file_system();

} // namespace silicon::fs
