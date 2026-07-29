#pragma once

#if defined(_WIN32) && defined(CLI_SHARED_LIB)
#if defined(CLI_EXPORT)
#define CLI_API __declspec(dllexport)
#else
#define CLI_API __declspec(dllimport)
#endif
#else
#define CLI_API
#endif
