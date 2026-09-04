#include "ffitest.h"
typedef struct
{
  float f1;
  float f2;
  double d;
} test_structure_7;

static test_structure_7 ABI_ATTR struct7 (test_structure_7 ts)
{
  ts.f1 += 1;
  ts.f2 += 1;
  ts.d += 1;

  return ts;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  sffi_type ts7_type;
  sffi_type *ts7_type_elements[4];

  test_structure_7 ts7_arg;

  test_structure_7 *ts7_result =
    (test_structure_7 *) malloc (sizeof(test_structure_7));

  ts7_type.size = 0;
  ts7_type.alignment = 0;
  ts7_type.type = SFFI_TYPE_STRUCT;
  ts7_type.elements = ts7_type_elements;
  ts7_type_elements[0] = &sffi_type_float;
  ts7_type_elements[1] = &sffi_type_float;
  ts7_type_elements[2] = &sffi_type_double;
  ts7_type_elements[3] = NULL;

  args[0] = &ts7_type;
  values[0] = &ts7_arg;

  CHECK(sffi_prep_cif(&cif, ABI_NUM, 1, &ts7_type, args) == SFFI_OK);

  ts7_arg.f1 = 5.55f;
  ts7_arg.f2 = 55.5f;
  ts7_arg.d = 6.66;

  printf ("%g\n", ts7_arg.f1);
  printf ("%g\n", ts7_arg.f2);
  printf ("%g\n", ts7_arg.d);

  sffi_call(&cif, SFFI_FN(struct7), ts7_result, values);

  printf ("%g\n", ts7_result->f1);
  printf ("%g\n", ts7_result->f2);
  printf ("%g\n", ts7_result->d);

  CHECK(ts7_result->f1 == 5.55f + 1);
  CHECK(ts7_result->f2 == 55.5f + 1);
  CHECK(ts7_result->d == 6.66 + 1);

  free (ts7_result);
  exit(0);
}
