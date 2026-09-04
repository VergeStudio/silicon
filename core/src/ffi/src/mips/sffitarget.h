#ifndef SILICON_FFI_TARGET_H
#define SILICON_FFI_TARGET_H

#ifndef SILICON_FFI_H
#    error "Please do not include sffitarget.h directly into your source.  Use sffi.h instead."
#endif

#ifdef __rtems__

#    define _MIPS_SIM_ABI32 1
#    define _MIPS_SIM_NABI32 2
#    define _MIPS_SIM_ABI64 3
#elif !defined(__OpenBSD__) && !defined(__FreeBSD__) && !defined(__linux__)
#    include <sgidefs.h>
#endif

#ifndef _ABIN32
#    define _ABIN32 _MIPS_SIM_NABI32
#endif
#ifndef _ABI64
#    define _ABI64 _MIPS_SIM_ABI64
#endif
#ifndef _ABIO32
#    define _ABIO32 _MIPS_SIM_ABI32
#endif

#if !defined(_MIPS_SIM)
#    error -- something is very wrong --
#else
#    if (_MIPS_SIM == _ABIN32 && defined(_ABIN32)) || (_MIPS_SIM == _ABI64 && defined(_ABI64))
#        define SFFI_MIPS_N32
#    else
#        if (_MIPS_SIM == _ABIO32 && defined(_ABIO32))
#            define SFFI_MIPS_O32
#        else
#            error -- this is an unsupported platform --
#        endif
#    endif
#endif

#ifdef SFFI_MIPS_O32

#    define SFFI_SIZEOF_ARG 4
#else

#    define SFFI_SIZEOF_ARG 8
#    if _MIPS_SIM == _ABIN32
#        define SFFI_SIZEOF_JAVA_RAW 4
#    endif
#endif

#define SFFI_TARGET_HAS_COMPLEX_TYPE 1
#define SFFI_FLAG_BITS 2

#define SFFI_ARGS_D SFFI_TYPE_DOUBLE
#define SFFI_ARGS_F SFFI_TYPE_FLOAT
#define SFFI_ARGS_DD SFFI_TYPE_DOUBLE * 4 + SFFI_TYPE_DOUBLE
#define SFFI_ARGS_FF SFFI_TYPE_FLOAT * 4 + SFFI_TYPE_FLOAT
#define SFFI_ARGS_FD SFFI_TYPE_DOUBLE * 4 + SFFI_TYPE_FLOAT
#define SFFI_ARGS_DF SFFI_TYPE_FLOAT * 4 + SFFI_TYPE_DOUBLE

#define SFFI_TYPE_SMALLSTRUCT SFFI_TYPE_UINT8
#define SFFI_TYPE_SMALLSTRUCT2 SFFI_TYPE_SINT8

#if 0

#    define SFFI_TYPE_STRUCT_DD ((SFFI_ARGS_DD) << 4) + SFFI_TYPE_STRUCT

#else

#    define SFFI_TYPE_STRUCT_D 61
#    define SFFI_TYPE_STRUCT_F 45
#    define SFFI_TYPE_STRUCT_DD 253
#    define SFFI_TYPE_STRUCT_FF 173
#    define SFFI_TYPE_STRUCT_FD 237
#    define SFFI_TYPE_STRUCT_DF 189
#    define SFFI_TYPE_STRUCT_SMALL 93
#    define SFFI_TYPE_STRUCT_SMALL2 109

#    define SFFI_TYPE_COMPLEX_SMALL 95
#    define SFFI_TYPE_COMPLEX_SMALL2 111
#    define SFFI_TYPE_COMPLEX_FF 47
#    define SFFI_TYPE_COMPLEX_DD 63
#    define SFFI_TYPE_COMPLEX_LDLD 79

