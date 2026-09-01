/* -----------------------------------------------------------------*-C-*-
   sffitarget.h - Copyright (c) 2012 Tilera Corp.
   Target configuration macros for TILE.

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

#ifndef SILICON_FFI_ASM

#    include <arch/abi.h>

typedef uint_reg_t sffi_arg;
typedef int_reg_t sffi_sarg;

typedef enum sffi_abi {
    SFFI_FIRST_ABI = 0,
    SFFI_UNIX,
    SFFI_LAST_ABI,
    SFFI_DEFAULT_ABI = SFFI_UNIX
} sffi_abi;
#endif

/* ---- Definitions for closures ----------------------------------------- */
#define SFFI_CLOSURES 1

#ifdef __tilegx__
/* We always pass 8-byte values, even in -m32 mode. */
#    define SFFI_SIZEOF_ARG 8
#    ifdef __LP64__
#        define SFFI_TRAMPOLINE_SIZE (8 * 5) /* 5 bundles */
#    else
#        define SFFI_TRAMPOLINE_SIZE (8 * 3) /* 3 bundles */
#    endif
#else
#    define SFFI_SIZEOF_ARG 4
#    define SFFI_TRAMPOLINE_SIZE 8 /* 1 bundle */
#endif
#define SFFI_NATIVE_RAW_API 0

#endif
