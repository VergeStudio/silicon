#include "ffitest.h"
static long long return_ll(int ll0, long long ll1, int ll2)
{
  CHECK(ll0 == 11111111);
  CHECK(ll1 == 11111111111000LL);
  CHECK(ll2 == 11111111);
  return ll0 + ll1 + ll2;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  long long rlonglong;
  long long ll1;
  unsigned ll0, ll2;

  args[0] = &sffi_type_sint;
  args[1] = &sffi_type_sint64;
  args[2] = &sffi_type_sint;
  values[0] = &ll0;
  values[1] = &ll1;
  values[2] = &ll2;

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 3,
		     &sffi_type_sint64, args) == SFFI_OK);

  ll0 = 11111111;
  ll1 = 11111111111000LL;
  ll2 = 11111111;

  sffi_call(&cif, SFFI_FN(return_ll), &rlonglong, values);
  printf("res: %" PRIdLL ", %" PRIdLL "\n", rlonglong, ll0 + ll1 + ll2);

  CHECK(rlonglong == 11111133333222);
  CHECK(ll0 + ll1 + ll2 == 11111133333222);
  exit(0);
}
