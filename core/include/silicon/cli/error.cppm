module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>

#include "silicon/common.h"
export module silicon.cli.error;

import silicon.error;

// cli_error / make_error_code / cli_category / cli_category_impl / cli_error_category_instance
// 已折为 header-only 全局实体（见 cli_error_defs.h），由消费方（parser_types.h / 测试 /
// 组合根）直接 #include；本模块保留导出壳，供 legacy `import silicon.cli.error` 引用。
