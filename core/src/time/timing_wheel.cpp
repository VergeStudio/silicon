module;

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "silicon/common.h"
module silicon.time.timing_wheel;

namespace silicon::time {

namespace {

// 把毫秒量折算为"tick 数"：向上取整到 slot 边界，保证到期不早于请求时长；
// 最小为 1 tick（delay<=0 / 小于一个槽时也在下一 tick 触发）。
[[nodiscard]] std::uint64_t ceil_ticks(std::int64_t amount_ms, std::int64_t slot_ms) noexcept {
    if(slot_ms <= 0) {
        return 1;
    }
    if(amount_ms <= 0) {
        return 1;
    }
    std::uint64_t ticks = static_cast<std::uint64_t>((amount_ms + slot_ms - 1) / slot_ms);
    return ticks < 1 ? 1 : ticks;
}

// 取 >= n 的最小 2 的幂。
[[nodiscard]] std::size_t next_pow2(std::size_t n) noexcept {
    std::size_t p = 1;
    while(p < n) {
        p <<= 1;
    }
    return p;
}

} // namespace

// timer_handle 私有构造：仅 timing_wheel 可通过 friend 调用。
timer_handle::timer_handle(std::uint64_t id, std::uint32_t generation) noexcept
    : m_id(id), m_generation(generation) {}

class timing_wheel::impl {
  public:
    struct node {
        std::uint64_t id{0};
        std::uint32_t generation{0};
        std::uint64_t expire_tick{0};
        std::uint64_t period_ticks{0}; // 0 = 一次性
        bool active{false};
        std::function<void()> callback;
    };

    std::int64_t slot_ms{1};
    std::size_t slot_count{64};
    std::size_t slot_mask{63};
    std::vector<std::vector<node *>> slots;
    std::vector<node *> overrun;
    std::deque<std::unique_ptr<node>> pool;
    std::unordered_map<std::uint64_t, node *> by_id;
    std::uint64_t now_tick{0};
    std::uint64_t next_id{1};
    std::uint64_t active_count{0};

    // 是否落入当前环形窗口（rel = expire_tick - now_tick，要求 > 0）。
    [[nodiscard]] bool within_window(std::uint64_t rel) const noexcept {
        return rel > 0 && rel < slot_count;
    }

    // 把节点挂到环形槽或溢出列表。调用方保证 rel >= 1。
    void schedule(node *n) {
        std::uint64_t rel = n->expire_tick - now_tick;
        if(within_window(rel)) {
            slots[n->expire_tick & slot_mask].push_back(n);
        } else {
            overrun.push_back(n);
        }
    }

    // 一次性到期回收 + 惰性压缩：当池中惰性(已失效)节点占比过高时重建池，
    // 仅保留仍 active 的节点，重挂到槽/溢出，避免内存无限增长。
    void maybe_compact() {
        std::size_t pool_size = pool.size();
        if(pool_size == 0 || active_count == 0) {
            if(pool_size > 0 && active_count == 0) {
                pool.clear();
                slots.assign(slot_count, {});
                overrun.clear();
            }
            return;
        }
        // 惰性节点数量超过活跃数量即压缩一次。
        std::size_t dead = pool_size - active_count;
        if(dead < active_count) {
            return;
        }
        // 先把仍 active 的节点拷贝到 fresh（旧池尚未释放，可安全读取）。
        std::deque<std::unique_ptr<node>> fresh;
        std::unordered_map<std::uint64_t, node *> fresh_ids;
        for(auto &u : pool) {
            if(u->active) {
                auto np = std::make_unique<node>(*u);
                fresh_ids.emplace(np->id, np.get());
                fresh.push_back(std::move(np));
            }
        }
        // 清空仍引用旧节点指针的槽/溢出，再释放旧池，最后以新池指针重挂。
        slots.assign(slot_count, {});
        overrun.clear();
        pool = std::move(fresh);
        by_id = std::move(fresh_ids);
        for(auto &u : pool) {
            schedule(u.get());
        }
    }

