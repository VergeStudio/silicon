target("scheduler", function()
    set_kind("$(kind)")

    -- Windows DLL：C++20 模块附着实体不隐式 inline，MSVC 目标无自动导出，
    -- 统一用 .def 全量导出，保证消费方可跨 DLL 链接模块符号。
    if is_plat("windows") and is_config("kind", "shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end

    if is_os("windows") then
        add_defines("WIN")
    end

    -- NOTE: set_kind("$(kind)") means the target-scope is_kind() cannot match
    -- "shared"; must query the global config variable via is_config() instead.
    if is_config("kind", "shared") then
        add_defines("SCHEDULER_SHARED_LIB", "SCHEDULER_EXPORT", {public = true})
    end

    -- 分区：:ischeduler（抽象基类）、:thread_pool（CPU 线程池，原 silicon.thread）、
    -- :io_scheduler（事件循环，原 silicon.coroutine:scheduler）、
    -- :run_loop（单线程同步执行上下文，参考 stdexec::run_loop）、
    -- :inline_scheduler（内联同步调度器，参考 stdexec::inline_scheduler）、
    -- :parallel_scheduler（系统级并行调度器，参考 stdexec::parallel_scheduler / get_parallel_scheduler()，内部持有 thread_pool）、
    -- :default_executor（进程级默认执行器）、:config。
    --
    -- 另含自 silicon.coroutine 下沉的调度原语（命名空间仍为 silicon::coroutine）：
    -- :concepts.*、:expected、:fd、:poll、:time、:sync_wait、:awaiter_list、
    -- :pipe。依赖方向已单向化为 coroutine -> scheduler -> task。
    add_includedirs("include")

    -- task 目录已并入 scheduler/task（target 名仍为 task）。
    -- 注意：不得再依赖 silicon::coroutine —— coroutine 现在反向依赖本 target。
    add_deps("silicon::task", {configs = {shared = true}})

    -- 错误码体系由各模块自维护：silicon.scheduler 内置 scheduler_error。

    add_files("src/**.cpp")
    add_files("include/silicon/scheduler/**.cppm", {public = true})

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("scheduler.config.cppm.in")
    add_files("$(builddir)/silicon/config/scheduler.*.cppm", {public = true})

end)
