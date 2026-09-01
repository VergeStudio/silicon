/* Area:	sffi_call
   Purpose:	Check return value float.
   Limitations:	none.
   PR:		none.
   Originator:	<andreast@gcc.gnu.org> 20050212  */

/* { dg-do run } */
#include "ffitest.h"

static float return_fl(float fl1, float fl2)
{
  return fl1 + fl2;
}
int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  float fl1, fl2, rfl;

  args[0] = &sffi_type_float;
  args[1] = &sffi_type_float;
  values[0] = &fl1;
  values[1] = &fl2;

  /* Initialize the cif */
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 2,
		     &sffi_type_float, args) == SFFI_OK);
  fl1 = 127.0;
  fl2 = 128.0;

  sffi_call(&cif, SFFI_FN(return_fl), &rfl, values);
  printf ("%f vs %f\n", rfl, return_fl(fl1, fl2));
  CHECK(rfl ==  fl1 + fl2);
  exit(0);
}
