/* Area:	closure_call
   Purpose:	Check return value sint32.
   Limitations:	none.
   PR:		none.
   Originator:	<andreast@gcc.gnu.org> 20031108	 */

/* { dg-do run } */
#include "ffitest.h"

static void cls_ret_sint_fn(sffi_cif* cif __UNUSED__, void* resp, void** args,
			    void* userdata __UNUSED__)
{
  *(sffi_arg*)resp = *(signed int *)args[0];
  printf("%d: %d\n",*(signed int *)args[0],
	 (int)*(sffi_arg *)(resp));
  CHECK(*(signed int *)args[0] == 65534);
  CHECK((int)*(sffi_arg *)(resp) == 65534);
}
typedef signed int (*cls_ret_sint)(signed int);

int main (void)
{
  sffi_cif cif;
  void *code;
  sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
  sffi_type * cl_arg_types[2];
  signed int res;

  cl_arg_types[0] = &sffi_type_sint;
  cl_arg_types[1] = NULL;

  /* Initialize the cif */
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
		     &sffi_type_sint, cl_arg_types) == SFFI_OK);

  CHECK(sffi_prep_closure_loc(pcl, &cif, cls_ret_sint_fn, NULL, code)  == SFFI_OK);

  res = (*((cls_ret_sint)code))(65534);
  /* { dg-output "65534: 65534" } */
  printf("res: %d\n",res);
  /* { dg-output "\nres: 65534" } */

  exit(0);
}
