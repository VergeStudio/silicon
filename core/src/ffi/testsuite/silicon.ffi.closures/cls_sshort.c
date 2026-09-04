#include "ffitest.h"

static void cls_ret_sshort_fn(sffi_cif* cif __UNUSED__, void* resp, void** args,
			      void* userdata __UNUSED__)
{
  *(sffi_arg*)resp = *(signed short *)args[0];
  printf("%d: %d\n",*(signed short *)args[0],
	 (int)*(sffi_arg *)(resp));
  CHECK(*(signed short *)args[0] == 255);
  CHECK((int)*(sffi_arg *)(resp) == 255);
}
typedef signed short (*cls_ret_sshort)(signed short);

int main (void)
{
  sffi_cif cif;
  void *code;
  sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
  sffi_type * cl_arg_types[2];
  signed short res;

  cl_arg_types[0] = &sffi_type_sshort;
  cl_arg_types[1] = NULL;

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
		     &sffi_type_sshort, cl_arg_types) == SFFI_OK);

  CHECK(sffi_prep_closure_loc(pcl, &cif, cls_ret_sshort_fn, NULL, code)  == SFFI_OK);

  res = (*((cls_ret_sshort)code))(255);

  printf("res: %d\n",res);

  CHECK(res == 255);

  exit(0);
}
