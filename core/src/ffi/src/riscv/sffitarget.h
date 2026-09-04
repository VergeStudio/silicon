#ifndef SILICON_FFI_TARGET_H
#define SILICON_FFI_TARGET_H

#ifndef SILICON_FFI_H
#    error "Please do not include sffitarget.h directly into your source.  Use sffi.h instead."
#endif

#ifndef __riscv
#    error "SILICON_FFI was configured for a RISC-V target but this does not appear to be a RISC-V compiler."
#endif

#ifndef SILICON_FFI_ASM

typedef unsigned long sffi_arg;
typedef signed long sffi_sarg;

typedef enum sffi_abi {
    SFFI_FIRST_ABI = 0,
    SFFI_SYSV,
    SFFI_UNUSED_1,
    SFFI_UNUSED_2,
    SFFI_UNUSED_3,
    SFFI_LAST_ABI,

    SFFI_DEFAULT_ABI = SFFI_SYSV
} sffi_abi;

#endif 

#define SFFI_CLOSURES 1
#define SFFI_GO_CLOSURES 1
#define SFFI_TRAMPOLINE_SIZE 24
#define SFFI_NATIVE_RAW_API 0
#define SFFI_EXTRA_CIF_FIELDS  \
    unsigned riscv_nfixedargs; \
    unsigned riscv_unused
#define SFFI_TARGET_SPECIFIC_VARIADIC
#define SFFI_TARGET_HAS_INT128

#endif
