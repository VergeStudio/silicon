/* -----------------------------------------------------------------*-C-*-
   sffitarget.h - 2014 Michael Knyszek

   Target configuration macros for RISC-V.

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

#ifndef __riscv
#    error "SILICON_FFI was configured for a RISC-V target but this does not appear to be a RISC-V compiler."
#endif

#ifndef SILICON_FFI_ASM

typedef unsigned long sffi_arg;
typedef signed long sffi_sarg;

/* SFFI_UNUSED_NN and riscv_unused are to maintain ABI compatibility with a
   distributed Berkeley patch from 2014, and can be removed at SONAME bump */
typedef enum sffi_abi {
    SFFI_FIRST_ABI = 0,
    SFFI_SYSV,
    SFFI_UNUSED_1,
    SFFI_UNUSED_2,
    SFFI_UNUSED_3,
    SFFI_LAST_ABI,

    SFFI_DEFAULT_ABI = SFFI_SYSV
} sffi_abi;

#endif /* SILICON_FFI_ASM */

/* ---- Definitions for closures ----------------------------------------- */

#define SFFI_CLOSURES 1
#define SFFI_GO_CLOSURES 1
#define SFFI_TRAMPOLINE_SIZE 24
#define SFFI_NATIVE_RAW_API 0
#define SFFI_EXTRA_CIF_FIELDS  \
    unsigned riscv_nfixedargs; \
    unsigned riscv_unused
#define SFFI_TARGET_SPECIFIC_VARIADIC
#define SFFI_TARGET_HAS_INT128

#endif
