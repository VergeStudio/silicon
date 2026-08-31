add_requires("c-ares")

target("network", function()
    set_kind("moduleonly")

    if is_os("windows") then
        -- NOMINMAX/WIN32_LEAN_AND_MEAN must be set before any Windows header
        -- (winsock2.h via the network sources) is included, otherwise
        -- minwindef.h defines max/min macros that break std::max/std::min.
        add_defines("WIN", "NOMINMAX", "WIN32_LEAN_AND_MEAN")
    end

    -- 依赖方向保持单向：network -> coroutine -> scheduler -> task。
    -- 依赖方向保持单向：network -> coroutine（scheduler 实体已并入 core.dll，
    -- 经 add_deps("silicon::coroutine") 的 public IFC + core 导入库解析）。
    -- 错误码体系由各模块自维护：silicon.network 内置 network_error + network_category。
    add_deps("silicon::coroutine", "core")

    -- 单 DLL 伞宏：本模块编译进 silicon.dll 时，实体经 NET_API（SILICON_EXPORT）dllexport。
    add_defines("SILICON_EXPORT")

    add_includedirs("include", {public = true})
    add_packages("c-ares", {public = true})

    -- 接口分区（主接口 + 各子分区）+ 生成的 :config 分区。
    add_files("include/silicon/network/**.cppm", {public = true})

    -- 生成的 :config 分区（版本信息），由 network.config.cppm.in 产出。
    set_configdir("$(builddir)/silicon/config")
    add_configfiles("network.config.cppm.in")
    add_files("$(builddir)/silicon/config/network.*.cppm", {public = true})
end)

target("network.impl", function()
    set_kind("static")

    add_deps("silicon::network")

    add_defines("SILICON_EXPORT")

    if is_os("windows") then
        add_syslinks("ws2_32", "crypt32")
    end

    -- 实现单元（传统 .cpp，但均为模块实现单元）。
    -- 平台差异（io_status_linux.cpp / io_status_win.cpp）由文件内互斥 #if 守卫选择。
    add_files("src/**.cpp")
end)
