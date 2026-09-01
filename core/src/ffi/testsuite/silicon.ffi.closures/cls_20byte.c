/* Area:	sffi_call, closure_call
   Purpose:	Check structure passing with different structure size.
		Depending on the ABI. Check overlapping.
   Limitations:	none.
   PR:		none.
   Originator:	<andreast@gcc.gnu.org> 20030828	 */

/* { dg-do run } */
#include "ffitest.h"

typedef struct cls_struct_20byte {
  double a;
  double b;
  int c;
} cls_struct_20byte;

static cls_struct_20byte cls_struct_20byte_fn(struct cls_struct_20byte a1,
			    struct cls_struct_20byte a2)
{
  struct cls_struct_20byte result;

  result.a = a1.a + a2.a;
  result.b = a1.b + a2.b;
  result.c = a1.c + a2.c;

  printf("%g %g %d %g %g %d: %g %g %d\n", a1.a, a1.b, a1.c, a2.a, a2.b, a2.c,
	 result.a, result.b, result.c);

  CHECK(a1.a == 1);
  CHECK(a1.b == 2);
  CHECK(a1.c == 3);

  CHECK(a2.a == 4);
  CHECK(a2.b == 5);
  CHECK(a2.c == 7);

  CHECK(result.a == 5);
  CHECK(result.b == 7);
  CHECK(result.c == 10);
  return result;
}

static void
cls_struct_20byte_gn(sffi_cif* cif __UNUSED__, void* resp, void** args,
		     void* userdata __UNUSED__)
{
  struct cls_struct_20byte a1, a2;

  a1 = *(struct cls_struct_20byte*)(args[0]);
  a2 = *(struct cls_struct_20byte*)(args[1]);

  *(cls_struct_20byte*)resp = cls_struct_20byte_fn(a1, a2);
}

int main (void)
{
  sffi_cif cif;
  void *code;
  sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
  void* args_dbl[5];
  sffi_type* cls_struct_fields[4];
  sffi_type cls_struct_type;
  sffi_type* dbl_arg_types[5];

  struct cls_struct_20byte g_dbl = { 1.0, 2.0, 3 };
  struct cls_struct_20byte f_dbl = { 4.0, 5.0, 7 };
  struct cls_struct_20byte res_dbl;

  cls_struct_type.size = 0;
  cls_struct_type.alignment = 0;
  cls_struct_type.type = SFFI_TYPE_STRUCT;
  cls_struct_type.elements = cls_struct_fields;

  cls_struct_fields[0] = &sffi_type_double;
  cls_struct_fields[1] = &sffi_type_double;
  cls_struct_fields[2] = &sffi_type_sint;
  cls_struct_fields[3] = NULL;

  dbl_arg_types[0] = &cls_struct_type;
  dbl_arg_types[1] = &cls_struct_type;
  dbl_arg_types[2] = NULL;

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 2, &cls_struct_type,
		     dbl_arg_types) == SFFI_OK);

  args_dbl[0] = &g_dbl;
  args_dbl[1] = &f_dbl;
  args_dbl[2] = NULL;

  sffi_call(&cif, SFFI_FN(cls_struct_20byte_fn), &res_dbl, args_dbl);
  /* { dg-output "1 2 3 4 5 7: 5 7 10" } */
  printf("res: %g %g %d\n", res_dbl.a, res_dbl.b, res_dbl.c);
  /* { dg-output "\nres: 5 7 10" } */
  CHECK(res_dbl.a == 5);
  CHECK(res_dbl.b == 7);
  CHECK(res_dbl.c == 10);

  CHECK(sffi_prep_closure_loc(pcl, &cif, cls_struct_20byte_gn, NULL, code) == SFFI_OK);

  res_dbl = ((cls_struct_20byte(*)(cls_struct_20byte, cls_struct_20byte))(code))(g_dbl, f_dbl);
  /* { dg-output "\n1 2 3 4 5 7: 5 7 10" } */
  printf("res: %g %g %d\n", res_dbl.a, res_dbl.b, res_dbl.c);
  /* { dg-output "\nres: 5 7 10" } */
  CHECK(res_dbl.a == 5);
  CHECK(res_dbl.b == 7);
  CHECK(res_dbl.c == 10);

  exit(0);
}
