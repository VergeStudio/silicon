module;

// CORE_API（键控 CORE_EXPORT）宏来源；定义侧须标注。
#include "silicon/common.h"

module silicon.coroutine;


namespace silicon::coroutine {
using namespace std::string_literals;

// 变量与自由函数不随 DLL 自动导出（MSVC），
// 定义侧与声明侧（semaphore.cppm）均标 CORE_API。
CORE_API std::string semaphore_acquire_result_acquired = "acquired"s;
CORE_API std::string semaphore_acquire_result_shutdown = "shutdown"s;
CORE_API std::string semaphore_acquire_result_unknown = "unknown"s;

CORE_API auto to_string(semaphore_acquire_result result) -> const std::string & {
    switch(result) {
        case semaphore_acquire_result::kAcquired:
            return semaphore_acquire_result_acquired;
        case semaphore_acquire_result::kShutdown:
            return semaphore_acquire_result_shutdown;
    }

    return semaphore_acquire_result_unknown;
}

} // namespace silicon::coroutine
