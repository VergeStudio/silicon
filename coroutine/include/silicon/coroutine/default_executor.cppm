module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
#include <memory>



#ifdef LIBCORO_FEATURE_NETWORKING
#else
#endif

export module silicon.coroutine:default_executor;

import :scheduler;
import :pool;

export namespace silicon::coroutine::default_executor {

/**
 * Set up default silicon::coroutine::pool::options before constructing a single instance of silicon::coroutine::pool in
 * silicon::coroutine::default_executor::executor()
 * @param pool_options pool options
 */
void set_executor_options(pool::options pool_options);

/**
 * Get default silicon::coroutine::pool
 */
auto executor() -> std::unique_ptr<silicon::coroutine::pool> &;

#ifdef LIBCORO_FEATURE_NETWORKING
/**
 * Set up default silicon::coroutine::scheduler::options before constructing a single instance of silicon::coroutine::scheduler in
 * silicon::coroutine::default_executor::io_executor()
 * @param scheduler_options scheduler options
 */
void set_io_executor_options(scheduler::options scheduler_options);

/**
 * Get default silicon::coroutine::scheduler
 */
auto io_executor() -> std::unique_ptr<silicon::coroutine::scheduler> &;
#endif

/**
 * Get the perfect default executor
 *
 * This executor is ideal as a default argument in a library,
 * in a place where pool functionality is sufficient,
 * but you don't want to have two executor instances per application for the same thing,
 * one pool and one scheduler.
 */
template<typename return_type>
inline auto perfect() -> return_type & {
#ifdef LIBCORO_FEATURE_NETWORKING
    return io_executor();
#else
    return executor();
#endif
}

} // namespace silicon::coroutine::default_executor
