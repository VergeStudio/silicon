#include "ffitest.h"
#include "float.h"

#include <math.h>

static long double ldblit(float f)
{
  return (long double) (((long double) f)/ (long double) 3.0);
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  float f;
  long double ld;
  long double original;

  args[0] = &sffi_type_float;
  values[0] = &f;

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
		     &sffi_type_longdouble, args) == SFFI_OK);

  f = 3.14159;

#if defined(__sun) && defined(__GNUC__)

#else
  printf ("%Lf\n", ldblit(f));
#endif

  ld = 666;
  sffi_call(&cif, SFFI_FN(ldblit), &ld, values);

#if defined(__sun) && defined(__GNUC__)

#else
  printf ("%Lf, %Lf, %Lf, %Lf\n", ld, ldblit(f), ld - ldblit(f), LDBL_EPSILON);
#endif

  original = ldblit(f);
  if (((ld > original) ? (ld - original) : (original - ld)) < LDBL_EPSILON)
    puts("long double return value tests ok!");
  else
    CHECK(0);

  exit(0);
}
