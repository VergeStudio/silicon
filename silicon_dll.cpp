// 聚合 silicon.dll 的入口桩。
// 聚合 DLL 无自有编译单元（仅 add_deps 链接各子模块 static 实现），MSVC 链接器在
// 仅有 .lib 输入时无目标文件可拉入 CRT 启动符 _DllMainCRTStartup，导致
// LNK2001/LNK4001。本桩提供一个真实目标文件 + DllMain 入口，使 CRT 被正确链接。
#include <windows.h>

extern "C" BOOL APIENTRY DllMain(HINSTANCE /*instance*/, DWORD /*reason*/, LPVOID /*reserved*/) {
    return TRUE;
}
