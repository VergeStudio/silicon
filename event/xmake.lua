target("event", function()
    -- 接口模块（moduleonly）：纯抽象接口，不含实现；实现单元归属 event.impl（static）。
    set_kind("moduleonly")

    -- 事件门面走 silicon.proxy 类型擦除，需依赖 proxy 模块（core 内）。
    -- public：silicon.core 的 IFC 经 silicon::event 向消费方传递。
    add_deps("core", {public = true})

    -- 单 DLL 伞宏：本模块编译进 silicon.dll 时，实体经 EVENT_API（SILICON_EXPORT）dllexport。
    add_defines("SILICON_EXPORT")

    add_includedirs("include")

    add_files("include/silicon/event/**.cppm", {public = true})

    -- 生成式 :config 分区（版本信息），由 event.config.cppm.in 产出。
    set_configdir("$(builddir)/silicon/config")
    add_configfiles("event.config.cppm.in")
    add_files("$(builddir)/silicon/config/event.*.cppm", {public = true})
end)

target("event.impl", function()
    -- 实现模块（static）：持有 event 的模块实现单元，链接进单一 silicon.dll。
    set_kind("static")

    -- 单向依赖接口模块（moduleonly）；绝不反向。
    add_deps("silicon::event")

    -- 与接口模块一致：编译进 DLL 时 dllexport。
    add_defines("SILICON_EXPORT")

    add_files("src/**.cpp")
end)
