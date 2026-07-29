target("test", function()
    -- 纯头文件库（doctest 封装，无任何编译单元）：headeronly 仅传播
    -- include 路径。没有 .cpp/.cppm 就没有目标文件，物理上不存在可产出的
    -- DLL，故不参与 $(kind) 切换。
    set_kind("headeronly")
    add_includedirs("include", {public = true})
    add_includedirs("doctest", {public = true})
    add_headerfiles("include/silicon/test/**.hpp")
    add_headerfiles("doctest/doctest.h")
end)
