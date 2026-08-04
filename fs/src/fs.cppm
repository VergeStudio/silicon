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
class Result {
    struct Impl {
      public:
        bool ok_{false};
        T value_{};
        silicon::exception::FsError error_{};
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    Result(T v) {
        impl_->ok_ = true;
        impl_->value_ = std::move(v);
    }
    Result(silicon::exception::FsError e) {
        impl_->ok_ = false;
        impl_->error_ = std::move(e);
    }
    Result(const Result &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    Result &operator=(const Result &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    Result(Result &&) noexcept = default;
    Result &operator=(Result &&) noexcept = default;
    ~Result() = default;

    bool has_value() const { return impl_->ok_; }
    explicit operator bool() const { return impl_->ok_; }
    T &value() { return impl_->value_; }
    const T &value() const { return impl_->value_; }
    silicon::exception::FsError &error() { return impl_->error_; }
    const silicon::exception::FsError &error() const { return impl_->error_; }
};

// 针对 void 的特化
template<>
class Result<void> {
    struct Impl {
      public:
        bool ok_{false};
        silicon::exception::FsError error_{};
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    Result() { impl_->ok_ = true; }
    Result(silicon::exception::FsError e) {
        impl_->ok_ = false;
        impl_->error_ = std::move(e);
    }
    Result(const Result &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    Result &operator=(const Result &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    Result(Result &&) noexcept = default;
    Result &operator=(Result &&) noexcept = default;
    ~Result() = default;

    bool has_value() const { return impl_->ok_; }
    explicit operator bool() const { return impl_->ok_; }
    silicon::exception::FsError &error() { return impl_->error_; }
    const silicon::exception::FsError &error() const { return impl_->error_; }
};

/// 文件系统抽象（统一接口）。
/// 文本 read/write 平台无关地以 UTF-8 表达；平台相关细节
/// （Windows 文本 CRLF 归一化、POSIX 目录默认权限）由具体实现处理。
class FileSystem {
  public:
    virtual ~FileSystem() = default;

    /// 文本读取，返回 UTF-8 内容
    virtual Result<std::string> Read(const std::string &path) const = 0;
    /// 文本写入（平台相关：Windows 归一化为 CRLF，POSIX 保持 LF）
    virtual Result<void> Write(const std::string &path, const std::string &content) const = 0;

    /// 二进制读取
    virtual Result<std::vector<std::byte>> ReadBinary(const std::string &path) const = 0;
    /// 二进制写入
    virtual Result<void> WriteBinary(const std::string &path, const std::vector<std::byte> &data) const = 0;

    virtual bool Exists(const std::string &path) const = 0;
    virtual Result<std::vector<std::string>> ListDir(const std::string &path) const = 0;
    virtual bool CreateDirectories(const std::string &path) const = 0;
};

} // namespace silicon::fs

// ── 分平台实现（_win / _posix 后缀头文件） ──────────────────────────
// 按 SILICON_PLATFORM_* 宏（顶层 xmake.lua 定义）在编译期选用对应实现，
// 由本模块 #include 进 silicon.fs，避免多模块 BMI 暴露问题。
#if defined(SILICON_PLATFORM_WINDOWS)
#    include "silicon/fs/fs_win.hpp"
#elif defined(SILICON_PLATFORM_UNIX)
#    include "silicon/fs/fs_posix.hpp"
#endif

export namespace silicon::fs {

/// 工厂：返回当前平台的文件系统实现（具体类型由上面选中的头文件提供）。
std::unique_ptr<FileSystem> CreateFileSystem();

} // namespace silicon::fs
