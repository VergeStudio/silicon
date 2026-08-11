target("di", function()
    set_kind("$(kind)")

    add_includedirs("include", {public = true})

    -- di 为纯模板库，仅有模块接口单元（无 .cpp 实现单元）；接口单元已迁移到 include 下 .cppm 分区
    add_files("include/silicon/di/**.cppm", {public = true})

    -- silicon::error 为零依赖基础模块，提供统一的 std::error_code 错误码体系；
    -- di 的 resolve 链路失败统一以 di_error 枚举经 make_error_code 返回。
    add_deps("silicon::error")

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("di.config.cppm.in")
    add_files("$(builddir)/silicon/config/di.*.cppm", {public = true})
end)
