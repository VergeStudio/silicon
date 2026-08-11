target("error", function()
    set_kind("$(kind)")

    -- silicon.error：全项目统一错误码体系（std::error_code + 各模块错误枚举）。
    -- 无任何模块依赖（仅标准库 <system_error>），是所有模块错误码的单一来源。

    add_includedirs("include", {public = true})

    -- 接口单元（export module silicon.error;）
    add_files("include/silicon/error/**.cppm", {public = true})

    -- 实现单元（category 与 make_error_code 定义）
    add_files("src/**.cpp")
end)
