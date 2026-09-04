#ifndef SILICON_FFI_TARGET_H
#define SILICON_FFI_TARGET_H

#ifndef SILICON_FFI_H
#    error "Please do not include sffitarget.h directly into your source.  Use sffi.h instead."
#endif

#ifndef SILICON_FFI_ASM

#    include <arch/abi.h>

typedef uint_reg_t sffi_arg;
typedef int_reg_t sffi_sarg;

typedef enum sffi_abi {
    SFFI_FIRST_ABI = 0,
    SFFI_UNIX,
    SFFI_LAST_ABI,
    SFFI_DEFAULT_ABI = SFFI_UNIX
} sffi_abi;
#endif

#define SFFI_CLOSURES 1

#ifdef __tilegx__

#    define SFFI_SIZEOF_ARG 8
#    ifdef __LP64__
#        define SFFI_TRAMPOLINE_SIZE (8 * 5) 
#    else
#        define SFFI_TRAMPOLINE_SIZE (8 * 3) 
#    endif
#else
#    define SFFI_SIZEOF_ARG 4
#    define SFFI_TRAMPOLINE_SIZE 8 
#endif
#define SFFI_NATIVE_RAW_API 0

#endif
