/* Area:	closure_call
   Purpose:	Check simple closure handling with all ABIs
   Limitations:	none.
   PR:		none.
   Originator:	<twalljava@dev.java.net> */

/* { dg-do run } */
#include "ffitest.h"

static void
closure_test(sffi_cif* cif __UNUSED__, void* resp, void** args, void* userdata)
{
  *(sffi_arg*)resp =
    (int)*(int *)args[0] + (int)(*(int *)args[1])
    + (int)(*(int *)args[2])  + (int)(*(int *)args[3])
    + (int)(intptr_t)userdata;

  printf("%d %d %d %d: %d\n",
	 (int)*(int *)args[0], (int)(*(int *)args[1]),
	 (int)(*(int *)args[2]), (int)(*(int *)args[3]),
         (int)*(sffi_arg *)resp);

  CHECK((int)*(int *)args[0] == 0);
  CHECK((int)*(int *)args[1] == 1);
  CHECK((int)*(int *)args[2] == 2);
  CHECK((int)*(int *)args[3] == 3);
  CHECK((int)*(sffi_arg *)resp == 9);
}

typedef int (ABI_ATTR *closure_test_type0)(int, int, int, int);

int main (void)
{
  sffi_cif cif;
  void *code;
  sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
  sffi_type * cl_arg_types[17];
  int res;

  cl_arg_types[0] = &sffi_type_uint;
  cl_arg_types[1] = &sffi_type_uint;
  cl_arg_types[2] = &sffi_type_uint;
  cl_arg_types[3] = &sffi_type_uint;
  cl_arg_types[4] = NULL;

  /* Initialize the cif */
  CHECK(sffi_prep_cif(&cif, ABI_NUM, 4,
		     &sffi_type_sint, cl_arg_types) == SFFI_OK);

  CHECK(sffi_prep_closure_loc(pcl, &cif, closure_test,
                             (void *) 3 /* userdata */, code) == SFFI_OK);

  res = (*(closure_test_type0)code)(0, 1, 2, 3);
  /* { dg-output "0 1 2 3: 9" } */

  printf("res: %d\n",res);
  /* { dg-output "\nres: 9" } */
  CHECK(res == 9);

  exit(0);
}
