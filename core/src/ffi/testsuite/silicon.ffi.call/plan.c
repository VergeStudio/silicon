/* Area:	sffi_call_plan
   Purpose:	Check that a reusable call plan reproduces sffi_call for the
		pure-GP64 fast path, pointer arguments and repeated reuse,
		and that a signature with no fast path still yields a usable
		plan that falls back to sffi_call.
   Limitations:	none.
   PR:		none.
   Originator:	sffi_call_plan tests  */

/* { dg-do run } */
#include "ffitest.h"

static uint64_t gp6(uint64_t a, uint64_t b, uint64_t c,
		    uint64_t d, uint64_t e, uint64_t f)
{
  return a + b * 2 + c * 3 + d * 4 + e * 5 + f * 6;
}

static void *ptr_ident(void *p)
{
  return p;
}

struct small_pair { long x; long y; };

static long ssum(struct small_pair s)
{
  return s.x - s.y;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[6];
  void *values[6];
  sffi_call_plan *plan;
  uint64_t a[6], r_call, r_plan;
  int i, k;

  /* Pure GP64: every argument is one 64-bit integer, so build_plan
     selects the sffi_plan_gpN direct thunk.  Reuse the plan across many
     invocations with changing values.  */
  for (i = 0; i < 6; i++)
    args[i] = &sffi_type_uint64;
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 6, &sffi_type_uint64, args)
	== SFFI_OK);
  plan = sffi_call_plan_alloc(&cif);
  CHECK(plan != NULL);

  for (k = 0; k < 100; k++)
    {
      for (i = 0; i < 6; i++)
	{
	  a[i] = (uint64_t) (k * 7 + i + 1);
	  values[i] = &a[i];
	}
      sffi_call(&cif, SFFI_FN(gp6), &r_call, values);
      sffi_call_plan_invoke(plan, SFFI_FN(gp6), &r_plan, values);
      CHECK(r_call == r_plan);
      CHECK(r_plan == gp6(a[0], a[1], a[2], a[3], a[4], a[5]));
    }
  sffi_call_plan_free(plan);

  /* Pointer argument and pointer return. */
  {
    sffi_cif cifp;
    sffi_type *pargs[1];
    void *pvalues[1];
    void *in, *rc, *rp;
    sffi_call_plan *planp;

    pargs[0] = &sffi_type_pointer;
    CHECK(sffi_prep_cif(&cifp, SFFI_DEFAULT_ABI, 1, &sffi_type_pointer, pargs)
	  == SFFI_OK);
    planp = sffi_call_plan_alloc(&cifp);
    CHECK(planp != NULL);

    in = &cifp;
    pvalues[0] = &in;
    sffi_call(&cifp, SFFI_FN(ptr_ident), &rc, pvalues);
    sffi_call_plan_invoke(planp, SFFI_FN(ptr_ident), &rp, pvalues);
    CHECK(rc == in);
    CHECK(rp == in);
    sffi_call_plan_free(planp);
  }

  /* No fast path: a struct-by-value argument.  build_plan returns NULL for
     the fast plan, but sffi_call_plan_alloc must still hand back a valid plan
     whose invoke falls back to sffi_call and produces the same result.  */
  {
    sffi_cif cifs;
    sffi_type *sargs[1];
    sffi_type stype;
    sffi_type *selements[3];
    void *svalues[1];
    struct small_pair s;
    sffi_arg rc, rp;
    sffi_call_plan *plans;

    selements[0] = &sffi_type_slong;
    selements[1] = &sffi_type_slong;
    selements[2] = NULL;
    stype.size = stype.alignment = 0;
    stype.type = SFFI_TYPE_STRUCT;
    stype.elements = selements;

    sargs[0] = &stype;
    CHECK(sffi_prep_cif(&cifs, SFFI_DEFAULT_ABI, 1, &sffi_type_slong, sargs)
	  == SFFI_OK);
    plans = sffi_call_plan_alloc(&cifs);
    CHECK(plans != NULL);

    s.x = 123;
    s.y = 45;
    svalues[0] = &s;
    sffi_call(&cifs, SFFI_FN(ssum), &rc, svalues);
    sffi_call_plan_invoke(plans, SFFI_FN(ssum), &rp, svalues);
    CHECK(rc == rp);
    CHECK((long) rp == ssum(s));
    sffi_call_plan_free(plans);
  }

  /* Freeing NULL is documented to be harmless. */
  sffi_call_plan_free(NULL);

  exit(0);
}
