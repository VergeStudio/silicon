/* Area:	sffi_call
   Purpose:	Check return value float.
   Limitations:	none.
   PR:		none.
   Originator:	From the original ffitest.c  */

/* { dg-do run } */

#include "ffitest.h"

static int floating(int a, float b, double c, long double d)
{
  int i;

  i = (int) ((float)a/b + ((float)c/(float)d));

  return i;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  sffi_arg rint;

  float f;
  signed int si1;
  double d;
  long double ld;

  args[0] = &sffi_type_sint;
  values[0] = &si1;
  args[1] = &sffi_type_float;
  values[1] = &f;
  args[2] = &sffi_type_double;
  values[2] = &d;
  args[3] = &sffi_type_longdouble;
  values[3] = &ld;

  /* Initialize the cif */
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 4,
		     &sffi_type_sint, args) == SFFI_OK);

  si1 = 6;
  f = 3.14159;
  d = (double)1.0/(double)3.0;
  ld = 2.71828182846L;

  floating (si1, f, d, ld);

  sffi_call(&cif, SFFI_FN(floating), &rint, values);

  printf ("%d vs %d\n", (int)rint, floating (si1, f, d, ld));

  CHECK((int)rint == floating(si1, f, d, ld));

  exit (0);
}
