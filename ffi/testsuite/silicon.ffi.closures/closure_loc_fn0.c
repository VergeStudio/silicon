/* Area:	closure_call
   Purpose:	Check multiple values passing from different type.
		Also, exceed the limit of gpr and fpr registers on PowerPC
		Darwin.
   Limitations:	none.
   PR:		none.
   Originator:	<andreast@gcc.gnu.org> 20030828	 */

/* { dg-do run } */

#include "ffitest.h"

static void
closure_loc_test_fn0(sffi_cif* cif __UNUSED__, void* resp, void** args,
		 void* userdata)
{
  *(sffi_arg*)resp =
    (int)*(unsigned long long *)args[0] + (int)(*(int *)args[1]) +
    (int)(*(unsigned long long *)args[2]) + (int)*(int *)args[3] +
    (int)(*(signed short *)args[4]) +
    (int)(*(unsigned long long *)args[5]) +
    (int)*(int *)args[6] + (int)(*(int *)args[7]) +
    (int)(*(double *)args[8]) + (int)*(int *)args[9] +
    (int)(*(int *)args[10]) + (int)(*(float *)args[11]) +
    (int)*(int *)args[12] + (int)(*(int *)args[13]) +
    (int)(*(int *)args[14]) +  *(int *)args[15] + (intptr_t)userdata;

  printf("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d: %d\n",
	 (int)*(unsigned long long *)args[0], (int)(*(int *)args[1]),
	 (int)(*(unsigned long long *)args[2]),
	 (int)*(int *)args[3], (int)(*(signed short *)args[4]),
	 (int)(*(unsigned long long *)args[5]),
	 (int)*(int *)args[6], (int)(*(int *)args[7]),
	 (int)(*(double *)args[8]), (int)*(int *)args[9],
	 (int)(*(int *)args[10]), (int)(*(float *)args[11]),
	 (int)*(int *)args[12], (int)(*(int *)args[13]),
	 (int)(*(int *)args[14]),*(int *)args[15],
	 (int)(intptr_t)userdata, (int)*(sffi_arg *)resp);

}

typedef int (*closure_loc_test_type0)(unsigned long long, int, unsigned long long,
				  int, signed short, unsigned long long, int,
				  int, double, int, int, float, int, int,
				  int, int);

int main (void)
{
  sffi_cif cif;
  sffi_closure *pcl;
  sffi_type * cl_arg_types[17];
  int res;
  void *codeloc;

  cl_arg_types[0] = &sffi_type_uint64;
  cl_arg_types[1] = &sffi_type_sint;
  cl_arg_types[2] = &sffi_type_uint64;
  cl_arg_types[3] = &sffi_type_sint;
  cl_arg_types[4] = &sffi_type_sshort;
  cl_arg_types[5] = &sffi_type_uint64;
  cl_arg_types[6] = &sffi_type_sint;
  cl_arg_types[7] = &sffi_type_sint;
  cl_arg_types[8] = &sffi_type_double;
  cl_arg_types[9] = &sffi_type_sint;
  cl_arg_types[10] = &sffi_type_sint;
  cl_arg_types[11] = &sffi_type_float;
  cl_arg_types[12] = &sffi_type_sint;
  cl_arg_types[13] = &sffi_type_sint;
  cl_arg_types[14] = &sffi_type_sint;
  cl_arg_types[15] = &sffi_type_sint;
  cl_arg_types[16] = NULL;

  /* Initialize the cif */
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 16,
		     &sffi_type_sint, cl_arg_types) == SFFI_OK);

  pcl = sffi_closure_alloc(sizeof(sffi_closure), &codeloc);
  CHECK(pcl != NULL);
  CHECK(codeloc != NULL);

  CHECK(sffi_prep_closure_loc(pcl, &cif, closure_loc_test_fn0,
			 (void *) 3 /* userdata */, codeloc) == SFFI_OK);

#if !defined(SFFI_EXEC_STATIC_TRAMP) && !defined(__EMSCRIPTEN__) \
    && !(defined(SFFI_EXEC_TRAMPOLINE_TABLE) && SFFI_EXEC_TRAMPOLINE_TABLE)
  /* With static trampolines or a trampoline table (Apple aarch64), the
     codeloc does not point to the closure */
  CHECK(memcmp(pcl, SFFI_CL(codeloc), sizeof(*pcl)) == 0);
#endif

  res = (*((closure_loc_test_type0)codeloc))
    (1LL, 2, 3LL, 4, 127, 429LL, 7, 8, 9.5, 10, 11, 12, 13,
     19, 21, 1);
  /* { dg-output "1 2 3 4 127 429 7 8 9 10 11 12 13 19 21 1 3: 680" } */
  printf("res: %d\n",res);
  /* { dg-output "\nres: 680" } */
  CHECK(res == 680);
  exit(0);
}
