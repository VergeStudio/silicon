#include "ffitest.h"

typedef struct
{
  double d1;
  double d2;
} test_structure_2;

static test_structure_2 ABI_ATTR struct2(test_structure_2 ts)
{
  ts.d1--;
  ts.d2--;

  return ts;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  test_structure_2 ts2_arg;
  sffi_type ts2_type;
  sffi_type *ts2_type_elements[3];

  test_structure_2 *ts2_result =
    (test_structure_2 *) malloc (sizeof(test_structure_2));

  ts2_type.size = 0;
  ts2_type.alignment = 0;
  ts2_type.type = SFFI_TYPE_STRUCT;
  ts2_type.elements = ts2_type_elements;
  ts2_type_elements[0] = &sffi_type_double;
  ts2_type_elements[1] = &sffi_type_double;
  ts2_type_elements[2] = NULL;

  args[0] = &ts2_type;
  values[0] = &ts2_arg;

  CHECK(sffi_prep_cif(&cif, ABI_NUM, 1, &ts2_type, args) == SFFI_OK);

  ts2_arg.d1 = 5.55;
  ts2_arg.d2 = 6.66;

  printf ("%g\n", ts2_arg.d1);
  printf ("%g\n", ts2_arg.d2);

  sffi_call(&cif, SFFI_FN(struct2), ts2_result, values);

  printf ("%g\n", ts2_result->d1);
  printf ("%g\n", ts2_result->d2);

  CHECK(ts2_result->d1 == 5.55 - 1);
  CHECK(ts2_result->d2 == 6.66 - 1);

  free (ts2_result);
  exit(0);
}
