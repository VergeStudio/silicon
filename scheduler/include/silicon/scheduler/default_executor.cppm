module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
#include <memory>



#ifdef LIBCORO_FEATURE_NETWORKING
#else
#endif

export module silicon.scheduler:default_executor;

import :io_scheduler;
import :thread_pool;

export namespace silicon::scheduler::default_executor {

/**
 * Set up default silicon::scheduler::thread_pool::options before constructing a single instance of silicon::scheduler::thread_pool in
 * silicon::scheduler::default_executor::executor()
 * @param pool_options pool options
 */
void set_executor_options(thread_pool::options);

/**
 * Get default silicon::scheduler::thread_pool
 */
std::unique_ptr<silicon::scheduler::thread_pool> & executor() ;

#ifdef LIBCORO_FEATURE_NETWORKING
/**
 * Set up default silicon::scheduler::io_scheduler::options before constructing a single instance of silicon::scheduler::io_scheduler in
 * silicon::scheduler::default_executor::io_executor()
 * @param scheduler_options scheduler options
 */
void set_io_executor_options(io_scheduler::options);

/**
 * Get default silicon::scheduler::io_scheduler
 */
std::unique_ptr<silicon::scheduler::io_scheduler> & io_executor() ;
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
inline return_type & perfect() {
#ifdef LIBCORO_FEATURE_NETWORKING
    return io_executor();
#else
    return executor();
#endif
}

} // namespace silicon::scheduler::default_executor
