


#include "ffitest.h"

typedef struct
{
  unsigned int ui01;
  unsigned int ui02;
  unsigned int ui03;
  unsigned int ui04;
  unsigned int ui05;
  unsigned int ui06;
  unsigned int ui07;
  unsigned int ui08;
  unsigned int ui09;
  unsigned int ui10;
  unsigned int ui11;
  unsigned int ui12;
  unsigned int ui13;
  unsigned int ui14;
  unsigned int ui15;
  unsigned int ui16;
  unsigned int ui17;
} test_structure_1;

static test_structure_1 ABI_ATTR struct1(test_structure_1 ts)
{
  ts.ui17++;

  return ts;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  sffi_type ts1_type;
  sffi_type *ts1_type_elements[18];

  test_structure_1 ts1_arg;

  
  test_structure_1 *ts1_result =
    (test_structure_1 *) malloc (sizeof(test_structure_1));

  ts1_type.size = 0;
  ts1_type.alignment = 0;
  ts1_type.type = SFFI_TYPE_STRUCT;
  ts1_type.elements = ts1_type_elements;
  ts1_type_elements[0] = &sffi_type_uint;
  ts1_type_elements[1] = &sffi_type_uint;
  ts1_type_elements[2] = &sffi_type_uint;
  ts1_type_elements[3] = &sffi_type_uint;
  ts1_type_elements[4] = &sffi_type_uint;
  ts1_type_elements[5] = &sffi_type_uint;
  ts1_type_elements[6] = &sffi_type_uint;
  ts1_type_elements[7] = &sffi_type_uint;
  ts1_type_elements[8] = &sffi_type_uint;
  ts1_type_elements[9] = &sffi_type_uint;
  ts1_type_elements[10] = &sffi_type_uint;
  ts1_type_elements[11] = &sffi_type_uint;
  ts1_type_elements[12] = &sffi_type_uint;
  ts1_type_elements[13] = &sffi_type_uint;
  ts1_type_elements[14] = &sffi_type_uint;
  ts1_type_elements[15] = &sffi_type_uint;
  ts1_type_elements[16] = &sffi_type_uint;
  ts1_type_elements[17] = NULL;

  args[0] = &ts1_type;
  values[0] = &ts1_arg;

  
  CHECK(sffi_prep_cif(&cif, ABI_NUM, 1,
		     &ts1_type, args) == SFFI_OK);

  ts1_arg.ui17 = 555;

  sffi_call(&cif, SFFI_FN(struct1), ts1_result, values);

  CHECK(ts1_result->ui17 == 556);

  
  CHECK(ts1_arg.ui17 == 555);

  free (ts1_result);
  exit(0);
}
