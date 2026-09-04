#include "ffitest.h"

static void closure_test_fn2(sffi_cif* cif __UNUSED__, void* resp, void** args,
			     void* userdata)
{
  *(sffi_arg*)resp =
    (int)*(double *)args[0] +(int)(*(double *)args[1]) +
    (int)(*(double *)args[2]) + (int)*(double *)args[3] +
    (int)(*(signed short *)args[4]) + (int)(*(double *)args[5]) +
    (int)*(double *)args[6] + (int)(*(int *)args[7]) +
    (int)(*(double *)args[8]) + (int)*(int *)args[9] +
    (int)(*(int *)args[10]) + (int)(*(float *)args[11]) +
    (int)*(int *)args[12] + (int)(*(float *)args[13]) +
    (int)(*(int *)args[14]) + *(int *)args[15] + (intptr_t)userdata;

  printf("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d: %d\n",
	 (int)*(double *)args[0], (int)(*(double *)args[1]),
	 (int)(*(double *)args[2]), (int)*(double *)args[3],
	 (int)(*(signed short *)args[4]), (int)(*(double *)args[5]),
	 (int)*(double *)args[6], (int)(*(int *)args[7]),
	 (int)(*(double*)args[8]), (int)*(int *)args[9],
	 (int)(*(int *)args[10]), (int)(*(float *)args[11]),
	 (int)*(int *)args[12], (int)(*(float *)args[13]),
	 (int)(*(int *)args[14]), *(int *)args[15], (int)(intptr_t)userdata,
	 (int)*(sffi_arg *)resp);
  CHECK((int)*(sffi_arg *)resp == 255);
}

typedef int (*closure_test_type2)(double, double, double, double, signed short,
				  double, double, int, double, int, int, float,
				  int, float, int, int);

int main (void)
{
  sffi_cif cif;
  void *code;
  sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
  sffi_type * cl_arg_types[17];
  int res;

  cl_arg_types[0] = &sffi_type_double;
  cl_arg_types[1] = &sffi_type_double;
  cl_arg_types[2] = &sffi_type_double;
  cl_arg_types[3] = &sffi_type_double;
  cl_arg_types[4] = &sffi_type_sshort;
  cl_arg_types[5] = &sffi_type_double;
  cl_arg_types[6] = &sffi_type_double;
  cl_arg_types[7] = &sffi_type_sint;
  cl_arg_types[8] = &sffi_type_double;
  cl_arg_types[9] = &sffi_type_sint;
  cl_arg_types[10] = &sffi_type_sint;
  cl_arg_types[11] = &sffi_type_float;
  cl_arg_types[12] = &sffi_type_sint;
  cl_arg_types[13] = &sffi_type_float;
  cl_arg_types[14] = &sffi_type_sint;
  cl_arg_types[15] = &sffi_type_sint;
  cl_arg_types[16] = NULL;

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 16,
		     &sffi_type_sint, cl_arg_types) == SFFI_OK);

  CHECK(sffi_prep_closure_loc(pcl, &cif, closure_test_fn2,
                             (void *) 3 , code) == SFFI_OK);

  res = (*((closure_test_type2)code))
    (1, 2, 3, 4, 127, 5, 6, 8, 9, 10, 11, 12.0, 13,
     19.0, 21, 1);

  printf("res: %d\n",res);

  CHECK(res == 255);
  exit(0);
}
