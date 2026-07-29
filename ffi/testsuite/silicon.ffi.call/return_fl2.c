/* Area:	sffi_call
   Purpose:	Check return value float.
   Limitations:	none.
   PR:		none.
   Originator:	<andreast@gcc.gnu.org> 20050212  */

/* { dg-do run } */
#include "ffitest.h"

/* Use volatile float to avoid false negative on ix86.  See PR target/323.  */
static float return_fl(float fl1, float fl2, float fl3, float fl4)
{
  volatile float sum;

  sum = fl1 + fl2 + fl3 + fl4;
  return sum;
}
int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  float fl1, fl2, fl3, fl4, rfl;
  volatile float sum;

  args[0] = &sffi_type_float;
  args[1] = &sffi_type_float;
  args[2] = &sffi_type_float;
  args[3] = &sffi_type_float;
  values[0] = &fl1;
  values[1] = &fl2;
  values[2] = &fl3;
  values[3] = &fl4;

  /* Initialize the cif */
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 4,
		     &sffi_type_float, args) == SFFI_OK);
  fl1 = 127.0;
  fl2 = 128.0;
  fl3 = 255.1;
  fl4 = 512.7;

  sffi_call(&cif, SFFI_FN(return_fl), &rfl, values);
  printf ("%f vs %f\n", rfl, return_fl(fl1, fl2, fl3, fl4));

  sum = fl1 + fl2 + fl3 + fl4;
  CHECK(rfl == sum);
  exit(0);
}
