#pragma once

#if defined(_WIN32) && defined(EVENT_SHARED_LIB)
#if defined(EVENT_EXPORT)
#define EVENT_API __declspec(dllexport)
#else
#define EVENT_API __declspec(dllimport)
#endif
#else
#define EVENT_API
#endif
