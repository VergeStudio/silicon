/* Area:		sffi_prep_cif
   Purpose:		Test error return for bad typedefs.
   Limitations:	none.
   PR:			none.
   Originator:	Blake Chaffin 6/6/2007	 */

/* { dg-do run } */

#include "ffitest.h"

int main (void)
{
	sffi_cif cif;
	sffi_type* arg_types[1];

	sffi_type	badType	= sffi_type_void;

	arg_types[0] = NULL;

	badType.size = 0;

	CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 0, &badType,
		arg_types) == SFFI_BAD_TYPEDEF);

	exit(0);
}
