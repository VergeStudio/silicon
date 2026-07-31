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

    -- 平台专属 io_notifier 后端（iocp / epoll / kqueue）的类声明已合并进单一分区
    -- src/io_notifier.cppm（:io_notifier）：该分区按平台宏展开当前后端，并始终导出
    -- 三个 constexpr 可用标志以保持非空（规避 clang 22 零导出分区崩溃）。三个同名
    -- .cpp 实现单元由宏开关决定是否编译。因此这里不再按平台 remove_files。

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("coroutine.config.cppm.in")
    add_files("$(builddir)/silicon/config/coroutine.*.cppm", {public = true})


end)
