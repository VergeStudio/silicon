#include "ffitest.h"

typedef struct
{
  unsigned ui1;
  unsigned ui2;
  unsigned ui3;
} test_structure_4;

static test_structure_4 ABI_ATTR struct4(test_structure_4 ts)
{
  ts.ui3 = ts.ui1 * ts.ui2 * ts.ui3;

  return ts;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  sffi_type ts4_type;
  sffi_type *ts4_type_elements[4];  

  test_structure_4 ts4_arg;

  test_structure_4 *ts4_result =
    (test_structure_4 *) malloc (sizeof(test_structure_4));

  ts4_type.size = 0;
  ts4_type.alignment = 0;
  ts4_type.type = SFFI_TYPE_STRUCT;
  ts4_type.elements = ts4_type_elements;
  ts4_type_elements[0] = &sffi_type_uint;
  ts4_type_elements[1] = &sffi_type_uint;
  ts4_type_elements[2] = &sffi_type_uint;
  ts4_type_elements[3] = NULL;

  args[0] = &ts4_type;
  values[0] = &ts4_arg;

  CHECK(sffi_prep_cif(&cif, ABI_NUM, 1, &ts4_type, args) == SFFI_OK);

  ts4_arg.ui1 = 2;
  ts4_arg.ui2 = 3;
  ts4_arg.ui3 = 4;

  sffi_call (&cif, SFFI_FN(struct4), ts4_result, values);

  CHECK(ts4_result->ui3 == 2U * 3U * 4U);

  free (ts4_result);
  exit(0);
}
