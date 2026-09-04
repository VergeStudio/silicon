#include "ffitest.h"

static void cls_ret_float_fn(sffi_cif* cif __UNUSED__, void* resp, void** args,
			     void* userdata __UNUSED__)
 {
   *(float *)resp = *(float *)args[0];

   printf("%g: %g\n",*(float *)args[0],
	  *(float *)resp);

   CHECK((int)(*(float *)args[0]) == -2122);
   CHECK((int)(*(float *)resp) == -2122);
 }

typedef float (*cls_ret_float)(float);

int main (void)
{
  sffi_cif cif;
  void *code;
  sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
  sffi_type * cl_arg_types[2];
  float res;

  cl_arg_types[0] = &sffi_type_float;
  cl_arg_types[1] = NULL;

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
		     &sffi_type_float, cl_arg_types) == SFFI_OK);

  CHECK(sffi_prep_closure_loc(pcl, &cif, cls_ret_float_fn, NULL, code)  == SFFI_OK);
  res = ((((cls_ret_float)code)(-2122.12)));

  printf("res: %.6f\n", res);

  CHECK((int)res == -2122);
  exit(0);
}
