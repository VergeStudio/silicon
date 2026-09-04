



#include "ffitest.h"

static void cls_ret_ulonglong_fn(sffi_cif* cif __UNUSED__, void* resp,
				 void** args, void* userdata __UNUSED__)
{
  *(unsigned long long *)resp= 0xfffffffffffffffLL ^ *(unsigned long long *)args[0];

  printf("%" PRIuLL ": %" PRIuLL "\n",*(unsigned long long *)args[0],
	 *(unsigned long long *)(resp));
}
typedef unsigned long long (*cls_ret_ulonglong)(unsigned long long);

int main (void)
{
  sffi_cif cif;
  void *code;
  sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
  sffi_type * cl_arg_types[2];
  unsigned long long res;

  cl_arg_types[0] = &sffi_type_uint64;
  cl_arg_types[1] = NULL;

  
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
		     &sffi_type_uint64, cl_arg_types) == SFFI_OK);
  CHECK(sffi_prep_closure_loc(pcl, &cif, cls_ret_ulonglong_fn, NULL, code)  == SFFI_OK);
  res = (*((cls_ret_ulonglong)code))(214LL);
  
  printf("res: %" PRIdLL "\n", res);
  
  CHECK(res == 1152921504606846761LL);

  res = (*((cls_ret_ulonglong)code))(9223372035854775808LL);
  
  printf("res: %" PRIdLL "\n", res);
  
  CHECK(res == 8070450533247928831LL);

  exit(0);
}
