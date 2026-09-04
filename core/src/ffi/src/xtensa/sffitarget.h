

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
    SFFI_DEFAULT_ABI = SFFI_SYSV
} sffi_abi;
#endif

#define SFFI_REGISTER_NARGS 6
#define XTENSA_STACK_ALIGNMENT 16
#define SFFI_REGISTER_ARGS_SPACE ((SFFI_REGISTER_NARGS * 4 + XTENSA_STACK_ALIGNMENT - 1) & -XTENSA_STACK_ALIGNMENT)



#define SFFI_CLOSURES 1
#define SFFI_NATIVE_RAW_API 0
#define SFFI_TRAMPOLINE_SIZE 24

#endif
