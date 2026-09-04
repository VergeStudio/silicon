#include "ffitest.h"
static long return_sl(long l1, long l2)
{
  CHECK(l1 == 1073741823L);
  CHECK(l2 == 1073741824L);
  return l1 - l2;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  sffi_arg res;
  unsigned long l1, l2;

  args[0] = &sffi_type_slong;
  args[1] = &sffi_type_slong;
  values[0] = &l1;
  values[1] = &l2;

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 2,
		     &sffi_type_slong, args) == SFFI_OK);

  l1 = 1073741823L;
  l2 = 1073741824L;

  sffi_call(&cif, SFFI_FN(return_sl), &res, values);
  printf("res: %ld, %ld\n", (long)res, l1 - l2);

  CHECK((long)res == -1);
  CHECK(l1 + 1 == l2);

  exit(0);
}
