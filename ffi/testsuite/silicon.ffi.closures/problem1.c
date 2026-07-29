/* Area:	sffi_call, closure_call
   Purpose:	Check structure passing with different structure size.
   Limitations:	none.
   PR:		none.
   Originator:	<andreast@gcc.gnu.org> 20030828	 */

/* { dg-do run } */
#include "ffitest.h"

typedef struct my_ffi_struct {
  double a;
  double b;
  double c;
} my_ffi_struct;

my_ffi_struct callee(struct my_ffi_struct a1, struct my_ffi_struct a2)
{
  struct my_ffi_struct result;
  result.a = a1.a + a2.a;
  result.b = a1.b + a2.b;
  result.c = a1.c + a2.c;


  printf("%g %g %g %g %g %g: %g %g %g\n", a1.a, a1.b, a1.c,
	 a2.a, a2.b, a2.c, result.a, result.b, result.c);

  return result;
}

void stub(sffi_cif* cif __UNUSED__, void* resp, void** args,
	  void* userdata __UNUSED__)
{
  struct my_ffi_struct a1;
  struct my_ffi_struct a2;

  a1 = *(struct my_ffi_struct*)(args[0]);
  a2 = *(struct my_ffi_struct*)(args[1]);

  *(my_ffi_struct *)resp = callee(a1, a2);
}


int main(void)
{
  sffi_type* my_ffi_struct_fields[4];
  sffi_type my_ffi_struct_type;
  sffi_cif cif;
  void *code;
  sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
  void* args[4];
  sffi_type* arg_types[3];

  struct my_ffi_struct g = { 1.0, 2.0, 3.0 };
  struct my_ffi_struct f = { 1.0, 2.0, 3.0 };
  struct my_ffi_struct res;

  my_ffi_struct_type.size = 0;
  my_ffi_struct_type.alignment = 0;
  my_ffi_struct_type.type = SFFI_TYPE_STRUCT;
  my_ffi_struct_type.elements = my_ffi_struct_fields;

  my_ffi_struct_fields[0] = &sffi_type_double;
  my_ffi_struct_fields[1] = &sffi_type_double;
  my_ffi_struct_fields[2] = &sffi_type_double;
  my_ffi_struct_fields[3] = NULL;

  arg_types[0] = &my_ffi_struct_type;
  arg_types[1] = &my_ffi_struct_type;
  arg_types[2] = NULL;

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 2, &my_ffi_struct_type,
		     arg_types) == SFFI_OK);

  args[0] = &g;
  args[1] = &f;
  args[2] = NULL;
  sffi_call(&cif, SFFI_FN(callee), &res, args);
  /* { dg-output "1 2 3 1 2 3: 2 4 6" } */
  printf("res: %g %g %g\n", res.a, res.b, res.c);
  /* { dg-output "\nres: 2 4 6" } */

  CHECK(sffi_prep_closure_loc(pcl, &cif, stub, NULL, code) == SFFI_OK);

  res = ((my_ffi_struct(*)(struct my_ffi_struct, struct my_ffi_struct))(code))(g, f);
  /* { dg-output "\n1 2 3 1 2 3: 2 4 6" } */
  printf("res: %g %g %g\n", res.a, res.b, res.c);
  /* { dg-output "\nres: 2 4 6" } */

  exit(0);;
}
