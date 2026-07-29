module;

module silicon.util;

#include <cstring>
#include <atomic>

namespace silicon::util {

bool HasSuffix(const char *ptrStr, const char *ptrSuffix) {
    const size_t len = strlen(ptrStr);
    const size_t slen = strlen(ptrSuffix);
    return (len >= slen and (memcmp(ptrStr + len - slen, ptrSuffix, slen) == 0));
}

std::uint64_t GenerateUniqueId() {
    static std::atomic<std::uint64_t> s_uniqueId = 0;
    return s_uniqueId.fetch_add(1, std::memory_order_relaxed);
}

} // namespace silicon::util
