module;

#include <cstring>
#include <atomic>

module silicon.util;

namespace silicon::util {

bool has_suffix(const char *str, const char *suffix) {
    const size_t len = strlen(str);
    const size_t slen = strlen(suffix);
    return (len >= slen and (memcmp(str + len - slen, suffix, slen) == 0));
}

std::uint64_t generate_unique_id() {
    static std::atomic<std::uint64_t> unique_id = 0;
    return unique_id.fetch_add(1, std::memory_order_relaxed);
}

}
