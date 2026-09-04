#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <silicon/test/test.h>






import silicon.cli.error;

namespace {
    const silicon::cli::cli_category_impl g_cli_category;
    struct inject_cli_on_init {
        inject_cli_on_init() {
            silicon::cli::inject_cli_error_category(g_cli_category);
        }
    } _inject_cli_on_init;
}
