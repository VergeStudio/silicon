/* Area:	sffi_call, closure_call
   Purpose:	Check passing of multiple signed char values.
   Limitations:	none.
   PR:		PR13221.
   Originator:	<hos@tamanegi.org> 20031129  */

/* { dg-do run } */
#include "ffitest.h"

static signed char test_func_fn(signed char a1, signed char a2)
{
  signed char result;

  result = a1 + a2;

  printf("%d %d: %d\n", a1, a2, result);

  return result;

}

static void test_func_gn(sffi_cif *cif __UNUSED__, void *rval, void **avals,
			 void *data __UNUSED__)
{
  signed char a1, a2;

  a1 = *(signed char *)avals[0];
  a2 = *(signed char *)avals[1];
  CHECK(a1 == 2);
  CHECK(a2 == 125);

  *(sffi_arg *)rval = test_func_fn(a1, a2);

}

typedef signed char (*test_type)(signed char, signed char);

int main (void)
{
  sffi_cif cif;
  void *code;
  sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
  void * args_dbl[3];
  sffi_type * cl_arg_types[3];
  sffi_arg res_call;
  signed char a, b, res_closure;

  a = 2;
  b = 125;

  args_dbl[0] = &a;
  args_dbl[1] = &b;
  args_dbl[2] = NULL;

  cl_arg_types[0] = &sffi_type_schar;
  cl_arg_types[1] = &sffi_type_schar;
  cl_arg_types[2] = NULL;

  /* Initialize the cif */
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 2,
		     &sffi_type_schar, cl_arg_types) == SFFI_OK);

  sffi_call(&cif, SFFI_FN(test_func_fn), &res_call, args_dbl);
  /* { dg-output "2 125: 127" } */
  printf("res: %d\n", (signed char)res_call);
  /* { dg-output "\nres: 127" } */
  CHECK((signed char)res_call == 127);

  CHECK(sffi_prep_closure_loc(pcl, &cif, test_func_gn, NULL, code)  == SFFI_OK);

  res_closure = (*((test_type)code))(2, 125);
  /* { dg-output "\n2 125: 127" } */
  printf("res: %d\n", res_closure);
  /* { dg-output "\nres: 127" } */
  CHECK(res_closure == 127);

  exit(0);
}
