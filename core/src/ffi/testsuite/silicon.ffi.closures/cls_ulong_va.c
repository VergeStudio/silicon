#include "ffitest.h"

typedef unsigned long T;

static void cls_ret_T_fn(sffi_cif* cif __UNUSED__, void* resp, void** args,
			 void* userdata __UNUSED__)
 {
   *(T *)resp = *(T *)args[0];

   printf("%ld: %ld %ld\n", *(T *)resp, *(T *)args[0], *(T *)args[1]);
   CHECK(*(T *)args[0] == 67);
   CHECK(*(T *)args[1] == 4);
   CHECK(*(T *)resp == 67);
 }

typedef T (*cls_ret_T)(T, ...);

int main (void)
{
  sffi_cif cif;
  void *code;
  sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
  sffi_type * cl_arg_types[3];
  T res;

  cl_arg_types[0] = &sffi_type_ulong;
  cl_arg_types[1] = &sffi_type_ulong;
  cl_arg_types[2] = NULL;

  CHECK(sffi_prep_cif_var(&cif, SFFI_DEFAULT_ABI, 1, 2,
			 &sffi_type_ulong, cl_arg_types) == SFFI_OK);

  CHECK(sffi_prep_closure_loc(pcl, &cif, cls_ret_T_fn, NULL, code)  == SFFI_OK);
  res = ((((cls_ret_T)code)(67, 4)));

  printf("res: %ld\n", res);

  CHECK(res == 67);
  exit(0);
}
