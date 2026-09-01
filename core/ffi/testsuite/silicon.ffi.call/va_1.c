/* Area:		sffi_call
   Purpose:		Test passing struct in variable argument lists.
   Limitations:	none.
   PR:			none.
   Originator:	        ARM Ltd. */

/* { dg-do run } */

#include "ffitest.h"
#include <stdarg.h>

struct small_tag
{
  unsigned char a;
  unsigned char b;
};

struct large_tag
{
  unsigned a;
  unsigned b;
  unsigned c;
  unsigned d;
  unsigned e;
};

int
main (void)
{
  sffi_cif cif;
  sffi_type* arg_types[15];

  sffi_type s_type;
  sffi_type *s_type_elements[3];

  sffi_type l_type;
  sffi_type *l_type_elements[6];

  s_type.size = 0;
  s_type.alignment = 0;
  s_type.type = SFFI_TYPE_STRUCT;
  s_type.elements = s_type_elements;

  s_type_elements[0] = &sffi_type_uchar;
  s_type_elements[1] = &sffi_type_uchar;
  s_type_elements[2] = NULL;

  l_type.size = 0;
  l_type.alignment = 0;
  l_type.type = SFFI_TYPE_STRUCT;
  l_type.elements = l_type_elements;

  l_type_elements[0] = &sffi_type_uint;
  l_type_elements[1] = &sffi_type_uint;
  l_type_elements[2] = &sffi_type_uint;
  l_type_elements[3] = &sffi_type_uint;
  l_type_elements[4] = &sffi_type_uint;
  l_type_elements[5] = NULL;

  arg_types[0] = &sffi_type_sint;
  arg_types[1] = &s_type;
  arg_types[2] = &l_type;
  arg_types[3] = &s_type;
  arg_types[4] = &sffi_type_uchar;
  arg_types[5] = &sffi_type_schar;
  arg_types[6] = &sffi_type_ushort;
  arg_types[7] = &sffi_type_sshort;
  arg_types[8] = &sffi_type_uint;
  arg_types[9] = &sffi_type_sint;
  arg_types[10] = &sffi_type_ulong;
  arg_types[11] = &sffi_type_slong;
  arg_types[12] = &sffi_type_double;
  arg_types[13] = &sffi_type_double;
  arg_types[14] = NULL;

  CHECK(sffi_prep_cif_var(&cif, SFFI_DEFAULT_ABI, 1, 14, &sffi_type_sint, arg_types) == SFFI_BAD_ARGTYPE);
  return 0;
}
