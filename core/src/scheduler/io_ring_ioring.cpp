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
#    include <io.h>
#    include <ioringapi.h>
#endif

module silicon.scheduler;

#if defined(_MSC_VER)
import silicon.scheduler;
#endif

#if defined(SILICON_PLATFORM_WINDOWS) && defined(SILICON_FEATURE_IO_RING)

namespace silicon::scheduler {

namespace {

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

    done.result = FAILED(cqe.ResultCode) ? static_cast<std::int32_t>(cqe.ResultCode)
                                         : static_cast<std::int32_t>(cqe.Information);
    return done;
}

}

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

    return is_valid() && (which == op::read || which == op::write);
}

bool io_ring::submit_read(int fd, void *buf, std::uint32_t len, std::uint64_t offset, std::uint64_t user_data) {
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
        int fd, const void *buf, std::uint32_t len, std::uint64_t offset, std::uint64_t user_data
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

    return false;
}

std::uint32_t io_ring::submit() {
    if(!is_valid()) { return 0u; }

    UINT32 submitted = 0;

    if(FAILED(m_p->fn.submit(m_p->ring, 0, 0, &submitted))) { return 0u; }
    return submitted;
}

auto io_ring::wait_completion(std::chrono::milliseconds timeout) -> std::optional<completion> {
    if(!is_valid()) { return std::nullopt; }

    UINT32 submitted = 0;
    HRESULT hr = m_p->fn.submit(
            m_p->ring, 1, static_cast<UINT32>(timeout.count() > 0 ? timeout.count() : 0), &submitted
    );
    if(FAILED(hr)) { return std::nullopt; }

    IORING_CQE cqe{};
    if(FAILED(m_p->fn.pop(m_p->ring, &cqe))) { return std::nullopt; }
    return to_completion(cqe);
}

auto io_ring::peek_completion() -> std::optional<completion> {
    if(!is_valid()) { return std::nullopt; }

    UINT32 submitted = 0;
    if(FAILED(m_p->fn.submit(m_p->ring, 0, 0, &submitted))) { return std::nullopt; }

    IORING_CQE cqe{};
    if(FAILED(m_p->fn.pop(m_p->ring, &cqe))) { return std::nullopt; }
    return to_completion(cqe);
}

}

#endif
