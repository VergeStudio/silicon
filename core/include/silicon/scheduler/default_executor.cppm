module;


#include <memory>



#ifdef LIBCORO_FEATURE_NETWORKING
#else
#endif

export module silicon.scheduler:default_executor;

import :io_scheduler;
import :thread_pool;

export namespace silicon::scheduler::default_executor {


void set_executor_options(thread_pool::options);


std::unique_ptr<silicon::scheduler::thread_pool> & executor() ;

#ifdef LIBCORO_FEATURE_NETWORKING

void set_io_executor_options(io_scheduler::options);


std::unique_ptr<silicon::scheduler::io_scheduler> & io_executor() ;
#endif


template<typename return_type>
inline return_type & perfect() {
#ifdef LIBCORO_FEATURE_NETWORKING
    return io_executor();
#else
    return executor();
#endif
}

}
