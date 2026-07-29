/* Area:	sffi_call
   Purpose:	Check return value double.
   Limitations:	none.
   PR:		none.
   Originator:	From the original ffitest.c  */

/* { dg-do run } */
#include "ffitest.h"
#include "float.h"

#include <math.h>

typedef union
{
  double d;
  unsigned char c[sizeof (double)];
} value_type;

#define CANARY 0xba

static double dblit(float f)
{
  return f/3.0;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  float f;
  value_type result[2];
  unsigned int i;

  args[0] = &sffi_type_float;
  values[0] = &f;

  /* Initialize the cif */
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
		     &sffi_type_double, args) == SFFI_OK);

  f = 3.14159;

  /* Put a canary in the return array.  This is a regression test for
     a buffer overrun.  */
  memset(result[1].c, CANARY, sizeof (double));

  sffi_call(&cif, SFFI_FN(dblit), &result[0].d, values);

  /* These are not always the same!! Check for a reasonable delta */

  CHECK(fabs(result[0].d - dblit(f)) < DBL_EPSILON);

  /* Check the canary.  */
  for (i = 0; i < sizeof (double); ++i)
    CHECK(result[1].c[i] == CANARY);

  exit(0);

}
