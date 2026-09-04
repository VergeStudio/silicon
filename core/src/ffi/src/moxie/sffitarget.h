

#ifndef SILICON_FFI_TARGET_H
#define SILICON_FFI_TARGET_H



#ifndef SILICON_FFI_ASM
typedef unsigned long sffi_arg;
typedef signed long sffi_sarg;

typedef enum sffi_abi {
    SFFI_FIRST_ABI = 0,
    SFFI_EABI,
    SFFI_DEFAULT_ABI = SFFI_EABI,
    SFFI_LAST_ABI = SFFI_DEFAULT_ABI + 1
} sffi_abi;
#endif



#define SFFI_CLOSURES 1
#define SFFI_NATIVE_RAW_API 0


#define SFFI_TRAMPOLINE_SIZE (12)

#endif