#    define SFFI_TYPE_STRUCT_D_SOFT 317
#    define SFFI_TYPE_STRUCT_F_SOFT 301
#    define SFFI_TYPE_STRUCT_DD_SOFT 509
#    define SFFI_TYPE_STRUCT_FF_SOFT 429
#    define SFFI_TYPE_STRUCT_FD_SOFT 493
#    define SFFI_TYPE_STRUCT_DF_SOFT 445
#    define SFFI_TYPE_STRUCT_SOFT 16
#endif

#ifdef SILICON_FFI_ASM
#    define v0 $2
#    define v1 $3
#    define a0 $4
#    define a1 $5
#    define a2 $6
#    define a3 $7
#    define a4 $8
#    define a5 $9
#    define a6 $10
#    define a7 $11
#    define t0 $8
#    define t1 $9
#    define t2 $10
#    define t3 $11
#    define t4 $12
#    define t5 $13
#    define t6 $14
#    define t7 $15
#    define t8 $24
#    define t9 $25
#    define ra $31

#    ifdef SFFI_MIPS_O32
#        define REG_L lw
#        define REG_S sw
#        define SUBU subu
#        define ADDU addu
#        define SRL srl
#        define LI li
#    else 
#        define REG_L ld
#        define REG_S sd
#        define SUBU dsubu
#        define ADDU daddu
#        define SRL dsrl
#        define LI dli
#        if (_MIPS_SIM == _ABI64)
#            define LA dla
#            define EH_FRAME_ALIGN 3
#            define FDE_ADDR_BYTES .8byte
#        else
#            define LA la
#            define EH_FRAME_ALIGN 2
#            define FDE_ADDR_BYTES .4byte
#        endif 
#    endif     
#else          
#    ifdef __GNUC__
#        ifdef SFFI_MIPS_O32

typedef unsigned int sffi_arg __attribute__((__mode__(__SI__)));
typedef signed int sffi_sarg __attribute__((__mode__(__SI__)));
#        else

typedef unsigned int sffi_arg __attribute__((__mode__(__DI__)));
typedef signed int sffi_sarg __attribute__((__mode__(__DI__)));
#        endif
#    else
#        ifdef SFFI_MIPS_O32

typedef __uint32_t sffi_arg;
typedef __int32_t sffi_sarg;
#        else

typedef __uint64_t sffi_arg;
typedef __int64_t sffi_sarg;
#        endif
#    endif 

typedef enum sffi_abi {
    SFFI_FIRST_ABI = 0,
    SFFI_O32,
    SFFI_N32,
    SFFI_N64,
    SFFI_O32_SOFT_FLOAT,
    SFFI_N32_SOFT_FLOAT,
    SFFI_N64_SOFT_FLOAT,
    SFFI_LAST_ABI,

#    ifdef SFFI_MIPS_O32
#        ifdef __mips_soft_float
    SFFI_DEFAULT_ABI = SFFI_O32_SOFT_FLOAT
#        else
    SFFI_DEFAULT_ABI = SFFI_O32
#        endif
#    else
#        if _MIPS_SIM == _ABI64
#            ifdef __mips_soft_float
    SFFI_DEFAULT_ABI = SFFI_N64_SOFT_FLOAT
#            else
    SFFI_DEFAULT_ABI = SFFI_N64
#            endif
#        else
#            ifdef __mips_soft_float
    SFFI_DEFAULT_ABI = SFFI_N32_SOFT_FLOAT
#            else
    SFFI_DEFAULT_ABI = SFFI_N32
#            endif
#        endif
#    endif
} sffi_abi;

#    define SFFI_EXTRA_CIF_FIELDS \
        unsigned rstruct_flag;    \
        unsigned mips_nfixedargs
#    define SFFI_TARGET_SPECIFIC_VARIADIC
#endif 

#define SFFI_CLOSURES 1
#define SFFI_GO_CLOSURES 1
#define SFFI_NATIVE_RAW_API 0

#if defined(SFFI_MIPS_O32) || (_MIPS_SIM == _ABIN32)
#    define SFFI_TRAMPOLINE_SIZE 20
#else
#    define SFFI_TRAMPOLINE_SIZE 56
#endif

#endif
