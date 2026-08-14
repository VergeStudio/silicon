#ifndef SILICON_SCHEDULER_COMMON_H
#define SILICON_SCHEDULER_COMMON_H

#if defined(SILICON_PLATFORM_WINDOWS)
#    if defined(SCHEDULER_SHARED_LIB)
#        if defined(SCHEDULER_EXPORT)
#            define SCHEDULER_API __declspec(dllexport)
#        else
#            define SCHEDULER_API __declspec(dllimport)
#        endif
#    else
#        define SCHEDULER_API
#    endif
#else
#    if defined(SCHEDULER_SHARED_LIB)
#        if defined(SCHEDULER_EXPORT)
#            define SCHEDULER_API __attribute__((visibility("default")))
#        else
#            define SCHEDULER_API
#        endif
#    else
#        define SCHEDULER_API
#    endif
#endif

#endif // SILICON_SCHEDULER_COMMON_H
