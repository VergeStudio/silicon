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
    SFFI_SYSV,
    SFFI_LAST_ABI,
    SFFI_DEFAULT_ABI = SFFI_SYSV,
} sffi_abi;
#endif

#ifdef __CSKYABIV2__
#    define SFFI_ASM_ARGREG_SIZE 16
#    define TRAMPOLINE_SIZE 16
#    define SFFI_TRAMPOLINE_SIZE 24
#else
#    define SFFI_ASM_ARGREG_SIZE 24
#    define TRAMPOLINE_SIZE 20
#    define SFFI_TRAMPOLINE_SIZE 28
#endif

#define SFFI_CLOSURES 1
#define SFFI_NATIVE_RAW_API 0
#endif
