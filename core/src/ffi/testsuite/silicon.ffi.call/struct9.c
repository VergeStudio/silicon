/* Area:	sffi_call
   Purpose:	Check structures.
   Limitations:	none.
   PR:		none.
   Originator:	From the original ffitest.c  */

/* { dg-do run } */
#include "ffitest.h"

typedef struct
{
  float f;
  int i;
} test_structure_9;

static test_structure_9 ABI_ATTR struct9 (test_structure_9 ts)
{
  ts.f += 1;
  ts.i += 1;

  return ts;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  sffi_type ts9_type;
  sffi_type *ts9_type_elements[3];

  test_structure_9 ts9_arg;

  /* This is a hack to get a properly aligned result buffer */
  test_structure_9 *ts9_result =
    (test_structure_9 *) malloc (sizeof(test_structure_9));

  ts9_type.size = 0;
  ts9_type.alignment = 0;
  ts9_type.type = SFFI_TYPE_STRUCT;
  ts9_type.elements = ts9_type_elements;
  ts9_type_elements[0] = &sffi_type_float;
  ts9_type_elements[1] = &sffi_type_sint;
  ts9_type_elements[2] = NULL;

  args[0] = &ts9_type;
  values[0] = &ts9_arg;
  
  /* Initialize the cif */
  CHECK(sffi_prep_cif(&cif, ABI_NUM, 1, &ts9_type, args) == SFFI_OK);
  
  ts9_arg.f = 5.55f;
  ts9_arg.i = 5;
  
  printf ("%g\n", ts9_arg.f);
  printf ("%d\n", ts9_arg.i);
  
  sffi_call(&cif, SFFI_FN(struct9), ts9_result, values);

  printf ("%g\n", ts9_result->f);
  printf ("%d\n", ts9_result->i);
  
  CHECK(ts9_result->f == 5.55f + 1);
  CHECK(ts9_result->i == 5 + 1);

  free (ts9_result);
  exit(0);
}
