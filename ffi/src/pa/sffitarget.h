/* -----------------------------------------------------------------*-C-*-
   sffitarget.h - Copyright (c) 2012, 2026  Anthony Green
                 Copyright (c) 1996-2003  Red Hat, Inc.
   Target configuration macros for hppa.

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

#ifndef SILICON_FFI_ASM
typedef unsigned long sffi_arg;
typedef signed long sffi_sarg;

typedef enum sffi_abi {
    SFFI_FIRST_ABI = 0,

#    ifdef PA_LINUX
    SFFI_PA32,
    SFFI_LAST_ABI,
    SFFI_DEFAULT_ABI = SFFI_PA32
#    endif

#    ifdef PA_HPUX
            SFFI_PA32,
    SFFI_LAST_ABI,
    SFFI_DEFAULT_ABI = SFFI_PA32
#    endif

#    ifdef PA64_HPUX
            SFFI_PA64,
    SFFI_LAST_ABI,
    SFFI_DEFAULT_ABI = SFFI_PA64
#    endif
} sffi_abi;
#endif

#define SFFI_TARGET_SPECIFIC_STACK_SPACE_ALLOCATION

/* ---- Definitions for closures ----------------------------------------- */

#define SFFI_CLOSURES 1
#define SFFI_NATIVE_RAW_API 0
#if defined(PA64_HPUX)
#    define SFFI_TRAMPOLINE_SIZE 32
#else
#    define SFFI_TRAMPOLINE_SIZE 12
#endif

#define SFFI_TYPE_SMALL_STRUCT1 -1
#define SFFI_TYPE_SMALL_STRUCT2 -2
#define SFFI_TYPE_SMALL_STRUCT3 -3
#define SFFI_TYPE_SMALL_STRUCT4 -4
#define SFFI_TYPE_SMALL_STRUCT5 -5
#define SFFI_TYPE_SMALL_STRUCT6 -6
#define SFFI_TYPE_SMALL_STRUCT7 -7
#define SFFI_TYPE_SMALL_STRUCT8 -8

/* The return-value jump tables in linux.S and hpux32.S are indexed by
   cif->flags, which sffi_prep_cif_machdep derives from the return type.  Any
   return type it does not handle explicitly -- including SFFI_TYPE_COMPLEX and
   the 128-bit integer types SFFI_TYPE_UINT128/SFFI_TYPE_SINT128 -- falls through
   to the default case and is mapped to SFFI_TYPE_INT, so cif->flags never
   exceeds SFFI_TYPE_COMPLEX and the existing tables remain sufficient.  Bump
   SFFI_PA_TYPE_LAST to the current SFFI_TYPE_LAST once you have confirmed any
   newly added generic type is likewise handled (or the tables extended).  */
#define SFFI_PA_TYPE_LAST SFFI_TYPE_SINT128

/* Tripwire: when a new generic type is added SFFI_TYPE_LAST changes and this
   fires, forcing a review of sffi_prep_cif_machdep and the linux.S / hpux32.S
   jump tables before SFFI_PA_TYPE_LAST above is bumped.  */
#if SFFI_TYPE_LAST != SFFI_PA_TYPE_LAST
#    error "You likely have broken jump tables"
#endif

#endif
