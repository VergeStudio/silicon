#ifndef SILICON_SCHEDULER_TASK_COMMON_H
#define SILICON_SCHEDULER_TASK_COMMON_H

#if defined(SILICON_PLATFORM_WINDOWS)
#    if defined(TASK_SHARED_LIB)
#        if defined(TASK_EXPORT)
#            define TASK_API __declspec(dllexport)
#        else
#            define TASK_API __declspec(dllimport)
#        endif
#    else
#        define TASK_API
#    endif
#else
#    if defined(TASK_SHARED_LIB)
#        if defined(TASK_EXPORT)
#            define TASK_API __attribute__((visibility("default")))
#        else
#            define TASK_API
#        endif
#    else
#        define TASK_API
#    endif
#endif

#endif // SILICON_SCHEDULER_TASK_COMMON_H
