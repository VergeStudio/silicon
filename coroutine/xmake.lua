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
    add_files("src/**.cppm", {public = true})

    -- 平台专属 io_notifier 后端：仅编译当前平台对应实现；其余平台的
    -- 分区接口与实现单元一并剔除（接口侧以 #if 条件 import 匹配）。
    if is_plat("windows") then
        remove_files("src/detail/io_notifier_epoll.cpp", "src/detail/io_notifier_epoll.cppm",
                     "src/detail/io_notifier_kqueue.cpp", "src/detail/io_notifier_kqueue.cppm")
    elseif is_plat("linux") then
        remove_files("src/detail/io_notifier_iocp.cpp", "src/detail/io_notifier_iocp.cppm",
                     "src/detail/io_notifier_kqueue.cpp", "src/detail/io_notifier_kqueue.cppm")
    else -- macosx / bsd
        remove_files("src/detail/io_notifier_iocp.cpp", "src/detail/io_notifier_iocp.cppm",
                     "src/detail/io_notifier_epoll.cpp", "src/detail/io_notifier_epoll.cppm")
    end

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("coroutine.config.cppm.in")
    add_files("$(builddir)/silicon/config/coroutine.*.cppm", {public = true})


end)
