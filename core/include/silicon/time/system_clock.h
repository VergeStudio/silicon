#pragma once

#include <chrono>
#include <cstdint>

namespace silicon::time {

/// 默认系统时钟（包装 std::chrono::system_clock）。
///
/// ⚠️ 必须 header-only（global module 实体，成员类内 inline），原因有二：
/// 1. MSVC 对命名模块（named module）实体的修饰名追加模块标签
///    （`?now@system_clock@time@silicon@@...::<!silicon.time>`），clang 不生成
///    该标签 → 任何「MSVC 编译的定义 + clang 引用」的跨工具链链接均无法解析；
/// 2. MSVC IFC 不含成员函数体，clang 导入模块后对成员函数发强引用，无法本地
///    内联展开。
/// 内联成员让每个消费 TU 本地发射（weak）符号，彻底规避跨 DLL/跨工具链符号解析。
class system_clock {
  public:
    std::chrono::system_clock::time_point now() const {
        return std::chrono::system_clock::now();
    }
    std::int64_t now_ms() const {
        using namespace std::chrono;
        return duration_cast<milliseconds>(now().time_since_epoch()).count();
    }
};

} // namespace silicon::time
