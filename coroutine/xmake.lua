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
    -- 调度原语与事件循环已下沉到 silicon.scheduler；coroutine 单向依赖之
    -- （coroutine -> scheduler -> task），primary interface 对其整体 re-export。
    add_deps("core", "task", "scheduler", {configs = {shared = true}})

    add_includedirs("include", {public = true})
    add_headerfiles("include/silicon/coroutine/**.hpp")

    add_files("src/**.cpp")
    add_files("include/silicon/coroutine/**.cppm", {public = true})

    -- 注：平台专属 io_notifier 后端（iocp / epoll / kqueue）连同 poll_info /
    -- timer_handle / io_scheduler 已迁至 silicon.scheduler，不再由本 target 编译。

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
