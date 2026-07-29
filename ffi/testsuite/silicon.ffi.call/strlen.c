/* Area:	sffi_call
   Purpose:	Check strlen function call.
   Limitations:	none.
   PR:		none.
   Originator:	From the original ffitest.c  */

/* { dg-do run } */
#include "ffitest.h"

static unsigned int ABI_ATTR my_strlen(char *s)
{
  return (unsigned int) (strlen(s));
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  sffi_arg rint;
  char *s;

  args[0] = &sffi_type_pointer;
  values[0] = (void*) &s;

  /* Initialize the cif */
  CHECK(sffi_prep_cif(&cif, ABI_NUM, 1,
		     &sffi_type_uint, args) == SFFI_OK);

  s = "a";
  sffi_call(&cif, SFFI_FN(my_strlen), &rint, values);
  CHECK(rint == 1);

  s = "1234567";
  sffi_call(&cif, SFFI_FN(my_strlen), &rint, values);
  CHECK(rint == 7);

  s = "1234567890123456789012345";
  sffi_call(&cif, SFFI_FN(my_strlen), &rint, values);
  CHECK(rint == 25);

  exit (0);
}
