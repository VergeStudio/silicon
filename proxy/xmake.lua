target("proxy", function()
    set_kind("$(kind)")

    -- Windows DLL：C++20 模块附着实体不隐式 inline，MSVC 目标无自动导出，
    -- 统一用 .def 全量导出，保证消费方可跨 DLL 链接模块符号。
    if is_plat("windows") and is_config("kind", "shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end

    if is_os("windows") then
        add_defines("WIN")
    end

    -- clang-cl 对标准 C++20 属性 [[no_unique_address]] 误报 unknown-attribute，
    -- 该属性在 MSFT proxy 库中被广泛使用；抑制此误报以免触发 -Werror 红线。
    -- 消费方实例化 proxy 模板时同样会命中该误报，故设为 public 向下传递。
    add_cxflags("-Wno-unknown-attributes", {public = true})

    if is_config("kind", "shared") then
        add_defines("PROXY_SHARED_LIB", "PROXY_EXPORT", {public = true})
    end

    add_includedirs("include", {public = true})
    -- proxy_macros.h 需随库安装：dispatch 宏无法经模块导出，消费方必须
    -- 在全局模块片段 #include 后才能定义自己的 facade。
    add_headerfiles("include/silicon/proxy/common.h", "include/silicon/proxy/proxy_macros.h")
    -- silicon.exception 模块由 core 目标编译，依赖 core 以获取其 BMI。
    add_deps("silicon::core")

    add_files("include/silicon/proxy/**.cppm", {public = true})

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("proxy.config.cppm.in")
    add_files("$(builddir)/silicon/config/proxy.*.cppm", {public = true})

end)
