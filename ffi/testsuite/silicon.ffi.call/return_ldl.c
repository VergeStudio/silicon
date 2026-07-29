/* Area:	sffi_call
   Purpose:	Check return value long double.
   Limitations:	none.
   PR:		none.
   Originator:	<andreast@gcc.gnu.org> 20071113  */
/* { dg-do run } */

#include "ffitest.h"

static long double return_ldl(long double ldl)
{
  return 2*ldl;
}
int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  long double ldl, rldl;

  args[0] = &sffi_type_longdouble;
  values[0] = &ldl;

  /* Initialize the cif */
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
		     &sffi_type_longdouble, args) == SFFI_OK);

  for (ldl = -127.0; ldl <  127.0; ldl++)
    {
      sffi_call(&cif, SFFI_FN(return_ldl), &rldl, values);
      CHECK(rldl ==  2 * ldl);
    }
  exit(0);
}