    // 推进一格：重挂溢出 + 触发当前槽到期回调。返回触发数。
    std::size_t fire_tick() {
        ++now_tick;

        // 把已进入窗口的溢出项重挂回槽。
        if(!overrun.empty()) {
            std::vector<node *> remaining;
            remaining.reserve(overrun.size());
            for(node *n : overrun) {
                if(n->active && n->expire_tick <= now_tick + slot_count - 1) {
                    schedule(n);
                } else if(n->active) {
                    remaining.push_back(n);
                }
                // 已失效的溢出项直接丢弃（不再持有）。
            }
            overrun = std::move(remaining);
        }

        std::size_t fired = 0;
        std::size_t idx = now_tick & slot_mask;
        std::vector<node *> due;
        due.swap(slots[idx]);

        for(node *n : due) {
            if(!n->active) {
                continue;
            }
            // 到期判定：绝对到期 tick 已到（本槽 == now 槽即代表到期）。
            n->callback();
            ++fired;
            // 回调可能 cancel 自身；仅当仍活跃才作为周期项重插。
            if(n->active && n->period_ticks > 0) {
                n->expire_tick += n->period_ticks;
                schedule(n);
            } else if(!n->active) {
                // 回调内被取消：已在 cancel() 中 decrement；避免重复统计。
                continue;
            } else {
                n->active = false;
                --active_count;
                by_id.erase(n->id);
            }
        }

        maybe_compact();
        return fired;
    }
};

timing_wheel::timing_wheel(options o)
    : m_p(std::make_unique<impl>()) {
    if(o.slot_duration.count() <= 0) {
        o.slot_duration = std::chrono::milliseconds{1};
    }
    m_p->slot_ms = o.slot_duration.count();
    m_p->slot_count = next_pow2(o.slot_count == 0 ? 1 : o.slot_count);
    m_p->slot_mask = m_p->slot_count - 1;
    m_p->slots.assign(m_p->slot_count, {});
}

timing_wheel::~timing_wheel() = default;

timer_handle timing_wheel::add_once(
    std::chrono::milliseconds delay, std::function<void()> callback) {
    auto n = std::make_unique<impl::node>();
    std::uint64_t id = m_p->next_id++;
    n->id = id;
    n->generation = 1;
    n->expire_tick = m_p->now_tick + ceil_ticks(delay.count(), m_p->slot_ms);
    n->period_ticks = 0;
    n->active = true;
    n->callback = std::move(callback);

    impl::node *raw = n.get();
    m_p->by_id.emplace(id, raw);
    ++m_p->active_count;
    m_p->schedule(raw);
    m_p->pool.push_back(std::move(n));
    return timer_handle{id, raw->generation};
}

timer_handle timing_wheel::add_periodic(
    std::chrono::milliseconds period, std::function<void()> callback) {
    auto n = std::make_unique<impl::node>();
    std::uint64_t id = m_p->next_id++;
    n->id = id;
    n->generation = 1;
    n->expire_tick = m_p->now_tick + ceil_ticks(period.count(), m_p->slot_ms);
    n->period_ticks = ceil_ticks(period.count(), m_p->slot_ms);
    n->active = true;
    n->callback = std::move(callback);

    impl::node *raw = n.get();
    m_p->by_id.emplace(id, raw);
    ++m_p->active_count;
    m_p->schedule(raw);
    m_p->pool.push_back(std::move(n));
    return timer_handle{id, raw->generation};
}

void timing_wheel::cancel(const timer_handle &handle) noexcept {
    auto it = m_p->by_id.find(handle.m_id);
    if(it == m_p->by_id.end()) {
        return;
    }
    impl::node *n = it->second;
    if(!n->active || n->generation != handle.m_generation) {
        return;
    }
    n->active = false;
    --m_p->active_count;
    m_p->by_id.erase(it);
    // 节点本体留在池中，由下一次 maybe_compact 回收（惰性取消，安全）。
}

std::size_t timing_wheel::advance() {
    return m_p->fire_tick();
}

std::size_t timing_wheel::advance_by(std::chrono::milliseconds elapsed) {
    if(m_p->slot_ms <= 0) {
        return 0;
    }
    std::size_t steps = static_cast<std::size_t>(elapsed.count() / m_p->slot_ms);
    std::size_t fired = 0;
    for(std::size_t i = 0; i < steps; ++i) {
        fired += m_p->fire_tick();
    }
    return fired;
}

std::size_t timing_wheel::pending() const noexcept {
    return static_cast<std::size_t>(m_p->active_count);
}

}
