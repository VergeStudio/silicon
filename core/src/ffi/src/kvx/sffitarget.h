#ifndef SILICON_FFI_TARGET_H
#define SILICON_FFI_TARGET_H

#ifndef SILICON_FFI_H
#    error "Please do not include sffitarget.h directly into your source. Use sffi.h instead."
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

typedef enum kvx_intext_method {
    KVX_RET_NONE = 0,
    KVX_RET_SXBD = 1,
    KVX_RET_SXHD = 2,
    KVX_RET_SXWD = 3,
    KVX_RET_ZXBD = 4,
    KVX_RET_ZXHD = 5,
    KVX_RET_ZXWD = 6
} kvx_intext_method;

#endif

#define SFFI_CLOSURES 1
#define SFFI_TRAMPOLINE_SIZE 0

#define SFFI_NATIVE_RAW_API 0
#define SFFI_TARGET_SPECIFIC_VARIADIC 1
#define SFFI_TARGET_HAS_COMPLEX_TYPE

#endif
