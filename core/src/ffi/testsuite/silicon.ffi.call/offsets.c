/* Area:		Struct layout
   Purpose:		Test sffi_get_struct_offsets
   Limitations:		none.
   PR:			none.
   Originator: 		Tom Tromey. */

/* { dg-do run } */
#include "ffitest.h"
#include <stddef.h>

struct test_1
{
  char c;
  float f;
  char c2;
  int i;
};

int
main (void)
{
  sffi_type test_1_type;
  sffi_type *test_1_elements[5];
  size_t test_1_offsets[4];

  test_1_elements[0] = &sffi_type_schar;
  test_1_elements[1] = &sffi_type_float;
  test_1_elements[2] = &sffi_type_schar;
  test_1_elements[3] = &sffi_type_sint;
  test_1_elements[4] = NULL;

  test_1_type.size = 0;
  test_1_type.alignment = 0;
  test_1_type.type = SFFI_TYPE_STRUCT;
  test_1_type.elements = test_1_elements;

  CHECK (sffi_get_struct_offsets (SFFI_DEFAULT_ABI, &test_1_type, test_1_offsets)
	 == SFFI_OK);
  CHECK (test_1_type.size == sizeof (struct test_1));
  CHECK (offsetof (struct test_1, c) == test_1_offsets[0]);
  CHECK (offsetof (struct test_1, f) == test_1_offsets[1]);
  CHECK (offsetof (struct test_1, c2) == test_1_offsets[2]);
  CHECK (offsetof (struct test_1, i) == test_1_offsets[3]);

  return 0;
}
