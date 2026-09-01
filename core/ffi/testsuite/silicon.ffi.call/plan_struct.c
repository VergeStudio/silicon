/* Area:	sffi_call_plan
   Purpose:	Check that a reusable call plan reproduces sffi_call for struct
		returns.  A struct return does not disable planning (only a
		struct *argument* does), so this drives both the in-memory
		return path (RET_IN_MEM, including a NULL rvalue) and the
		register-pair struct return path, plus a large struct argument
		that forces the sffi_call by-value copy fallback.
   Limitations:	none.
   PR:		none.
   Originator:	sffi_call_plan tests  */

/* { dg-do run } */
#include "ffitest.h"

static int call_count = 0;

/* 24 bytes: returned in memory (a hidden pointer in the first argument). */
struct big3 { double a, b, c; };

static struct big3 make_big3(double a, double b, double c)
{
  struct big3 r;
  call_count++;
  r.a = a + 1.0;
  r.b = b + 2.0;
  r.c = c + 3.0;
  return r;
}

/* A struct larger than 16 bytes passed by value forces sffi_call to make a
   copy; build_plan has no fast path for a struct argument, so this exercises
   the plan's fallback to sffi_call. */
static double sum_big3(struct big3 s)
{
  return s.a + s.b + s.c;
}

/* 16 bytes: returned in a register pair (RAX:RDX on x86-64). */
struct pair2 { long x, y; };

static struct pair2 make_pair2(long x, long y)
{
  struct pair2 r;
  r.x = x * 2;
  r.y = y * 3;
  return r;
}

int main (void)
{
  sffi_type *big3_elements[4];
  sffi_type big3_t;
  sffi_type *pair2_elements[3];
  sffi_type pair2_t;

  big3_elements[0] = &sffi_type_double;
  big3_elements[1] = &sffi_type_double;
  big3_elements[2] = &sffi_type_double;
  big3_elements[3] = NULL;
  big3_t.size = big3_t.alignment = 0;
  big3_t.type = SFFI_TYPE_STRUCT;
  big3_t.elements = big3_elements;

  pair2_elements[0] = &sffi_type_slong;
  pair2_elements[1] = &sffi_type_slong;
  pair2_elements[2] = NULL;
  pair2_t.size = pair2_t.alignment = 0;
  pair2_t.type = SFFI_TYPE_STRUCT;
  pair2_t.elements = pair2_elements;

  /* In-memory struct return with scalar arguments. */
  {
    sffi_cif cif;
    sffi_type *args[3];
    void *values[3];
    sffi_call_plan *plan;
    double a = 10.0, b = 20.0, c = 30.0;
    struct big3 rc, rp;
    int before;

    args[0] = &sffi_type_double;
    args[1] = &sffi_type_double;
    args[2] = &sffi_type_double;
    values[0] = &a;
    values[1] = &b;
    values[2] = &c;
    CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 3, &big3_t, args) == SFFI_OK);
    plan = sffi_call_plan_alloc(&cif);
    CHECK(plan != NULL);

    sffi_call(&cif, SFFI_FN(make_big3), &rc, values);
    sffi_call_plan_invoke(plan, SFFI_FN(make_big3), &rp, values);
    CHECK_DOUBLE_EQ(rc.a, rp.a);
    CHECK_DOUBLE_EQ(rc.b, rp.b);
    CHECK_DOUBLE_EQ(rc.c, rp.c);
    CHECK_DOUBLE_EQ(rp.a, a + 1.0);
    CHECK_DOUBLE_EQ(rp.b, b + 2.0);
    CHECK_DOUBLE_EQ(rp.c, c + 3.0);

    /* A NULL rvalue for an in-memory struct return must not crash: SILICON_FFI
       supplies scratch space and discards the result, but the callee still
       runs.  Confirm the call actually happened. */
    before = call_count;
    sffi_call_plan_invoke(plan, SFFI_FN(make_big3), NULL, values);
    CHECK(call_count == before + 1);

    sffi_call_plan_free(plan);
  }

  /* Large struct argument: no fast path, falls back to sffi_call. */
  {
    sffi_cif cif;
    sffi_type *args[1];
    void *values[1];
    sffi_call_plan *plan;
    struct big3 s;
    double rc, rp;

    s.a = 1.5;
    s.b = 2.5;
    s.c = 3.5;
    args[0] = &big3_t;
    values[0] = &s;
    CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1, &sffi_type_double, args)
	  == SFFI_OK);
    plan = sffi_call_plan_alloc(&cif);
    CHECK(plan != NULL);

    sffi_call(&cif, SFFI_FN(sum_big3), &rc, values);
    sffi_call_plan_invoke(plan, SFFI_FN(sum_big3), &rp, values);
    CHECK_DOUBLE_EQ(rc, rp);
    CHECK_DOUBLE_EQ(rp, sum_big3(s));
    sffi_call_plan_free(plan);
  }

  /* Register-pair struct return. */
  {
    sffi_cif cif;
    sffi_type *args[2];
    void *values[2];
    sffi_call_plan *plan;
    long x = 7, y = 11;
    struct pair2 rc, rp;

    args[0] = &sffi_type_slong;
    args[1] = &sffi_type_slong;
    values[0] = &x;
    values[1] = &y;
    CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 2, &pair2_t, args) == SFFI_OK);
    plan = sffi_call_plan_alloc(&cif);
    CHECK(plan != NULL);

    sffi_call(&cif, SFFI_FN(make_pair2), &rc, values);
    sffi_call_plan_invoke(plan, SFFI_FN(make_pair2), &rp, values);
    CHECK(rc.x == rp.x);
    CHECK(rc.y == rp.y);
    CHECK(rp.x == x * 2);
    CHECK(rp.y == y * 3);
    sffi_call_plan_free(plan);
  }

  exit(0);
}
