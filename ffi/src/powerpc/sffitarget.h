/* -----------------------------------------------------------------*-C-*-
   sffitarget.h - Copyright (c) 2012, 2026  Anthony Green
                 Copyright (C) 2007, 2008, 2010 Free Software Foundation, Inc
                 Copyright (c) 1996-2003  Red Hat, Inc.

   Target configuration macros for PowerPC.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   ``Software''), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

   ----------------------------------------------------------------------- */

#ifndef SILICON_FFI_TARGET_H
#define SILICON_FFI_TARGET_H

#ifndef SILICON_FFI_H
#    error "Please do not include sffitarget.h directly into your source.  Use sffi.h instead."
#endif

/* ---- System specific configurations ----------------------------------- */

#if defined(POWERPC) && defined(__powerpc64__) /* linux64 */
#    ifndef POWERPC64
#        define POWERPC64
#    endif
#elif defined(POWERPC_DARWIN) && defined(__ppc64__) /* Darwin64 */
#    ifndef POWERPC64
#        define POWERPC64
#    endif
#    ifndef POWERPC_DARWIN64
#        define POWERPC_DARWIN64
#    endif
#elif defined(POWERPC_AIX) && defined(__64BIT__) /* AIX64 */
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
    /* The SFFI_COMPAT values are used by old code.  Since SILICON_FFI may be
     a shared library we have to support old values for backwards
     compatibility.  */
    SFFI_COMPAT_SYSV,
    SFFI_COMPAT_GCC_SYSV,
    SFFI_COMPAT_LINUX64,
    SFFI_COMPAT_LINUX,
    SFFI_COMPAT_LINUX_SOFT_FLOAT,

#        if defined(POWERPC64)
    /* This bit, always set in new code, must not be set in any of the
     old SFFI_COMPAT values that might be used for 64-bit linux.  We
     only need worry about SFFI_COMPAT_LINUX64, but to be safe avoid
     all old values.  */
    SFFI_LINUX = 8,
    /* This and following bits can reuse SFFI_COMPAT values.  */
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
    /* This bit, always set in new code, must not be set in any of the
     old SFFI_COMPAT values that might be used for 32-bit linux/sysv/bsd.  */
    SFFI_SYSV = 8,
    /* This and following bits can reuse SFFI_COMPAT values.  */
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

/* ---- Definitions for closures ----------------------------------------- */

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

/* Complex types are supported on ELFv2 (the only PowerPC64 variant where
   the assembly and C-side passing/return logic has been wired up).  Under
   ELFv2, float/double _Complex are passed and returned as a 2-element
   homogeneous floating-point aggregate, but each scalar half consumes a
   GPR shadow slot of its own — i.e. the same way the underlying C ABI
   handles them, which is what GCC's split_complex_arg emits.  */
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
#    else /* POWERPC || POWERPC_AIX */
#        define SFFI_TRAMPOLINE_SIZE 40
#    endif
#endif

#endif
