





#include "ffitest.h"
#include "sffi_common.h"

#include <setjmp.h>

static jmp_buf buf;

static void ABI_ATTR lev2(const char *str) {
  printf("lev2 %s\n", str);

  longjmp(buf, 1);
}

static void ABI_ATTR lev1(const char *str) {
  lev2(str);


  printf("lev1 %s\n", str);
}

int main()
{
  sffi_cif cif;
  sffi_type *args[1];
  void *values[1];
  char *s;
  sffi_arg rc;
  
  args[0] = &sffi_type_pointer;
  values[0] = &s;
  
  if (sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
    &sffi_type_sint, args) == SFFI_OK)
  {
    s = "direct call";
    if (!setjmp(buf)){

      lev1(s);
    } else {
      printf("back to main\n");
    }

    s = "through SILICON_FFI";
    if (!setjmp(buf)){

      sffi_call(&cif, (void (*)(void))lev1, &rc, values);
    } else {
      printf("back to main\n");
    }
  }
  return 0;
}
