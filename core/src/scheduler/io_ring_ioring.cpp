// Windows 后端：I/O Ring（kernel32 的 CreateIoRing 家族，Windows 11+）。
// 仅当 xmake 选项 --io_ring=y 且平台为 Windows 时参与构建；否则本文件编译为空
// TU。守卫与 io_ring_uring.cpp 的 Linux 守卫互斥，恰好一个文件定义同组符号。
//
// CreateIoRing 在部分 Windows 11 版本上被禁用，因此全部入口点经
// GetProcAddress 动态解析：缺失或创建失败时整个后端降级为 none，消费方回退
// :io_notifier 的 IOCP 路径——本类**不重新实现 IOCP**。
//
// I/O Ring 只作用于**文件句柄**，不覆盖套接字 accept/connect；fd_t 在 Windows
// 上按 CRT 文件描述符解释（_get_osfhandle 转换）。套接字请走 io_notifier。

module;

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

#if defined(SILICON_PLATFORM_WINDOWS) && defined(SILICON_FEATURE_IO_RING)
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    include <windows.h>
#    include <io.h>        // ::_get_osfhandle
#    include <ioringapi.h> // HIORING / BuildIoRing*
#endif

module silicon.scheduler;
// MSVC 须显式 import 本模块接口方可访问其导出实体；clang 与标准不允许
// 实现单元自引用，故以 _MSC_VER 守卫。
#if defined(_MSC_VER)
import silicon.scheduler;
#endif

#if defined(SILICON_PLATFORM_WINDOWS) && defined(SILICON_FEATURE_IO_RING)

namespace silicon::scheduler {

namespace {

// CreateIoRing 家族由 kernel32 导出但可能不存在，故一律动态解析；
// BuildIoRing* 是 ioringapi.h 中的用户态内联辅助，直接调用即可。
using create_io_ring_fn = HRESULT(WINAPI *)(IORING_VERSION, IORING_CREATE_FLAGS, UINT32, UINT32, HIORING *);
using submit_io_ring_fn = HRESULT(WINAPI *)(HIORING, UINT32, UINT32, UINT32 *);
using pop_completion_fn = HRESULT(WINAPI *)(HIORING, IORING_CQE *);
using close_io_ring_fn = HRESULT(WINAPI *)(HIORING);

struct io_ring_api {
    create_io_ring_fn create{nullptr};
    submit_io_ring_fn submit{nullptr};
    pop_completion_fn pop{nullptr};
    close_io_ring_fn close{nullptr};

