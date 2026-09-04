#ifndef SILICON_FFI_TARGET_H
#define SILICON_FFI_TARGET_H

#ifndef SILICON_FFI_H
#    error "Please do not include sffitarget.h directly into your source.  Use sffi.h instead."
#endif

typedef unsigned long sffi_arg;
typedef signed long sffi_sarg;

typedef void (*sffi_fp)(void);

typedef enum sffi_abi {
    SFFI_FIRST_ABI = 0,
#if __SIZEOF_POINTER__ == 4
    SFFI_WASM32,
    SFFI_WASM32_EMSCRIPTEN,
#elif __SIZEOF_POINTER__ == 8
    SFFI_WASM64,
    SFFI_WASM64_EMSCRIPTEN,
#else
#    error "Unknown pointer size"
#endif
    SFFI_LAST_ABI,
#if __SIZEOF_POINTER__ == 4
#    ifdef __EMSCRIPTEN__
    SFFI_DEFAULT_ABI = SFFI_WASM32_EMSCRIPTEN
#    else
    SFFI_DEFAULT_ABI = SFFI_WASM32
#    endif
#elif __SIZEOF_POINTER__ == 8
#    ifdef __EMSCRIPTEN__
    SFFI_DEFAULT_ABI = SFFI_WASM64_EMSCRIPTEN
#    else
    SFFI_DEFAULT_ABI = SFFI_WASM64
#    endif
#else
#    error "Unknown pointer size"
#endif
} sffi_abi;

#define SFFI_CLOSURES 1

#define SFFI_TRAMPOLINE_SIZE 4

#define SFFI_TARGET_SPECIFIC_VARIADIC 1
#define SFFI_EXTRA_CIF_FIELDS unsigned int nfixedargs

#endif
