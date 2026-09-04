#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <silicon/test/test.h>

import silicon.ai.llm.error;

namespace {
    const silicon::ai::llm::llm_category_impl g_llm_category;
    struct inject_llm_error_category_on_init {
        inject_llm_error_category_on_init() {
            silicon::ai::llm::inject_llm_error_category(g_llm_category);
        }
    } _inject_llm_error_category_on_init;
}
