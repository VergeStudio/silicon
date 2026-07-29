/* Area:	closure_call
   Purpose:	Check return value uint.
   Limitations:	none.
   PR:		none.
   Originator:	<andreast@gcc.gnu.org> 20030828	 */

/* { dg-do run } */
#include "ffitest.h"

static void cls_ret_uint_fn(sffi_cif* cif __UNUSED__, void* resp, void** args,
			    void* userdata __UNUSED__)
{
  *(sffi_arg *)resp = *(unsigned int *)args[0];

  printf("%d: %d\n",*(unsigned int *)args[0],
	 (int)*(sffi_arg *)(resp));

  CHECK(*(unsigned int *)args[0] == 2147483647);
  CHECK((int)*(sffi_arg *)(resp) == 2147483647);
}
typedef unsigned int (*cls_ret_uint)(unsigned int);

int main (void)
{
  sffi_cif cif;
  void *code;
  sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
  sffi_type * cl_arg_types[2];
  unsigned int res;

  cl_arg_types[0] = &sffi_type_uint;
  cl_arg_types[1] = NULL;

  /* Initialize the cif */
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
		     &sffi_type_uint, cl_arg_types) == SFFI_OK);

  CHECK(sffi_prep_closure_loc(pcl, &cif, cls_ret_uint_fn, NULL, code)  == SFFI_OK);

  res = (*((cls_ret_uint)code))(2147483647);
  /* { dg-output "2147483647: 2147483647" } */
  printf("res: %d\n",res);
  /* { dg-output "\nres: 2147483647" } */
  CHECK(res == 2147483647);

  exit(0);
}
