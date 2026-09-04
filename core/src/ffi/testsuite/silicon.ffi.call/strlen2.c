



#include "ffitest.h"

static int ABI_ATTR my_f(char *s, float a)
{
  return (int) strlen(s) + (int) a;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  sffi_arg rint;
  char *s;
  float v2;
  args[0] = &sffi_type_pointer;
  args[1] = &sffi_type_float;
  values[0] = (void*) &s;
  values[1] = (void*) &v2;
  
  
  CHECK(sffi_prep_cif(&cif, ABI_NUM, 2,
		       &sffi_type_sint, args) == SFFI_OK);
  
  s = "a";
  v2 = 0.0;
  sffi_call(&cif, SFFI_FN(my_f), &rint, values);
  CHECK(rint == 1);
  
  s = "1234567";
  v2 = -1.0;
  sffi_call(&cif, SFFI_FN(my_f), &rint, values);
  CHECK(rint == 6);
  
  s = "1234567890123456789012345";
  v2 = 1.0;
  sffi_call(&cif, SFFI_FN(my_f), &rint, values);
  CHECK(rint == 26);
  
  exit(0);
}
