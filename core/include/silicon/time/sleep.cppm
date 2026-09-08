module;

#include <chrono>
#include <thread>

export module silicon.time.sleep;

export namespace silicon::time {

// 阻塞当前线程指定时长。
//
// 统一收敛到 core/time，避免各模块裸用 std::this_thread::sleep_for，使休眠行为
// 单一可替换（便于插桩、单测与跨平台一致性）。语义与 std::this_thread::sleep_for
// 完全一致。
template<class Rep, class Period>
void sleep_for(std::chrono::duration<Rep, Period> d) {
    std::this_thread::sleep_for(d);
}

}
