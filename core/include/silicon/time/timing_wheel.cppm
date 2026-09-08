module;

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

#include "silicon/common.h"
export module silicon.time.timing_wheel;

export namespace silicon::time {

// 定时器句柄：对已注册定时项的不透明引用，仅用于 cancel。
// 内部 = 单调 id + 代数 generation；不持有任何节点指针，句柄可被安全复制，
// 即使底层定时项已触发/取消也不会悬垂。与 silicon::scheduler::timer_handle
// 属不同命名空间，互不冲突。
class SILICON_CORE_API timer_handle {
  private:
    std::uint64_t m_id{0};
    std::uint32_t m_generation{0};

    timer_handle(std::uint64_t id, std::uint32_t generation) noexcept;

  public:
    timer_handle() = default;

    [[nodiscard]] bool valid() const noexcept { return m_id != 0; }
    [[nodiscard]] bool operator==(const timer_handle &) const noexcept = default;

    friend class timing_wheel;
};

// 单层时间轮定时器：纯调用方驱动、非线程安全。
//
// 语义：以固定 slot_duration 为刻度，内部维护单调递增的"当前 tick"（absolute
// tick），到期用绝对 tick 计数而非墙上时间表达，因此不依赖任何真实时钟，
// 完全由调用方逐次 advance 推进，天然可单测。
//
// 结构：环形槽（数量须为 2 的幂）+ 超出环形跨度的"溢出列表"兜底；delay 落进
// 当前窗口内的定时项挂到对应槽，超窗者先入溢出列表、随 advance 进入窗口后再
// 挂回槽。插入/到期均摊 O(1)。
//
// 线程模型：调用方须保证 add_*/cancel/advance_* 在同一线程串行调用，内部不加锁。
class SILICON_CORE_API timing_wheel {
  public:
    struct options {
        // 单个槽对应的时间刻度。
        std::chrono::milliseconds slot_duration{std::chrono::milliseconds{1}};
        // 环形槽数量（须为 2 的幂，否则构造向上取整为 2 的幂）。环形覆盖的最大
        // delay = slot_duration * slot_count；超出部分走溢出列表兜底。
        std::size_t slot_count{64};
    };

    explicit timing_wheel(options o);

    // pimpl：禁止复制/移动。
    timing_wheel(const timing_wheel &) = delete;
    timing_wheel &operator=(const timing_wheel &) = delete;
    timing_wheel(timing_wheel &&) = delete;
    timing_wheel &operator=(timing_wheel &&) = delete;

    ~timing_wheel();

    // 注册一次性定时：delay 个 slot_duration 后触发一次。delay<=0 视为下一 tick 触发。
    [[nodiscard]] timer_handle add_once(
        std::chrono::milliseconds delay, std::function<void()> callback);

    // 注册周期定时：每隔 period 触发一次，直至被 cancel。首次触发在 period 之后。
    [[nodiscard]] timer_handle add_periodic(
        std::chrono::milliseconds period, std::function<void()> callback);

    // 取消定时项。惰性：仅将对应项标记为失效；已被触发或已取消的句柄调用无副作用。
    void cancel(const timer_handle &handle) noexcept;

    // 推进一个 slot_duration，触发当前槽内所有尚未取消的到期回调，返回触发次数。
    // 回调内再次 add_*/cancel 均安全（内部按 active+generation 守卫）。
    std::size_t advance();

    // 按真实流逝时长推进：elapsed / slot_duration 取整得 tick 数后逐 tick 推进。
    // 不足一个 slot_duration 的余数被丢弃（不累积）。返回累计触发次数。
    std::size_t advance_by(std::chrono::milliseconds elapsed);

    // 当前仍活跃（已注册且未取消）的定时项数量，供校验/测试。
    [[nodiscard]] std::size_t pending() const noexcept;

  private:
    struct impl;
    std::unique_ptr<impl> m_p;
};

}
