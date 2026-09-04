#include "ffitest.h"

static signed char return_sc(signed char sc)
{
  return sc;
}
int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  sffi_arg rint;
  signed char sc;

  args[0] = &sffi_type_schar;
  values[0] = &sc;

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
		     &sffi_type_schar, args) == SFFI_OK);

  for (sc = (signed char) -127;
       sc < (signed char) 127; sc++)
    {
      sffi_call(&cif, SFFI_FN(return_sc), &rint, values);
      CHECK((signed char)rint == sc);
    }
  exit(0);
}
