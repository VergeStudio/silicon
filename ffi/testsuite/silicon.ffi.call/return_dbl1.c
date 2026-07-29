/* Area:	sffi_call
   Purpose:	Check return value double.
   Limitations:	none.
   PR:		none.
   Originator:	<andreast@gcc.gnu.org> 20050212  */

/* { dg-do run } */
#include "ffitest.h"

static double return_dbl(double dbl1, float fl2, unsigned int in3, double dbl4)
{
  return dbl1 + fl2 + in3 + dbl4;
}
int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  double dbl1, dbl4, rdbl;
  float fl2;
  unsigned int in3;
  args[0] = &sffi_type_double;
  args[1] = &sffi_type_float;
  args[2] = &sffi_type_uint;
  args[3] = &sffi_type_double;
  values[0] = &dbl1;
  values[1] = &fl2;
  values[2] = &in3;
  values[3] = &dbl4;

  /* Initialize the cif */
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 4,
		     &sffi_type_double, args) == SFFI_OK);
  dbl1 = 127.0;
  fl2 = 128.0;
  in3 = 255;
  dbl4 = 512.7;

  sffi_call(&cif, SFFI_FN(return_dbl), &rdbl, values);
  printf ("%f vs %f\n", rdbl, return_dbl(dbl1, fl2, in3, dbl4));
  CHECK(rdbl ==  dbl1 + fl2 + in3 + dbl4);
  exit(0);
}
