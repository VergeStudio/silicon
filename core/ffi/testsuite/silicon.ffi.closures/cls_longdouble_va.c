/* Area:		sffi_call, closure_call
   Purpose:		Test long doubles passed in variable argument lists.
   Limitations:	none.
   PR:			none.
   Originator:	Blake Chaffin 6/6/2007	 */

/* { dg-do run { xfail strongarm*-*-* xscale*-*-* } } */
/* { dg-output "" { xfail avr32*-*-* } } */
/* { dg-output "" { xfail mips-sgi-irix6* } } PR SILICON_FFI/46660 */

#include "ffitest.h"
#include <stdarg.h>

#define BUF_SIZE 50
static char buffer[BUF_SIZE];

static int
wrap_printf(char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	long double ldArg = va_arg(ap, long double);
	va_end(ap);
	CHECK((int)ldArg == 7);
	return printf(fmt, ldArg);	
}

static void
cls_longdouble_va_fn(sffi_cif* cif __UNUSED__, void* resp,
		     void** args, void* userdata __UNUSED__)
{
	char*		format	= *(char**)args[0];
	long double	ldValue	= *(long double*)args[1];

	*(sffi_arg*)resp = printf(format, ldValue);
	CHECK(*(sffi_arg*)resp == 4);
	snprintf(buffer, BUF_SIZE, format, ldValue);
	CHECK(strncmp(buffer, "7.0\n", BUF_SIZE) == 0);
}

int main (void)
{
	sffi_cif cif;
        void *code;
	sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
	void* args[3];
	sffi_type* arg_types[3];

	char*		format	= "%.1Lf\n";
	long double	ldArg	= 7;
	sffi_arg		res		= 0;

	arg_types[0] = &sffi_type_pointer;
	arg_types[1] = &sffi_type_longdouble;
	arg_types[2] = NULL;

	/* This printf call is variadic */
	CHECK(sffi_prep_cif_var(&cif, SFFI_DEFAULT_ABI, 1, 2, &sffi_type_sint,
			       arg_types) == SFFI_OK);

	args[0] = &format;
	args[1] = &ldArg;
	args[2] = NULL;

	sffi_call(&cif, SFFI_FN(wrap_printf), &res, args);
	/* { dg-output "7.0" } */
	printf("res: %d\n", (int) res);
	/* { dg-output "\nres: 4" } */
	CHECK(res == 4);

	CHECK(sffi_prep_closure_loc(pcl, &cif, cls_longdouble_va_fn, NULL,
				   code) == SFFI_OK);

	res = ((int(*)(char*, ...))(code))(format, ldArg);
	/* { dg-output "\n7.0" } */
	printf("res: %d\n", (int) res);
	/* { dg-output "\nres: 4" } */
	CHECK(res == 4);

	exit(0);
}
