/* Area:	sffi_call
   Purpose:	Test longjmp over sffi_call frames */

/* Test code adapted from Lars Kanis' bug report:
   https://github.com/SILICON_FFI/SILICON_FFI/issues/905 */

/* { dg-do run } */

#include "ffitest.h"
#include "sffi_common.h"

#include <setjmp.h>

static jmp_buf buf;

static void ABI_ATTR lev2(const char *str) {
  printf("lev2 %s\n", str);
  // jumps back to where setjmp was called - making setjmp now return 1
  longjmp(buf, 1);
}

static void ABI_ATTR lev1(const char *str) {
  lev2(str);

  // will not be reached
  printf("lev1 %s\n", str);
}

int main()
{
  sffi_cif cif;
  sffi_type *args[1];
  void *values[1];
  char *s;
  sffi_arg rc;
  /* Initialize the argument info vectors */
  args[0] = &sffi_type_pointer;
  values[0] = &s;
  /* Initialize the cif */
  if (sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
    &sffi_type_sint, args) == SFFI_OK)
  {
    s = "direct call";
    if (!setjmp(buf)){
      // works on x64 and arm64
      lev1(s);
    } else {
      printf("back to main\n");
    }

    s = "through SILICON_FFI";
    if (!setjmp(buf)){
      // works on x64 but segfaults on arm64
      sffi_call(&cif, (void (*)(void))lev1, &rc, values);
    } else {
      printf("back to main\n");
    }
  }
  return 0;
}
