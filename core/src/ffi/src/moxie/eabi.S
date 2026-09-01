/* -----------------------------------------------------------------------
   eabi.S - Copyright (c) 2012, 2013  Anthony Green
   
   Moxie Assembly glue.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   ``Software''), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.
   ----------------------------------------------------------------------- */

#define SILICON_FFI_ASM	
#include <sfficonfig.h>
#include <sffi.h>

	.globl sffi_prep_args_EABI

	.text
	.p2align 4
	.globl sffi_call_EABI
	.type sffi_call_EABI, @function

	# $r0 :   sffi_prep_args
	# $r1 :   &ecif
	# $r2 :   cif->bytes
	# $r3 :   fig->flags
	# $r4 :   ecif.rvalue
	# $r5 :   fn
	
sffi_call_EABI:
	push	$sp, $r6
	push	$sp, $r7
	push	$sp, $r8
	dec	$sp, 24

	/* Store incoming args on stack.  */
	sto.l	0($sp), $r0 /* sffi_prep_args */
	sto.l	4($sp), $r1 /* ecif */
	sto.l	8($sp), $r2 /* bytes */
	sto.l	12($sp), $r3 /* flags */
	sto.l	16($sp), $r4 /* &rvalue */
	sto.l	20($sp), $r5 /* fn */
	
	/* Call sffi_prep_args.  */
	mov 	$r6, $r4 /* Save result buffer */
	mov	$r7, $r5 /* Save the target fn */
	mov	$r8, $r3 /* Save the flags */
	sub	$sp, $r2 /* Allocate stack space */
	mov	$r0, $sp /* We can stomp over $r0 */
	/* $r1 is already set up */
	jsra 	sffi_prep_args

	/* Load register arguments.  */
	ldo.l	$r0, 0($sp)
	ldo.l	$r1, 4($sp)
	ldo.l	$r2, 8($sp)
	ldo.l	$r3, 12($sp)
	ldo.l	$r4, 16($sp)
	ldo.l	$r5, 20($sp)
	
	/* Call the target function.  */
	jsr	$r7

	ldi.l	$r7, 0xffffffff
	cmp	$r8, $r7
	beq	retstruct

	ldi.l	$r7, 4
	cmp	$r8, $r7
	bgt	ret2reg

	st.l	($r6), $r0
	jmpa	retdone

ret2reg:	
	st.l	($r6), $r0
	sto.l	4($r6), $r1

retstruct:
retdone:	
	/* Return.  */
	ldo.l	$r6, -4($fp)
	ldo.l	$r7, -8($fp)
	ldo.l	$r8, -12($fp)
	ret
	.size sffi_call_EABI, .-sffi_call_EABI
	
