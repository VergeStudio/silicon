#include "ffitest.h"

struct s {
  int s32;
  float f32;
  signed char s8;
};

struct s ABI_ATTR make_s(void) {
  struct s r;
  r.s32 = 0x1234;
  r.f32 = 7.0;
  r.s8  = 0x78;
  return r;
}

int main() {
  sffi_cif cif;
  struct s r;
  sffi_type rtype;
  sffi_type* s_fields[] = {
    &sffi_type_sint,
    &sffi_type_float,
    &sffi_type_schar,
    NULL,
  };

  rtype.size      = 0;
  rtype.alignment = 0,
  rtype.type      = SFFI_TYPE_STRUCT,
  rtype.elements  = s_fields,

  r.s32 = 0xbad;
  r.f32 = 999.999;
  r.s8  = 0x51;

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 0, &rtype, NULL) == SFFI_OK);
  sffi_call(&cif, SFFI_FN(make_s), &r, NULL);

  CHECK(r.s32 == 0x1234);
  CHECK(r.f32 == 7.0);
  CHECK(r.s8  == 0x78);
  exit(0);
}
