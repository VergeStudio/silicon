target("json", function()
    -- 纯模块库（仅 .cppm，无 .cpp）：moduleonly 使模块接口与 header unit
    -- 全部默认 public，消费方（json.test/ai/config）才能拿到 silicon.json 的
    -- IFC 与 json.hpp 的 header unit IFC（static/shared 下 header unit 不跨目标
    -- 传递，MSVC 消费方报 C7612）。
    set_kind("moduleonly")

    add_includedirs("include", {public = true})
    add_headerfiles("include/silicon/json/**.hpp", {public = true})
    add_headerfiles("include/silicon/json_impl/**.hpp", {public = true})

    -- json 模块的异常层级：detail::exception 派生自 std::exception，
    -- 不依赖 silicon::exception，故无需附加 silicon 模块依赖。
    add_files("include/silicon/json/**.cppm", {public = true})
end)

target("json.test", function()
    set_kind("binary")
    add_deps("silicon::json", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)
