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

    -- 平台专属 io_notifier 后端（iocp / epoll / kqueue）始终加入编译；各
    -- .cpp/.cppm 内部以标准预定义宏（_WIN32 / __linux__ / __APPLE__ 等）围栏，
    -- 非目标平台编译为空翻译单元，仅当前平台分区被 coroutine.cppm 无条件
    -- re-export 选取。因此这里不再按平台 remove_files。

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("coroutine.config.cppm.in")
    add_files("$(builddir)/silicon/config/coroutine.*.cppm", {public = true})


end)
