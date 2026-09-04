


#include "ffitest.h"

static int call_count = 0;


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


static double sum_big3(struct big3 s)
{
  return s.a + s.b + s.c;
}


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

    
    before = call_count;
    sffi_call_plan_invoke(plan, SFFI_FN(make_big3), NULL, values);
    CHECK(call_count == before + 1);

    sffi_call_plan_free(plan);
  }

  
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
