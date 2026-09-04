



#include "ffitest.h"

static int checking(int a, short b, signed char c)
{

  return (a < 0 && b < 0 && c < 0);
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  sffi_arg rint;

  signed int si;
  signed short ss;
  signed char sc;

  args[0] = &sffi_type_sint;
  values[0] = &si;
  args[1] = &sffi_type_sshort;
  values[1] = &ss;
  args[2] = &sffi_type_schar;
  values[2] = &sc;

  
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 3,
		     &sffi_type_sint, args) == SFFI_OK);

  si = -6;
  ss = -12;
  sc = -1;

  checking (si, ss, sc);

  sffi_call(&cif, SFFI_FN(checking), &rint, values);

  printf ("%d vs %d\n", (int)rint, checking (si, ss, sc));

  CHECK(rint != 0);

  exit (0);
}
