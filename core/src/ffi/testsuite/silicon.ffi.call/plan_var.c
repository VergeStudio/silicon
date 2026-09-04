


#include <stdarg.h>
#include "ffitest.h"

static double vsum(int n, ...)
{
  va_list ap;
  double s = 0.0;
  int i;

  va_start(ap, n);
  for (i = 0; i < n; i++)
    s += va_arg(ap, double);
  va_end(ap);
  return s;
}

static void
run (int nvar)
{
  sffi_cif cif;
  sffi_type *args[1 + 8];
  void *values[1 + 8];
  double d[8];
  int n = nvar;
  double rc, rp, expect;
  sffi_call_plan *plan;
  int i;

  CHECK(nvar <= 8);

  args[0] = &sffi_type_sint;
  values[0] = &n;
  expect = 0.0;
  for (i = 0; i < nvar; i++)
    {
      d[i] = (double) (i + 1) * 1.5;
      args[1 + i] = &sffi_type_double;
      values[1 + i] = &d[i];
      expect += d[i];
    }

  CHECK(sffi_prep_cif_var(&cif, SFFI_DEFAULT_ABI, 1, 1 + nvar,
			 &sffi_type_double, args) == SFFI_OK);
  plan = sffi_call_plan_alloc(&cif);
  CHECK(plan != NULL);

  sffi_call(&cif, SFFI_FN(vsum), &rc, values);
  sffi_call_plan_invoke(plan, SFFI_FN(vsum), &rp, values);
  CHECK_DOUBLE_EQ(rc, rp);
  CHECK_DOUBLE_EQ(rp, expect);

  sffi_call_plan_free(plan);
}

int main (void)
{
  run (0);
  run (1);
  run (3);
  run (5);
  run (8);
  exit(0);
}
