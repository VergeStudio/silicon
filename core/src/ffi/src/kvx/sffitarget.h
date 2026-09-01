/* -----------------------------------------------------------------------
   sffitarget.h - Copyright (c) 2020 Kalray

   KVX Target configuration macros

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
#    error "Please do not include sffitarget.h directly into your source. Use sffi.h instead."
#endif

/* ---- System specific configurations ----------------------------------- */

#ifndef SILICON_FFI_ASM
typedef unsigned long sffi_arg;
typedef signed long sffi_sarg;

typedef enum sffi_abi {
    SFFI_FIRST_ABI = 0,
    SFFI_SYSV,
    SFFI_LAST_ABI,
    SFFI_DEFAULT_ABI = SFFI_SYSV
} sffi_abi;

/* Those values are set depending on return type
 * they are used in the assembly code in sysv.S
 */
typedef enum kvx_intext_method {
    KVX_RET_NONE = 0,
    KVX_RET_SXBD = 1,
    KVX_RET_SXHD = 2,
    KVX_RET_SXWD = 3,
    KVX_RET_ZXBD = 4,
    KVX_RET_ZXHD = 5,
    KVX_RET_ZXWD = 6
} kvx_intext_method;

#endif

/* ---- Definitions for closures ----------------------------------------- */

/* This is only to allow Python to compile
 * but closures are not supported yet
 */
#define SFFI_CLOSURES 1
#define SFFI_TRAMPOLINE_SIZE 0

#define SFFI_NATIVE_RAW_API 0
#define SFFI_TARGET_SPECIFIC_VARIADIC 1
#define SFFI_TARGET_HAS_COMPLEX_TYPE

#endif
