#pragma once

#if defined(_WIN32) && defined(CONFIG_SHARED_LIB)
#if defined(CONFIG_EXPORT)
#define CONFIG_API __declspec(dllexport)
#else
#define CONFIG_API __declspec(dllimport)
#endif
#else
#define CONFIG_API
#endif
