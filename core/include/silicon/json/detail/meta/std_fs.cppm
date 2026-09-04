












module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#if JSON_HAS_EXPERIMENTAL_FILESYSTEM
#    include <experimental/filesystem>
#elif JSON_HAS_FILESYSTEM
#    include <filesystem>
#endif

export module silicon.json:detail.meta.std_fs;


#if JSON_HAS_EXPERIMENTAL_FILESYSTEM
SILICON_JSON_NAMESPACE_BEGIN
namespace detail {


export namespace std_fs = std::experimental::filesystem;
}
SILICON_JSON_NAMESPACE_END
#elif JSON_HAS_FILESYSTEM
SILICON_JSON_NAMESPACE_BEGIN
namespace detail {
export namespace std_fs = std::filesystem;
}
SILICON_JSON_NAMESPACE_END
#endif
