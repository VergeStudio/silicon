/* -*-c-*- */
#include "ffitest.h"
#include <complex.h>

static _Complex T_C_TYPE return_c(_Complex T_C_TYPE c)
{
  printf ("%f,%fi\n", T_CONV creal (c), T_CONV cimag (c));
  return 2 * c;
}
int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  _Complex T_C_TYPE c, rc, rc2;
  T_C_TYPE cr, ci;

  args[0] = &T_FFI_TYPE;
  values[0] = &c;

  /* Initialize the cif */
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
		     &T_FFI_TYPE, args) == SFFI_OK);

  for (cr = -127.0; cr <  127; cr++)
    {
      ci = 1000.0 - cr;
      c = cr + ci * I;
      sffi_call(&cif, SFFI_FN(return_c), &rc, values);
      rc2 = return_c(c);
      printf ("%f,%fi vs %f,%fi\n",
	      T_CONV creal (rc), T_CONV cimag (rc),
	      T_CONV creal (rc2), T_CONV cimag (rc2));
      CHECK(rc == 2 * c);
    }
  exit(0);
}
