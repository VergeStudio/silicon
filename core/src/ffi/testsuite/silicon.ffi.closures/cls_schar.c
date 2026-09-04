




#include "ffitest.h"

static void cls_ret_schar_fn(sffi_cif* cif __UNUSED__, void* resp, void** args,
			     void* userdata __UNUSED__)
{
  *(sffi_arg*)resp = *(signed char *)args[0];
  printf("%d: %d\n",*(signed char *)args[0],
	 (int)*(sffi_arg *)(resp));
  CHECK(*(signed char *)args[0] == 127);
  CHECK((int)*(sffi_arg *)(resp) == 127);
}
typedef signed char (*cls_ret_schar)(signed char);

int main (void)
{
  sffi_cif cif;
  void *code;
  sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
  sffi_type * cl_arg_types[2];
  signed char res;

  cl_arg_types[0] = &sffi_type_schar;
  cl_arg_types[1] = NULL;

  
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
		     &sffi_type_schar, cl_arg_types) == SFFI_OK);

  CHECK(sffi_prep_closure_loc(pcl, &cif, cls_ret_schar_fn, NULL, code)  == SFFI_OK);

  res = (*((cls_ret_schar)code))(127);
  
  printf("res: %d\n", res);
  
  CHECK(res == 127);

  exit(0);
}
