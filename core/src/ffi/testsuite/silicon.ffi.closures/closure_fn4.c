/* Area:	closure_call
   Purpose:	Check multiple long long values passing.
		Also, exceed the limit of gpr and fpr registers on PowerPC
		Darwin.
   Limitations:	none.
   PR:		none.
   Originator:	<andreast@gcc.gnu.org> 20031026	 */

/* { dg-do run } */

#include "ffitest.h"

static void
closure_test_fn0(sffi_cif* cif __UNUSED__, void* resp, void** args,
		 void* userdata)
{
  *(sffi_arg*)resp =
    (int)*(unsigned long long *)args[0] + (int)*(unsigned long long *)args[1] +
    (int)*(unsigned long long *)args[2] + (int)*(unsigned long long *)args[3] +
    (int)*(unsigned long long *)args[4] + (int)*(unsigned long long *)args[5] +
    (int)*(unsigned long long *)args[6] + (int)*(unsigned long long *)args[7] +
    (int)*(unsigned long long *)args[8] + (int)*(unsigned long long *)args[9] +
    (int)*(unsigned long long *)args[10] +
    (int)*(unsigned long long *)args[11] +
    (int)*(unsigned long long *)args[12] +
    (int)*(unsigned long long *)args[13] +
    (int)*(unsigned long long *)args[14] +
    *(int *)args[15] + (intptr_t)userdata;

  printf("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d: %d\n",
	 (int)*(unsigned long long *)args[0],
	 (int)*(unsigned long long *)args[1],
	 (int)*(unsigned long long *)args[2],
	 (int)*(unsigned long long *)args[3],
	 (int)*(unsigned long long *)args[4],
	 (int)*(unsigned long long *)args[5],
	 (int)*(unsigned long long *)args[6],
	 (int)*(unsigned long long *)args[7],
	 (int)*(unsigned long long *)args[8],
	 (int)*(unsigned long long *)args[9],
	 (int)*(unsigned long long *)args[10],
	 (int)*(unsigned long long *)args[11],
	 (int)*(unsigned long long *)args[12],
	 (int)*(unsigned long long *)args[13],
	 (int)*(unsigned long long *)args[14],
	 *(int *)args[15],
	 (int)(intptr_t)userdata, (int)*(sffi_arg *)resp);
  CHECK((int)*(sffi_arg *)resp == 680);

}

typedef int (*closure_test_type0)(unsigned long long, unsigned long long,
				  unsigned long long, unsigned long long,
				  unsigned long long, unsigned long long,
				  unsigned long long, unsigned long long,
				  unsigned long long, unsigned long long,
				  unsigned long long, unsigned long long,
				  unsigned long long, unsigned long long,
				  unsigned long long, int);

int main (void)
{
  sffi_cif cif;
  void *code;
  sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
  sffi_type * cl_arg_types[17];
  int i, res;

  for (i = 0; i < 15; i++) {
    cl_arg_types[i] = &sffi_type_uint64;
  }
  cl_arg_types[15] = &sffi_type_sint;
  cl_arg_types[16] = NULL;

  /* Initialize the cif */
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 16,
		     &sffi_type_sint, cl_arg_types) == SFFI_OK);

  CHECK(sffi_prep_closure_loc(pcl, &cif, closure_test_fn0,
                             (void *) 3 /* userdata */, code) == SFFI_OK);

  res = (*((closure_test_type0)code))
    (1LL, 2LL, 3LL, 4LL, 127LL, 429LL, 7LL, 8LL, 9LL, 10LL, 11LL, 12LL,
     13LL, 19LL, 21LL, 1);
  /* { dg-output "1 2 3 4 127 429 7 8 9 10 11 12 13 19 21 1 3: 680" } */
  printf("res: %d\n",res);
  /* { dg-output "\nres: 680" } */
  CHECK(res == 680);

  exit(0);
}
