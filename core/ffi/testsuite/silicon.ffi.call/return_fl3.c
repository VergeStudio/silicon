/* Area:	sffi_call
   Purpose:	Check return value float.
   Limitations:	none.
   PR:		none.
   Originator:	<andreast@gcc.gnu.org> 20050212  */

/* { dg-do run } */
#include "ffitest.h"

static float return_fl(float fl1, float fl2, unsigned int in3, float fl4)
{
  return fl1 + fl2 + in3 + fl4;
}
int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  float fl1, fl2, fl4, rfl;
  unsigned int in3;
  args[0] = &sffi_type_float;
  args[1] = &sffi_type_float;
  args[2] = &sffi_type_uint;
  args[3] = &sffi_type_float;
  values[0] = &fl1;
  values[1] = &fl2;
  values[2] = &in3;
  values[3] = &fl4;

  /* Initialize the cif */
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 4,
		     &sffi_type_float, args) == SFFI_OK);
  fl1 = 127.0;
  fl2 = 128.0;
  in3 = 255;
  fl4 = 512.7;

  sffi_call(&cif, SFFI_FN(return_fl), &rfl, values);
  printf ("%f vs %f\n", rfl, return_fl(fl1, fl2, in3, fl4));
  CHECK(rfl ==  fl1 + fl2 + in3 + fl4);
  exit(0);
}
