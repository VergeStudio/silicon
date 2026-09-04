#ifndef SILICON_FFI_TARGET_H
#define SILICON_FFI_TARGET_H

#ifndef SILICON_FFI_H
#    error \
            "Please do not include sffitarget.h directly into your source.  Use sffi.h instead."
#endif

#ifndef __loongarch__
#    error \
            "SILICON_FFI was configured for a LoongArch target but this does not appear to be a LoongArch compiler."
#endif

#ifndef SILICON_FFI_ASM

typedef unsigned long sffi_arg;
typedef signed long sffi_sarg;

typedef enum sffi_abi {
    SFFI_FIRST_ABI = 0,
    SFFI_LP64S,
    SFFI_LP64F,
    SFFI_LP64D,
    SFFI_ILP32S,
    SFFI_ILP32F,
    SFFI_ILP32D,
    SFFI_LAST_ABI,

#    if __loongarch_grlen == 64
#        if defined(__loongarch_soft_float)
    SFFI_DEFAULT_ABI = SFFI_LP64S
#        elif defined(__loongarch_single_float)
    SFFI_DEFAULT_ABI = SFFI_LP64F
#        elif defined(__loongarch_double_float)
    SFFI_DEFAULT_ABI = SFFI_LP64D
#        else
#            error unsupported LoongArch floating-point ABI
#        endif
#    elif __loongarch_grlen == 32
#        if defined(__loongarch_soft_float)
    SFFI_DEFAULT_ABI = SFFI_ILP32S
#        elif defined(__loongarch_single_float)
    SFFI_DEFAULT_ABI = SFFI_ILP32F
#        elif defined(__loongarch_double_float)
    SFFI_DEFAULT_ABI = SFFI_ILP32D
#        else
#            error unsupported LoongArch floating-point ABI
#        endif
#    else
#        error unsupported LoongArch base architecture
#    endif
} sffi_abi;

#endif 

#define SFFI_TARGET_HAS_INT128

#define SFFI_CLOSURES 1
#define SFFI_GO_CLOSURES 1
#define SFFI_TRAMPOLINE_SIZE 24
#define SFFI_NATIVE_RAW_API 0
#define SFFI_EXTRA_CIF_FIELDS      \
    unsigned loongarch_nfixedargs; \
    unsigned loongarch_unused
#define SFFI_TARGET_SPECIFIC_VARIADIC
#endif
