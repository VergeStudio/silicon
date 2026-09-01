/* { dg-do run } */

#include "ffitest.h"

void doit(sffi_cif *cif, void *rvalue, void **avalue, void *closure)
{
  (void)cif;
  (void)avalue;
  *(void **)rvalue = closure;
}

typedef void * (*FN)(void);

int main()
{
  sffi_cif cif;
  sffi_go_closure cl;
  void *result;

  CHECK(sffi_prep_cif(&cif, ABI_NUM, 0, &sffi_type_pointer, NULL) == SFFI_OK);
  CHECK(sffi_prep_go_closure(&cl, &cif, doit) == SFFI_OK);

  sffi_call_go(&cif, SFFI_FN(*(FN *)&cl), &result, NULL, &cl);

  CHECK(result == &cl);

  exit(0);
}
