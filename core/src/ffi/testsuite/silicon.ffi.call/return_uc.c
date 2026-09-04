#include "ffitest.h"

static unsigned char return_uc(unsigned char uc)
{
  return uc;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  sffi_arg rint;

  unsigned char uc;

  args[0] = &sffi_type_uchar;
  values[0] = &uc;

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
		     &sffi_type_uchar, args) == SFFI_OK);

  for (uc = (unsigned char) '\x00';
       uc < (unsigned char) '\xff'; uc++)
    {
      sffi_call(&cif, SFFI_FN(return_uc), &rint, values);
      CHECK((unsigned char)rint == uc);
    }
  exit(0);
}
