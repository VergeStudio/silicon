target("time", function()
    -- 接口模块（moduleonly）：仅纯抽象接口（proxy 门面 + 工厂签名 + 类声明），
    -- 不含任何实现代码；实现单元 time.cpp 归属 time.impl（static）。
    set_kind("moduleonly")

    -- clock / date_source 门面走 silicon.proxy 类型擦除，需依赖 proxy 模块（core 内）。
    -- public：silicon.core 的 IFC 经 silicon::time 向消费方（time.test）传递。
    -- 注意 time.impl→time 为 private，故 core 不会经 silicon DLL 的 public 模块链 re-export
    --（避免 shared 目标把混合 .cppm+.cpp 的 core 作为 public 模块依赖触发 xmake 崩溃）。
    add_deps("core", {public = true})

    -- 单 DLL 伞宏：本模块编译进 silicon.dll 时，实体经 TIME_API（SILICON_EXPORT）dllexport。
    -- 不标记 public，避免向消费方（time.test / siliconbuddy）反向传播。
    add_defines("SILICON_EXPORT")

    -- 自包含 include 根：本模块 common.h（<silicon/time/common.h>）在此解析。
    add_includedirs("include")

    add_files("include/silicon/time/**.cppm", {public = true})
end)

target("time.impl", function()
    -- 实现模块（static）：持有 time.cpp（silicon.time 的模块实现单元），
    -- 链接进单一 silicon.dll；可后续扩展 Debug/Release/平台后端变体（单向依赖接口）。
    set_kind("static")

    -- 单向依赖接口模块（moduleonly）；绝不反向。
    add_deps("silicon::time")

    -- 与接口模块一致：编译进 DLL 时 dllexport。
    add_defines("SILICON_EXPORT")

    add_files("src/**.cpp")
end)

target("time.test", function()
    set_kind("binary")
    -- 消费方模式（xmake C++ modules 推荐）：模块 IFC 来自 moduleonly 目标（silicon::time /
    -- silicon::core），符号来自单一聚合 DLL（silicon，import lib）。DLL 的 deps 为 private，
    -- 不 re-export 模块 IFC，规避 shared 目标 public 模块依赖触发的 cxx_sourcebatch nil 崩溃。
    add_deps("silicon::time", "silicon", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)
