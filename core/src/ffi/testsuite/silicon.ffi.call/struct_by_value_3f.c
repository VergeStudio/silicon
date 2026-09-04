#include "ffitest.h"

typedef struct
{
  float f01;
  float f02;
  float f03;
} test_structure_1;

static test_structure_1 ABI_ATTR struct1(test_structure_1 ts)
{
  ts.f03++;

  return ts;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  sffi_type ts1_type;
  sffi_type *ts1_type_elements[5];

  test_structure_1 ts1_arg;

  test_structure_1 *ts1_result =
    (test_structure_1 *) malloc (sizeof(test_structure_1));

  ts1_type.size = 0;
  ts1_type.alignment = 0;
  ts1_type.type = SFFI_TYPE_STRUCT;
  ts1_type.elements = ts1_type_elements;
  ts1_type_elements[0] = &sffi_type_float;
  ts1_type_elements[1] = &sffi_type_float;
  ts1_type_elements[2] = &sffi_type_float;
  ts1_type_elements[3] = NULL;

  args[0] = &ts1_type;
  values[0] = &ts1_arg;

  CHECK(sffi_prep_cif(&cif, ABI_NUM, 1,
		     &ts1_type, args) == SFFI_OK);

  ts1_arg.f03 = 555.5;

  sffi_call(&cif, SFFI_FN(struct1), ts1_result, values);

  CHECK(fabs(ts1_result->f03 - 556.5) < FLT_EPSILON);

  CHECK(fabs(ts1_arg.f03 - 555.5) < FLT_EPSILON);

  free (ts1_result);
  exit(0);
}
