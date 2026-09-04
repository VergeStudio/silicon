

#ifndef SILICON_FFI_TARGET_H
#define SILICON_FFI_TARGET_H

#ifndef SILICON_FFI_H
#    error "Please do not include sffitarget.h directly into your source.  Use sffi.h instead."
#endif



#if defined(POWERPC) && defined(__powerpc64__) 
#    ifndef POWERPC64
#        define POWERPC64
#    endif
#elif defined(POWERPC_DARWIN) && defined(__ppc64__) 
#    ifndef POWERPC64
#        define POWERPC64
#    endif
#    ifndef POWERPC_DARWIN64
#        define POWERPC_DARWIN64
#    endif
#elif defined(POWERPC_AIX) && defined(__64BIT__) 
#    ifndef POWERPC64
#        define POWERPC64
#    endif
#endif

#ifndef SILICON_FFI_ASM
typedef unsigned long sffi_arg;
typedef signed long sffi_sarg;

typedef enum sffi_abi {
    SFFI_FIRST_ABI = 0,

#    if defined(POWERPC_AIX)
    SFFI_AIX,
    SFFI_DARWIN,
    SFFI_DEFAULT_ABI = SFFI_AIX,
    SFFI_LAST_ABI

#    elif defined(POWERPC_DARWIN)
    SFFI_AIX,
    SFFI_DARWIN,
    SFFI_DEFAULT_ABI = SFFI_DARWIN,
    SFFI_LAST_ABI

#    else
    
    SFFI_COMPAT_SYSV,
    SFFI_COMPAT_GCC_SYSV,
    SFFI_COMPAT_LINUX64,
    SFFI_COMPAT_LINUX,
    SFFI_COMPAT_LINUX_SOFT_FLOAT,

#        if defined(POWERPC64)
    
    SFFI_LINUX = 8,
    
    SFFI_LINUX_STRUCT_ALIGN = 1,
    SFFI_LINUX_LONG_DOUBLE_128 = 2,
    SFFI_LINUX_LONG_DOUBLE_IEEE128 = 4,
    SFFI_DEFAULT_ABI = (SFFI_LINUX
#            ifdef __STRUCT_PARM_ALIGN__
                        | SFFI_LINUX_STRUCT_ALIGN
#            endif
#            ifdef __LONG_DOUBLE_128__
                        | SFFI_LINUX_LONG_DOUBLE_128
#                ifdef __LONG_DOUBLE_IEEE128__
                        | SFFI_LINUX_LONG_DOUBLE_IEEE128
#                endif
#            endif
    ),
    SFFI_LAST_ABI = 16

#        else
    
    SFFI_SYSV = 8,
    
    SFFI_SYSV_SOFT_FLOAT = 1,
    SFFI_SYSV_STRUCT_RET = 2,
    SFFI_SYSV_IBM_LONG_DOUBLE = 4,
    SFFI_SYSV_LONG_DOUBLE_128 = 16,

    SFFI_DEFAULT_ABI = (SFFI_SYSV
#            ifdef __NO_FPRS__
                        | SFFI_SYSV_SOFT_FLOAT
#            endif
#            if (defined(__SVR4_STRUCT_RETURN) || defined(POWERPC_FREEBSD) && !defined(__AIX_STRUCT_RETURN))
                        | SFFI_SYSV_STRUCT_RET
#            endif
#            if __LDBL_MANT_DIG__ == 106
                        | SFFI_SYSV_IBM_LONG_DOUBLE
#            endif
#            ifdef __LONG_DOUBLE_128__
                        | SFFI_SYSV_LONG_DOUBLE_128
#            endif
    ),
    SFFI_LAST_ABI = 32
#        endif
#    endif

} sffi_abi;
#endif



#define SFFI_CLOSURES 1
#define SFFI_NATIVE_RAW_API 0
#if defined(POWERPC) || defined(POWERPC_FREEBSD)
#    define SFFI_GO_CLOSURES 1
#    define SFFI_TARGET_SPECIFIC_VARIADIC 1
#    define SFFI_EXTRA_CIF_FIELDS unsigned nfixedargs
#endif
#if defined(POWERPC_AIX)
#    define SFFI_GO_CLOSURES 1
#endif


#if defined(POWERPC64) && _CALL_ELF == 2
#    define SFFI_TARGET_HAS_COMPLEX_TYPE
#endif

#if _CALL_ELF == 2
#    define SFFI_TRAMPOLINE_SIZE 32
#else
#    if defined(POWERPC64) || defined(POWERPC_AIX)
#        if defined(POWERPC_DARWIN64)
#            define SFFI_TRAMPOLINE_SIZE 48
#        else
#            define SFFI_TRAMPOLINE_SIZE 24
#        endif
#    else 
#        define SFFI_TRAMPOLINE_SIZE 40
#    endif
#endif

#endif
