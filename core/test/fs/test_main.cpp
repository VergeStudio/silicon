#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <silicon/test/test.h>

import silicon.fs.error;

namespace {
    const silicon::fs::fs_category_impl g_fs_category;
    struct inject_fs_on_init {
        inject_fs_on_init() {
            silicon::fs::inject_fs_error_category(g_fs_category);
        }
    } _inject_fs_on_init;
}
