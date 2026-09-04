

#ifndef SILICON_FFI_TARGET_H
#define SILICON_FFI_TARGET_H

#ifndef SILICON_FFI_H
#    error "Please do not include sffitarget.h directly into your source.  Use sffi.h instead."
#endif




#define X86_ANY

#if defined(X86_64) && defined(__i386__)
#    undef X86_64
#    define X86
#endif

#ifdef X86_WIN64
#    define SFFI_SIZEOF_ARG 8
#    define USE_BUILTIN_FFS 0 
#endif

#define SFFI_TARGET_SPECIFIC_STACK_SPACE_ALLOCATION
#ifndef _MSC_VER
#    define SFFI_TARGET_HAS_COMPLEX_TYPE
#endif

#ifdef X86_64
#    define SFFI_TARGET_HAS_INT128
#endif



#ifndef SILICON_FFI_ASM
#    ifdef X86_WIN64
#        ifdef _MSC_VER
typedef unsigned __int64 sffi_arg;
typedef __int64 sffi_sarg;
#        else
typedef unsigned long long sffi_arg;
typedef long long sffi_sarg;
#        endif
#    else
#        if defined __x86_64__ && defined __ILP32__
#            define SFFI_SIZEOF_ARG 8
#            define SFFI_SIZEOF_JAVA_RAW 4
typedef unsigned long long sffi_arg;
typedef long long sffi_sarg;
#        else
typedef unsigned long sffi_arg;
typedef signed long sffi_sarg;
#        endif
#    endif

typedef enum sffi_abi {
#    if defined(X86_WIN64)
    SFFI_FIRST_ABI = 0,
    SFFI_WIN64,  
    SFFI_GNUW64, 
    SFFI_LAST_ABI,
#        ifdef __GNUC__
    SFFI_DEFAULT_ABI = SFFI_GNUW64
#        else
    SFFI_DEFAULT_ABI = SFFI_WIN64
#        endif

#    elif defined(X86_64) || (defined(__x86_64__) && defined(X86_DARWIN))
    SFFI_FIRST_ABI = 1,
    SFFI_UNIX64,
    SFFI_WIN64,
    SFFI_EFI64 = SFFI_WIN64,
    SFFI_GNUW64,
    SFFI_LAST_ABI,
    SFFI_DEFAULT_ABI = SFFI_UNIX64

#    elif defined(X86_WIN32)
    SFFI_FIRST_ABI = 0,
    SFFI_SYSV = 1,
    SFFI_STDCALL = 2,
    SFFI_THISCALL = 3,
    SFFI_FASTCALL = 4,
    SFFI_MS_CDECL = 5,
    SFFI_PASCAL = 6,
    SFFI_REGISTER = 7,
    SFFI_LAST_ABI,
    SFFI_DEFAULT_ABI = SFFI_MS_CDECL
#    else
    SFFI_FIRST_ABI = 0,
    SFFI_SYSV = 1,
    SFFI_THISCALL = 3,
    SFFI_FASTCALL = 4,
    SFFI_STDCALL = 5,
    SFFI_PASCAL = 6,
    SFFI_REGISTER = 7,
    SFFI_MS_CDECL = 8,
    SFFI_LAST_ABI,
    SFFI_DEFAULT_ABI = SFFI_SYSV
#    endif
} sffi_abi;
#endif



#define SFFI_CLOSURES 1
#define SFFI_GO_CLOSURES 1

#define SFFI_TYPE_SMALL_STRUCT_1B (SFFI_TYPE_LAST + 1)
#define SFFI_TYPE_SMALL_STRUCT_2B (SFFI_TYPE_LAST + 2)
#define SFFI_TYPE_SMALL_STRUCT_4B (SFFI_TYPE_LAST + 3)
#define SFFI_TYPE_MS_STRUCT (SFFI_TYPE_LAST + 4)

#if defined(X86_64) || defined(X86_WIN64) || (defined(__x86_64__) && defined(X86_DARWIN))

#    define SFFI_TRAMPOLINE_SIZE 32
#    define SFFI_NATIVE_RAW_API 0
#else

#    define SFFI_TRAMPOLINE_SIZE 16
#    define SFFI_NATIVE_RAW_API 1 
#endif

#if !defined(GENERATE_sffi_MAP) && defined(__CET__)
#    include <cet.h>
#    if (__CET__ & 1) != 0
#        define ENDBR_PRESENT
#    endif
#    define _CET_NOTRACK notrack
#else
#    define _CET_ENDBR
#    define _CET_NOTRACK
#endif

#endif
