#include "ffitest.h"

typedef struct
{
  float f;
} s55;

static s55 ABI_ATTR f55(s55 ts, float f)
{
  s55 r;
  r.f = ts.f + f;
  printf ("f55>> %g + %g = %g\n", ts.f, f, r.f);
  return r;
}

int main (void)
{
  sffi_cif cif;
  s55 F, Fr;
  float f;
  void *values[] = { &F, &f };
  sffi_type s55_type;
  sffi_type *args[] = { &s55_type, &sffi_type_float };
  sffi_type *s55_type_elements[] = { &sffi_type_float, NULL };

  s55 *s55_result =
    (s55 *) malloc (sizeof(s55));

  s55_type.size = 0;
  s55_type.alignment = 0;
  s55_type.type = SFFI_TYPE_STRUCT;
  s55_type.elements = s55_type_elements;

  CHECK(sffi_prep_cif(&cif, ABI_NUM, 2, &s55_type, args) == SFFI_OK);

  F.f = 1;
  Fr = f55(F, 2.14);
  printf ("%g\n", Fr.f);

  F.f = 1;
  f = 2.14;
  sffi_call(&cif, SFFI_FN(f55), s55_result, values);
  printf ("%g\n", s55_result->f);

  fflush(0);

  CHECK(fabs(Fr.f - s55_result->f) < FLT_EPSILON);

  free (s55_result);
  exit(0);
}
