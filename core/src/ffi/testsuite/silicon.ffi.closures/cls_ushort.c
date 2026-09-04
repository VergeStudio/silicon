


#include "ffitest.h"

static void cls_ret_ushort_fn(sffi_cif* cif __UNUSED__, void* resp, void** args,
			      void* userdata __UNUSED__)
{
  *(sffi_arg*)resp = *(unsigned short *)args[0];

  printf("%d: %d\n",*(unsigned short *)args[0],
	 (int)*(sffi_arg *)(resp));
  CHECK(*(unsigned short *)args[0] == 65535);
  CHECK((int)*(sffi_arg *)(resp) == 65535);
}
typedef unsigned short (*cls_ret_ushort)(unsigned short);

int main (void)
{
  sffi_cif cif;
  void *code;
  sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
  sffi_type * cl_arg_types[2];
  unsigned short res;

  cl_arg_types[0] = &sffi_type_ushort;
  cl_arg_types[1] = NULL;

  
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
		     &sffi_type_ushort, cl_arg_types) == SFFI_OK);

  CHECK(sffi_prep_closure_loc(pcl, &cif, cls_ret_ushort_fn, NULL, code)  == SFFI_OK);

  res = (*((cls_ret_ushort)code))(65535);
  
  printf("res: %d\n",res);
  
  CHECK(res == 65535);

  exit(0);
}
