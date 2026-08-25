target("di", function()
    set_kind("static")

    add_includedirs("include", {public = true})

    -- di 为纯模板库，仅有模块接口单元（无 .cpp 实现单元）；接口单元已迁移到 include 下 .cppm 分区
    add_files("include/silicon/di/**.cppm", {public = true})

    -- di::core 经 silicon.proxy 类型擦除 context_closure_base 内部接口，须依赖 proxy 模块；
    -- proxy 仅依赖 silicon.exception（而非 core），无循环依赖风险。
    add_deps("silicon::proxy", {configs = {shared = true}})
    add_deps("silicon::error")

    -- 错误码体系由各模块自维护：silicon.di 内置 di_error（inline 于 :core 分区）。

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("di.config.cppm.in")
    add_files("$(builddir)/silicon/config/di.*.cppm", {public = true})
end)
