



#include "ffitest.h"
#include "sffi_common.h"

static UINT32 ABI_ATTR align_arguments(UINT32 l1,
				       UINT64 l2)
{
  return l1 + (UINT32) l2;
}

int main(void)
{
  sffi_cif cif;
  sffi_type *args[4] = {
    &sffi_type_uint32,
    &sffi_type_uint64
  };
  sffi_arg lr1, lr2;
  UINT32 l1 = 1;
  UINT64 l2 = 2;
  void *values[2] = {&l1, &l2};

  
  CHECK(sffi_prep_cif(&cif, ABI_NUM, 2,
		     &sffi_type_uint32, args) == SFFI_OK);

  lr1 = align_arguments(l1, l2);

  sffi_call(&cif, SFFI_FN(align_arguments), &lr2, values);

  if (lr1 == lr2)
    printf("bpo-38748 arguments tests ok!\n");
  else
    CHECK(0);
  exit(0);
}
