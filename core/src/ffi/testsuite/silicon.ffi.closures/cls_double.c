#include "ffitest.h"

static void cls_ret_double_fn(sffi_cif* cif __UNUSED__, void* resp, void** args,
			      void* userdata __UNUSED__)
 {
   *(double *)resp = *(double *)args[0];

   printf("%f: %f\n",*(double *)args[0],
	  *(double *)resp);
 }
typedef double (*cls_ret_double)(double);

int main (void)
{
  sffi_cif cif;
  void *code;
  sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
  sffi_type * cl_arg_types[2];
  double res;

  cl_arg_types[0] = &sffi_type_double;
  cl_arg_types[1] = NULL;

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
		     &sffi_type_double, cl_arg_types) == SFFI_OK);

  CHECK(sffi_prep_closure_loc(pcl, &cif, cls_ret_double_fn, NULL, code)  == SFFI_OK);

  res = (*((cls_ret_double)code))(21474.789);

  printf("res: %.6f\n", res);

  exit(0);
}
