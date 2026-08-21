-- silicon.error: 基础模块，提供统一的 result 别名
-- （std::expected<T, std::error_code>），供全项目复用，避免各模块重复声明。
-- 纯接口模块（仅模板别名，无可导出实体），无内部依赖，属叶子模块。
target("error", function()
    set_kind("$(kind)")

    add_includedirs("include", {public = true})
    add_files("include/silicon/error/**.cppm", {public = true})
end)
