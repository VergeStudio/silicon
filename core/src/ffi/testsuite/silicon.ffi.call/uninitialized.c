
#include "ffitest.h"

typedef struct
{
  unsigned char uc;
  double d;
  unsigned int ui;
} test_structure_1;

static test_structure_1 struct1(test_structure_1 ts)
{
  ts.uc++;
  ts.d--;
  ts.ui++;

  return ts;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  sffi_type ts1_type;
  sffi_type *ts1_type_elements[4];

  memset(&cif, 1, sizeof(cif));
  ts1_type.size = 0;
  ts1_type.alignment = 0;
  ts1_type.type = SFFI_TYPE_STRUCT;
  ts1_type.elements = ts1_type_elements;
  ts1_type_elements[0] = &sffi_type_uchar;
  ts1_type_elements[1] = &sffi_type_double;
  ts1_type_elements[2] = &sffi_type_uint;
  ts1_type_elements[3] = NULL;

  test_structure_1 ts1_arg;
  
  test_structure_1 *ts1_result =
    (test_structure_1 *) malloc (sizeof(test_structure_1));

  args[0] = &ts1_type;
  values[0] = &ts1_arg;

  
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
		     &ts1_type, args) == SFFI_OK);

  ts1_arg.uc = '\x01';
  ts1_arg.d = 3.14159;
  ts1_arg.ui = 555;

  sffi_call(&cif, SFFI_FN(struct1), ts1_result, values);

  CHECK(ts1_result->ui == 556);
  CHECK(ts1_result->d == 3.14159 - 1);

  free (ts1_result);
  exit(0);
}
