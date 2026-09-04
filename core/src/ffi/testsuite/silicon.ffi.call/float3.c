#include "ffitest.h"
#include "float.h"

#include <math.h>

static double floating_1(float a, double b, long double c)
{
  return (double) a + b + (double) c;
}

static double floating_2(long double a, double b, float c)
{
  return (double) a + b + (double) c;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  double rd;

  float f;
  double d;
  long double ld;

  args[0] = &sffi_type_float;
  values[0] = &f;
  args[1] = &sffi_type_double;
  values[1] = &d;
  args[2] = &sffi_type_longdouble;
  values[2] = &ld;

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 3,
		     &sffi_type_double, args) == SFFI_OK);

  f = 3.14159;
  d = (double)1.0/(double)3.0;
  ld = 2.71828182846L;

  floating_1 (f, d, ld);

  sffi_call(&cif, SFFI_FN(floating_1), &rd, values);

  CHECK(fabs(rd - floating_1(f, d, ld)) < DBL_EPSILON);

  args[0] = &sffi_type_longdouble;
  values[0] = &ld;
  args[1] = &sffi_type_double;
  values[1] = &d;
  args[2] = &sffi_type_float;
  values[2] = &f;

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 3,
		     &sffi_type_double, args) == SFFI_OK);

  floating_2 (ld, d, f);

  sffi_call(&cif, SFFI_FN(floating_2), &rd, values);

  CHECK(fabs(rd - floating_2(ld, d, f)) < DBL_EPSILON);

  exit (0);
}
