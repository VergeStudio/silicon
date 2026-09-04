#include "ffitest.h"

typedef struct { int v[8]; } big_struct;

static int ABI_ATTR
sum_big (big_struct s)
{
  int i, sum = 0;
  for (i = 0; i < 8; i++)
    sum += s.v[i];
  return sum;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[1];
  void *values[1];
  sffi_type bs_type;
  sffi_type *bs_elements[9];
  big_struct in;
  sffi_arg result = 0;
  int i, expected = 0;

  bs_type.size = 0;
  bs_type.alignment = 0;
  bs_type.type = SFFI_TYPE_STRUCT;
  for (i = 0; i < 8; i++)
    bs_elements[i] = &sffi_type_sint;
  bs_elements[8] = NULL;
  bs_type.elements = bs_elements;

  for (i = 0; i < 8; i++)
    {
      in.v[i] = 0x1111 * (i + 1);
      expected += in.v[i];
    }

  args[0] = &bs_type;
  values[0] = &in;

  CHECK(sffi_prep_cif(&cif, ABI_NUM, 1, &sffi_type_sint, args) == SFFI_OK);

  sffi_call(&cif, SFFI_FN(sum_big), &result, values);

  CHECK((int) result == expected);

  exit(0);
}
