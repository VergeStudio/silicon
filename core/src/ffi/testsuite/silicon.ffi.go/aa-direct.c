#include "static-chain.h"

#if defined(__GNUC__) && !defined(__clang__) && defined(STATIC_CHAIN_REG)

#include "ffitest.h"

void *doit(void)
{
  register void *chain __asm__(STATIC_CHAIN_REG);
  return chain;
}

int main()
{
  sffi_cif cif;
  void *result;

  CHECK(sffi_prep_cif(&cif, ABI_NUM, 0, &sffi_type_pointer, NULL) == SFFI_OK);

  sffi_call_go(&cif, SFFI_FN(doit), &result, NULL, &result);

  CHECK(result == &result);

  return 0;
}

#else 
int main() { return 0; }
#endif
