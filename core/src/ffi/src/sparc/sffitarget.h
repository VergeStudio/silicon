/* -----------------------------------------------------------------*-C-*-
   sffitarget.h - Copyright (c) 2012  Anthony Green
                 Copyright (c) 1996-2003  Red Hat, Inc.
   Target configuration macros for SPARC.

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

#if defined(__arch64__) || defined(__sparcv9)
#    ifndef SPARC64
#        define SPARC64
#    endif
#endif

#ifndef SILICON_FFI_ASM
typedef unsigned long sffi_arg;
typedef signed long sffi_sarg;

typedef enum sffi_abi {
    SFFI_FIRST_ABI = 0,
#    ifdef SPARC64
    SFFI_V9,
    SFFI_DEFAULT_ABI = SFFI_V9,
#    else
    SFFI_V8,
    SFFI_DEFAULT_ABI = SFFI_V8,
#    endif
    SFFI_LAST_ABI
} sffi_abi;
#endif

#define SFFI_TARGET_SPECIFIC_STACK_SPACE_ALLOCATION 1
#define SFFI_TARGET_HAS_COMPLEX_TYPE 1

#ifdef SPARC64
#    define SFFI_TARGET_SPECIFIC_VARIADIC 1
#    define SFFI_EXTRA_CIF_FIELDS unsigned int nfixedargs
#endif

/* ---- Definitions for closures ----------------------------------------- */

#define SFFI_CLOSURES 1
#define SFFI_GO_CLOSURES 1
#define SFFI_NATIVE_RAW_API 0

#ifdef SPARC64
#    define SFFI_TRAMPOLINE_SIZE 24
#else
#    define SFFI_TRAMPOLINE_SIZE 16
#endif

#endif
