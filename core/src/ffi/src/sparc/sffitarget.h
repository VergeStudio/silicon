#ifndef SILICON_FFI_TARGET_H
#define SILICON_FFI_TARGET_H

#ifndef SILICON_FFI_H
#    error "Please do not include sffitarget.h directly into your source.  Use sffi.h instead."
#endif

#if defined(__arch64__) || defined(__sparcv9)
#    ifndef SPARC64
#        define SPARC64
#    endif
#endif

#ifndef SILICON_FFI_ASM
typedef unsigned long sffi_arg;
typedef signed long sffi_sarg;

typedef enum sffi_abi {
    SFFI_FIRST_ABI = 0,
#    ifdef SPARC64
    SFFI_V9,
    SFFI_DEFAULT_ABI = SFFI_V9,
#    else
    SFFI_V8,
    SFFI_DEFAULT_ABI = SFFI_V8,
#    endif
    SFFI_LAST_ABI
} sffi_abi;
#endif

#define SFFI_TARGET_SPECIFIC_STACK_SPACE_ALLOCATION 1
#define SFFI_TARGET_HAS_COMPLEX_TYPE 1

#ifdef SPARC64
#    define SFFI_TARGET_SPECIFIC_VARIADIC 1
#    define SFFI_EXTRA_CIF_FIELDS unsigned int nfixedargs
#endif

#define SFFI_CLOSURES 1
#define SFFI_GO_CLOSURES 1
#define SFFI_NATIVE_RAW_API 0

#ifdef SPARC64
#    define SFFI_TRAMPOLINE_SIZE 24
#else
#    define SFFI_TRAMPOLINE_SIZE 16
#endif

#endif
