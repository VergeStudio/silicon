

#ifndef SILICON_FFI_TARGET_H
#define SILICON_FFI_TARGET_H

#ifndef SILICON_FFI_H
#    error "Please do not include sffitarget.h directly into your source.  Use sffi.h instead."
#endif



#ifndef SILICON_FFI_ASM
typedef unsigned long sffi_arg;
typedef signed long sffi_sarg;

typedef enum sffi_abi {
    SFFI_FIRST_ABI = 0,

#    ifdef PA_LINUX
    SFFI_PA32,
    SFFI_LAST_ABI,
    SFFI_DEFAULT_ABI = SFFI_PA32
#    endif

#    ifdef PA_HPUX
            SFFI_PA32,
    SFFI_LAST_ABI,
    SFFI_DEFAULT_ABI = SFFI_PA32
#    endif

#    ifdef PA64_HPUX
            SFFI_PA64,
    SFFI_LAST_ABI,
    SFFI_DEFAULT_ABI = SFFI_PA64
#    endif
} sffi_abi;
#endif

#define SFFI_TARGET_SPECIFIC_STACK_SPACE_ALLOCATION



#define SFFI_CLOSURES 1
#define SFFI_NATIVE_RAW_API 0
#if defined(PA64_HPUX)
#    define SFFI_TRAMPOLINE_SIZE 32
#else
#    define SFFI_TRAMPOLINE_SIZE 12
#endif

#define SFFI_TYPE_SMALL_STRUCT1 -1
#define SFFI_TYPE_SMALL_STRUCT2 -2
#define SFFI_TYPE_SMALL_STRUCT3 -3
#define SFFI_TYPE_SMALL_STRUCT4 -4
#define SFFI_TYPE_SMALL_STRUCT5 -5
#define SFFI_TYPE_SMALL_STRUCT6 -6
#define SFFI_TYPE_SMALL_STRUCT7 -7
#define SFFI_TYPE_SMALL_STRUCT8 -8


#define SFFI_PA_TYPE_LAST SFFI_TYPE_SINT128


#if SFFI_TYPE_LAST != SFFI_PA_TYPE_LAST
#    error "You likely have broken jump tables"
#endif

#endif
