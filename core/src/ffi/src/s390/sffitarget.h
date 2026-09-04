#ifndef SILICON_FFI_TARGET_H
#define SILICON_FFI_TARGET_H

#ifndef SILICON_FFI_H
#    error "Please do not include sffitarget.h directly into your source.  Use sffi.h instead."
#endif

#if defined(__s390x__)
#    ifndef S390X
#        define S390X
#    endif
#endif

#ifndef SILICON_FFI_ASM
typedef unsigned long sffi_arg;
typedef signed long sffi_sarg;

typedef enum sffi_abi {
    SFFI_FIRST_ABI = 0,
    SFFI_SYSV,
    SFFI_LAST_ABI,
    SFFI_DEFAULT_ABI = SFFI_SYSV
} sffi_abi;
#endif

#define SFFI_TARGET_SPECIFIC_STACK_SPACE_ALLOCATION
#define SFFI_TARGET_HAS_COMPLEX_TYPE
#define SFFI_TARGET_HAS_INT128

#define SFFI_CLOSURES 1
#define SFFI_GO_CLOSURES 1
#ifdef S390X
#    define SFFI_TRAMPOLINE_SIZE 32
#else
#    define SFFI_TRAMPOLINE_SIZE 16
#endif
#define SFFI_NATIVE_RAW_API 0

#endif
