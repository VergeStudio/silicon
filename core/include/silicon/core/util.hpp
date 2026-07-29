#ifndef SILICON_CORE_UTIL_H
#define SILICON_CORE_UTIL_H

#include <cstdio>
#include <cstring>
#include <string>
#include <queue>
#include <mutex>

namespace silicon::util {

bool HasSuffix(const char *, const char *);

std::uint64_t GenerateUniqueId();

} // namespace silicon::util

#endif // SILICON_CORE_UTIL_H