



#include "ffitest.h"

static int ABI_ATTR my_f(float a, char *s, int i)
{
  return (int) strlen(s) + (int) a + i;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  sffi_arg rint;
  char *s;
  int v1;
  float v2;
  args[2] = &sffi_type_sint;
  args[1] = &sffi_type_pointer;
  args[0] = &sffi_type_float;
  values[2] = (void*) &v1;
  values[1] = (void*) &s;
  values[0] = (void*) &v2;
  
  
  CHECK(sffi_prep_cif(&cif, ABI_NUM, 3,
		       &sffi_type_sint, args) == SFFI_OK);
  
  s = "a";
  v1 = 1;
  v2 = 0.0;
  sffi_call(&cif, SFFI_FN(my_f), &rint, values);
  CHECK(rint == 2);
  
  s = "1234567";
  v2 = -1.0;
  v1 = -2;
  sffi_call(&cif, SFFI_FN(my_f), &rint, values);
  CHECK(rint == 4);
  
  s = "1234567890123456789012345";
  v2 = 1.0;
  v1 = 2;
  sffi_call(&cif, SFFI_FN(my_f), &rint, values);
  CHECK(rint == 28);
  
  exit(0);
}
