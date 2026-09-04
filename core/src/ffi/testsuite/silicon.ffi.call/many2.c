#include "ffitest.h"

#define NARGS 7

typedef unsigned char u8;

#ifdef __GNUC__
__attribute__((noinline))
#endif
uint8_t
foo (uint8_t a, uint8_t b, uint8_t c, uint8_t d,
     uint8_t e, uint8_t f, uint8_t g)
{
  return a + b + c + d + e + f + g;
}

uint8_t ABI_ATTR
bar (uint8_t a, uint8_t b, uint8_t c, uint8_t d,
     uint8_t e, uint8_t f, uint8_t g)
{
  return foo (a, b, c, d, e, f, g);
}

int
main (void)
{
  sffi_type *ffitypes[NARGS];
  int i;
  sffi_cif cif;
  sffi_arg result = 0;
  uint8_t args[NARGS];
  void *argptrs[NARGS];

  for (i = 0; i < NARGS; ++i)
    ffitypes[i] = &sffi_type_uint8;

  CHECK (sffi_prep_cif (&cif, ABI_NUM, NARGS,
		       &sffi_type_uint8, ffitypes) == SFFI_OK);

  for (i = 0; i < NARGS; ++i)
    {
      args[i] = i;
      argptrs[i] = &args[i];
    }
  sffi_call (&cif, SFFI_FN (bar), &result, argptrs);

  CHECK (result == 21);
  return 0;
}
