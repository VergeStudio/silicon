target("coroutine", function()
    set_kind("$(kind)")

    if is_plat("windows") then
        add_defines("WIN")
    end

    if is_kind("shared") then
        add_defines("COROUTINE_SHARED_LIB", "COROUTINE_EXPORT", {public = true})
    end

    -- NOTE: `network` is intentionally NOT a dependency here. coroutine only
    -- consumes network types under LIBCORO_FEATURE_NETWORKING (disabled by
    -- default), and network (a classic lib) already depends back on coroutine,
    -- which would form a circular target dependency. Keeping the edge one-way
    -- (network -> coroutine) breaks the cycle.
    add_deps("core", "task", "thread", {configs = {shared = true}})

    add_includedirs("include", {public = true})
    add_headerfiles("include/silicon/coroutine/**.hpp")

    add_files("src/**.cpp")
    add_files("include/silicon/coroutine/**.cppm", {public = true})

    -- 平台专属 io_notifier 后端（iocp / epoll / kqueue）统一为单一导出类
    -- silicon::coroutine::io_notifier（位于接口单元 include/silicon/coroutine/io_notifier.cppm，
    -- 即 :io_notifier 分区）。
    -- 该类对外接口在所有平台完全一致；平台专属状态隐藏在私有的嵌套 struct Impl（PIMPL）
    -- 中，Impl 的实体定义落在各自平台的 .cpp 实现单元
    -- （io_notifier_iocp.cpp / io_notifier_kqueue.cpp / io_notifier_epoll.cpp），由宏开关
    -- 决定哪个 .cpp 实际提供方法体。因此这里不再按平台 remove_files。

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("coroutine.config.cppm.in")
    add_files("$(builddir)/silicon/config/coroutine.*.cppm", {public = true})


end)

target("coroutine.test", function()
    set_kind("binary")
    add_deps("silicon::coroutine", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)
