#ifndef SILICON_AI_COMMON_H
#define SILICON_AI_COMMON_H






#if defined(SILICON_PLATFORM_WINDOWS)
#    if defined(AI_SHARED_LIB)
#        if defined(AI_EXPORT)
#            define AI_API __declspec(dllexport)
#        else
#            define AI_API
#        endif
#    else
#        define AI_API
#    endif
#else
#    if defined(AI_SHARED_LIB)
#        if defined(AI_EXPORT)
#            define AI_API __attribute__((visibility("default")))
#        else
#            define AI_API
#        endif
#    else
#        define AI_API
#    endif
#endif

#endif
