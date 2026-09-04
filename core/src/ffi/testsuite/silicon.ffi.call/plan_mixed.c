#include "ffitest.h"

static double mixed(int a, double b, long c, float d,
		    long long e, double f)
{
  return (double) a + b + (double) c + (double) d + (double) e + f;
}

static float faddf(float a, float b)
{
  return a + b;
}

static signed char ret_sc(signed char x)
{
  return (signed char) (x + 1);
}

static unsigned char ret_uc(unsigned char x)
{
  return (unsigned char) (x + 1);
}

int main (void)
{

  {
    sffi_cif cif;
    sffi_type *args[6];
    void *values[6];
    sffi_call_plan *plan;
    int a = 3;
    double b = 1.5, f = -2.25, rc, rp;
    long c = 7;
    float d = 0.5f;
    long long e = -11;

    args[0] = &sffi_type_sint;
    args[1] = &sffi_type_double;
    args[2] = &sffi_type_slong;
    args[3] = &sffi_type_float;
    args[4] = &sffi_type_sint64;
    args[5] = &sffi_type_double;
    values[0] = &a; values[1] = &b; values[2] = &c;
    values[3] = &d; values[4] = &e; values[5] = &f;

    CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 6, &sffi_type_double, args)
	  == SFFI_OK);
    plan = sffi_call_plan_alloc(&cif);
    CHECK(plan != NULL);

    sffi_call(&cif, SFFI_FN(mixed), &rc, values);
    sffi_call_plan_invoke(plan, SFFI_FN(mixed), &rp, values);
    CHECK_DOUBLE_EQ(rc, rp);
    CHECK_DOUBLE_EQ(rp, mixed(a, b, c, d, e, f));
    sffi_call_plan_free(plan);
  }

  {
    sffi_cif cif;
    sffi_type *args[2];
    void *values[2];
    sffi_call_plan *plan;
    float a = 1.25f, b = 2.5f, rc, rp;

    args[0] = &sffi_type_float;
    args[1] = &sffi_type_float;
    values[0] = &a;
    values[1] = &b;
    CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 2, &sffi_type_float, args)
	  == SFFI_OK);
    plan = sffi_call_plan_alloc(&cif);
    CHECK(plan != NULL);

    sffi_call(&cif, SFFI_FN(faddf), &rc, values);
    sffi_call_plan_invoke(plan, SFFI_FN(faddf), &rp, values);
    CHECK_FLOAT_EQ(rc, rp);
    CHECK_FLOAT_EQ(rp, faddf(a, b));
    sffi_call_plan_free(plan);
  }

  {
    sffi_cif cif;
    sffi_type *args[1];
    void *values[1];
    sffi_call_plan *plan;
    signed char in;
    sffi_arg rc, rp;
    int v;

    args[0] = &sffi_type_schar;
    CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1, &sffi_type_schar, args)
	  == SFFI_OK);
    plan = sffi_call_plan_alloc(&cif);
    CHECK(plan != NULL);

    for (v = -128; v < 128; v++)
      {
	in = (signed char) v;
	values[0] = &in;
	sffi_call(&cif, SFFI_FN(ret_sc), &rc, values);
	sffi_call_plan_invoke(plan, SFFI_FN(ret_sc), &rp, values);
	CHECK(rc == rp);
	CHECK((signed char) rp == ret_sc(in));
      }
    sffi_call_plan_free(plan);
  }

  {
    sffi_cif cif;
    sffi_type *args[1];
    void *values[1];
    sffi_call_plan *plan;
    unsigned char in;
    sffi_arg rc, rp;
    int v;

    args[0] = &sffi_type_uchar;
    CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1, &sffi_type_uchar, args)
	  == SFFI_OK);
    plan = sffi_call_plan_alloc(&cif);
    CHECK(plan != NULL);

    for (v = 0; v < 256; v++)
      {
	in = (unsigned char) v;
	values[0] = &in;
	sffi_call(&cif, SFFI_FN(ret_uc), &rc, values);
	sffi_call_plan_invoke(plan, SFFI_FN(ret_uc), &rp, values);
	CHECK(rc == rp);
	CHECK((unsigned char) rp == ret_uc(in));
      }
    sffi_call_plan_free(plan);
  }

  exit(0);
}
