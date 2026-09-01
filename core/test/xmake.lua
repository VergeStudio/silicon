target("test", function()
    -- 纯头文件库（doctest 封装，无编译单元），headeronly 仅传播 include 路径。
    set_kind("headeronly")
    add_includedirs("include", {public = true})
    add_includedirs("doctest", {public = true})
    add_headerfiles("include/silicon/test/**.hpp")
    add_headerfiles("doctest/doctest.h")
end)
