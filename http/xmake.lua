target("http", function()
    -- 接口模块（moduleonly）：纯抽象接口（proxy 门面），不含实现。
    set_kind("moduleonly")

    -- http_client 门面走 silicon.proxy 类型擦除，需依赖 proxy 模块（core 内）。
    add_deps("core")

    -- 单 DLL 伞宏：本模块编译进 silicon.dll 时，实体经 HTTP_API（SILICON_EXPORT）dllexport。
    add_defines("SILICON_EXPORT")

    add_files("include/silicon/http/**.cppm", {public = true})
end)

target("http.impl", function()
    set_kind("static")

    add_deps("silicon::http")

    add_defines("SILICON_EXPORT")

    add_files("src/**.cpp")
end)
