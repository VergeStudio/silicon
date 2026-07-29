#ifndef SILICON_LOGGER_COMMON_H
#define SILICON_LOGGER_COMMON_H

#if defined(_WIN32) || defined(_WIN64)
#    if defined(LOGGER_SHARED_LIB)
#        if defined(LOGGER_EXPORT)
#            define LOGGER_API __declspec(dllexport)
#        else
#            define LOGGER_API __declspec(dllimport)
#        endif
#    else
#        define LOGGER_API
#    endif
#else
#    if defined(LOGGER_SHARED_LIB)
#        if defined(LOGGER_EXPORT)
#            define LOGGER_API __attribute__((visibility("default")))
#        else
#            define LOGGER_API
#        endif
#    else
#        define LOGGER_API
#    endif
#endif

#endif // SILICON_LOGGER_COMMON_H
