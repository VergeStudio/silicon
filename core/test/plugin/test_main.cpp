#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <silicon/test/test.h>

import silicon.plugin.error;

namespace {
    const silicon::plugin::plugin_category_impl g_plugin_category;
    struct inject_plugin_on_init {
        inject_plugin_on_init() {
            silicon::plugin::inject_plugin_error_category(g_plugin_category);
        }
    } _inject_plugin_on_init;
}
