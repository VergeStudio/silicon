#include "ffitest.h"

typedef struct
{
  int si;
} test_structure_3;

static test_structure_3 ABI_ATTR struct3(test_structure_3 ts)
{
  ts.si = -(ts.si*2);

  return ts;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  int compare_value;
  sffi_type ts3_type;
  sffi_type *ts3_type_elements[2];

  test_structure_3 ts3_arg;
  test_structure_3 *ts3_result =
    (test_structure_3 *) malloc (sizeof(test_structure_3));

  ts3_type.size = 0;
  ts3_type.alignment = 0;
  ts3_type.type = SFFI_TYPE_STRUCT;
  ts3_type.elements = ts3_type_elements;
  ts3_type_elements[0] = &sffi_type_sint;
  ts3_type_elements[1] = NULL;

  args[0] = &ts3_type;
  values[0] = &ts3_arg;

  CHECK(sffi_prep_cif(&cif, ABI_NUM, 1,
		     &ts3_type, args) == SFFI_OK);

  ts3_arg.si = -123;
  compare_value = ts3_arg.si;

  sffi_call(&cif, SFFI_FN(struct3), ts3_result, values);

  printf ("%d %d\n", ts3_result->si, -(compare_value*2));

  CHECK(ts3_result->si == -(compare_value*2));

  free (ts3_result);
  exit(0);
}