    [[nodiscard]] bool complete() const {
        return create != nullptr && submit != nullptr && pop != nullptr && close != nullptr;
    }
};

// 逐实例持有函数指针：避免静态对象析构顺序影响 io_ring 的析构。
io_ring_api load_io_ring_api() {
    io_ring_api table{};
    HMODULE kernel32 = ::GetModuleHandleW(L"kernel32.dll");
    if(kernel32 == nullptr) { return table; }

    table.create = reinterpret_cast<create_io_ring_fn>(::GetProcAddress(kernel32, "CreateIoRing"));
    table.submit = reinterpret_cast<submit_io_ring_fn>(::GetProcAddress(kernel32, "SubmitIoRing"));
    table.pop = reinterpret_cast<pop_completion_fn>(::GetProcAddress(kernel32, "PopIoRingCompletion"));
    table.close = reinterpret_cast<close_io_ring_fn>(::GetProcAddress(kernel32, "CloseIoRing"));
    return table;
}

[[nodiscard]] completion to_completion(const IORING_CQE &cqe) {
    completion done{};
    done.user_data = static_cast<std::uint64_t>(cqe.UserData);
    // 失败时回填 HRESULT 的负值形态；成功时 Information 为传输字节数。
    done.result = FAILED(cqe.ResultCode) ? static_cast<std::int32_t>(cqe.ResultCode)
                                         : static_cast<std::int32_t>(cqe.Information);
    return done;
}

} // namespace

struct io_ring::impl {
    io_ring_api fn{};
    HIORING ring{nullptr};
    bool valid{false};
};

io_ring::io_ring(io_ring_config cfg): m_p(std::make_unique<impl>()) {
    m_p->fn = load_io_ring_api();
    if(!m_p->fn.complete()) { return; }

    std::uint32_t depth = 1u;
    while(depth < cfg.queue_depth && depth < 4096u) { depth <<= 1; }

    // 完成队列容量须不小于提交队列容量。
    HRESULT hr = m_p->fn.create(IORING_VERSION_1, IORING_CREATE_REQUIRED_FLAGS_NONE, depth, depth * 2u, &m_p->ring);
    m_p->valid = SUCCEEDED(hr) && m_p->ring != nullptr;
    if(!m_p->valid) { m_p->ring = nullptr; }
}

io_ring::~io_ring() {
    if(m_p == nullptr) { return; }

    if(m_p->valid && m_p->ring != nullptr) { m_p->fn.close(m_p->ring); }
    m_p->valid = false;
    m_p->ring = nullptr;
}

bool io_ring::is_valid() const noexcept { return m_p && m_p->valid && m_p->ring != nullptr; }

auto io_ring::active_backend() const noexcept -> backend {
    return is_valid() ? backend::windows_io_ring : backend::none;
}

bool io_ring::supports(op which) const noexcept {
    // I/O Ring 只覆盖文件句柄的读与写。其取消以句柄为粒度，与本接口按
    // user_data 取消的语义不匹配，故 cancel 不受支持。
    return is_valid() && (which == op::read || which == op::write);
}

bool io_ring::submit_read(fd_t fd, void *buf, std::uint32_t len, std::uint64_t offset, std::uint64_t user_data) {
    if(!supports(op::read)) { return false; }

    HANDLE file_handle = reinterpret_cast<HANDLE>(::_get_osfhandle(fd));
    if(file_handle == INVALID_HANDLE_VALUE) { return false; }

    HRESULT hr = BuildIoRingReadFile(
            m_p->ring,
            IoRingHandleRefFromHandle(file_handle),
            IoRingBufferRefFromPointer(buf),
            len,
            offset,
            static_cast<UINT_PTR>(user_data),
            IORING_SQE_FLAGS_NONE
    );
    return SUCCEEDED(hr);
}

bool io_ring::submit_write(
        fd_t fd, const void *buf, std::uint32_t len, std::uint64_t offset, std::uint64_t user_data
) {
    if(!supports(op::write)) { return false; }

    HANDLE file_handle = reinterpret_cast<HANDLE>(::_get_osfhandle(fd));
    if(file_handle == INVALID_HANDLE_VALUE) { return false; }

    HRESULT hr = BuildIoRingWriteFile(
            m_p->ring,
            IoRingHandleRefFromHandle(file_handle),
            IoRingBufferRefFromPointer(const_cast<void *>(buf)),
            len,
            offset,
            static_cast<UINT_PTR>(user_data),
            IORING_SQE_FLAGS_NONE
    );
    return SUCCEEDED(hr);
}

bool io_ring::submit_cancel(std::uint64_t, std::uint64_t) {
    // Windows I/O Ring 的取消以文件句柄为粒度，无法按 user_data 精确取消，
    // 与本接口语义不匹配；调用方应改用 io_notifier 的取消路径。
    return false;
}

std::uint32_t io_ring::submit() {
    if(!is_valid()) { return 0u; }

    UINT32 submitted = 0;
    // waitOperations=0：只提交，不阻塞等待完成。
    if(FAILED(m_p->fn.submit(m_p->ring, 0, 0, &submitted))) { return 0u; }
    return submitted;
}

auto io_ring::wait_completion(std::chrono::milliseconds timeout) -> std::optional<completion> {
    if(!is_valid()) { return std::nullopt; }

    // 先让已排队的 SQE 进入内核，再按超时等待至多一个完成包。
    UINT32 submitted = 0;
    HRESULT hr = m_p->fn.submit(
            m_p->ring, 1, static_cast<UINT32>(timeout.count() > 0 ? timeout.count() : 0), &submitted
    );
    if(FAILED(hr)) { return std::nullopt; }

    IORING_CQE cqe{};
    if(FAILED(m_p->fn.pop(m_p->ring, &cqe))) { return std::nullopt; } // 超时或队列空
    return to_completion(cqe);
}

auto io_ring::peek_completion() -> std::optional<completion> {
    if(!is_valid()) { return std::nullopt; }

    UINT32 submitted = 0;
    if(FAILED(m_p->fn.submit(m_p->ring, 0, 0, &submitted))) { return std::nullopt; }

    IORING_CQE cqe{};
    if(FAILED(m_p->fn.pop(m_p->ring, &cqe))) { return std::nullopt; } // 无就绪完成项
    return to_completion(cqe);
}

} // namespace silicon::scheduler

#endif // SILICON_PLATFORM_WINDOWS && SILICON_FEATURE_IO_RING
