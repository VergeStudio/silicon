#include <cstddef>
#include <filesystem>
#include <memory>
#include <silicon/test/test.h>
#include <string>
#include <vector>

import silicon.fs;

using namespace silicon::fs;

TEST_CASE("create_file_system 返回非空的当前平台实现") {
    auto fs = create_file_system();
    REQUIRE(fs);
}

TEST_CASE("文本写入后可读回且内容完整") {
    auto fs = create_file_system();
    REQUIRE(fs);
    const std::string path = "silicon_fs_test.txt";
    REQUIRE(static_cast<bool>(fs->write(path, "line1\nline2\n")));
    auto r = fs->read(path);
    REQUIRE(static_cast<bool>(r));
    REQUIRE(r.value().find("line1") != std::string::npos);
    REQUIRE(r.value().find("line2") != std::string::npos);
    std::filesystem::remove(path);
}

TEST_CASE("二进制读写精确往返") {
    auto fs = create_file_system();
    REQUIRE(fs);
    const std::string path = "silicon_fs_test.bin";
    std::vector<std::byte> data{std::byte{0}, std::byte{255}, std::byte{10}, std::byte{13}, std::byte{65}};
    REQUIRE(static_cast<bool>(fs->write_binary(path, data)));
    auto r = fs->read_binary(path);
    REQUIRE(static_cast<bool>(r));
    REQUIRE(r.value().size() == data.size());
    REQUIRE(r.value() == data);
    std::filesystem::remove(path);
}
