/* Area:	sffi_call
   Purpose:	Check return value float.
   Limitations:	none.
   PR:		none.
   Originator:	<andreast@gcc.gnu.org> 20050212  */

/* { dg-do run } */
#include "ffitest.h"

static float return_fl(float fl)
{
  return 2 * fl;
}
int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  float fl, rfl;

  args[0] = &sffi_type_float;
  values[0] = &fl;

  /* Initialize the cif */
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
		     &sffi_type_float, args) == SFFI_OK);

  for (fl = -127.0; fl <  127; fl++)
    {
      sffi_call(&cif, SFFI_FN(return_fl), &rfl, values);
      printf ("%f vs %f\n", rfl, return_fl(fl));
      CHECK(rfl ==  2 * fl);
    }
  exit(0);
}
