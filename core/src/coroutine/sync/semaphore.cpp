module;

#include "silicon/common.h"

module silicon.coroutine;

namespace silicon::coroutine {
using namespace std::string_literals;

SILICON_CORE_API std::string semaphore_acquire_result_acquired = "acquired"s;
SILICON_CORE_API std::string semaphore_acquire_result_shutdown = "shutdown"s;
SILICON_CORE_API std::string semaphore_acquire_result_unknown = "unknown"s;

SILICON_CORE_API auto to_string(semaphore_acquire_result result) -> const std::string & {
    switch(result) {
        case semaphore_acquire_result::kAcquired:
            return semaphore_acquire_result_acquired;
        case semaphore_acquire_result::kShutdown:
            return semaphore_acquire_result_shutdown;
    }

    return semaphore_acquire_result_unknown;
}

}
