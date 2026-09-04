#include "ffitest.h"

static double return_dbl(double dbl)
{
  printf ("%f\n", dbl);
  return 2 * dbl;
}
int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  double dbl, rdbl;

  args[0] = &sffi_type_double;
  values[0] = &dbl;

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
		     &sffi_type_double, args) == SFFI_OK);

  for (dbl = -127.3; dbl <  127; dbl++)
    {
      sffi_call(&cif, SFFI_FN(return_dbl), &rdbl, values);
      printf ("%f vs %f\n", rdbl, return_dbl(dbl));
      CHECK(rdbl == 2 * dbl);
    }
  exit(0);
}
