target("json", function()
    -- 纯模块库（仅 .cppm，无 .cpp）：moduleonly 使模块接口 IFC 默认 public，
    -- 消费方（json.test/ai/config）才能拿到 silicon.json 的 IFC。
    --
    -- 完整 JSON 实现（forked nlohmann/json）已内联进 json_impl.cppm 的
    -- global module fragment（原 json.hpp 伞头已删除并内联），作为真正的命名
    -- 模块只产出一份 BMI；消费方只 import 该模块，故不存在共享 header-unit
    -- 缓存条目，消除 clean 后 -j4 全量构建的 header-unit BMI 并发写 C3474 竞态。
    set_kind("moduleonly")

    add_includedirs("include", {public = true})

    -- json 模块的异常层级：detail::exception 派生自 std::exception，
    -- 不依赖 silicon::exception，故无需附加 silicon 模块依赖。
    -- detail/*.hpp 等实现仅在 json_impl.cppm 的 GMF 内被 include，属模块内部，
    -- 不再作为 public header 暴露。
    add_files("include/silicon/json/**.cppm", {public = true})
    add_files("include/silicon/json_impl/json_impl.cppm", {public = true})
end)

target("json.test", function()
    set_kind("binary")
    add_deps("silicon::json", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)
