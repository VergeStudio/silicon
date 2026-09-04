#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <silicon/test/test.h>





import silicon.coroutine.error;

namespace {
    const silicon::coroutine::coroutine_category_impl g_coroutine_category;
    const silicon::coroutine::channel_category_impl g_channel_category;
    struct inject_coroutine_on_init {
        inject_coroutine_on_init() {
            silicon::coroutine::inject_coroutine_error_category(g_coroutine_category);
            silicon::coroutine::inject_channel_error_category(g_channel_category);
        }
    } _inject_coroutine_on_init;
}
