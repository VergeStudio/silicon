/* Area:	sffi_call, closure_call
   Purpose:	Check structure alignment of sint16.
   Limitations:	none.
   PR:		none.
   Originator:	<hos@tamanegi.org> 20031203	 */

/* { dg-do run } */
#include "ffitest.h"

typedef struct cls_struct_align {
  unsigned char a;
  signed short b;
  unsigned char c;
} cls_struct_align;

static cls_struct_align cls_struct_align_fn(struct cls_struct_align a1,
			    struct cls_struct_align a2)
{
  struct cls_struct_align result;

  result.a = a1.a + a2.a;
  result.b = a1.b + a2.b;
  result.c = a1.c + a2.c;

  printf("%d %d %d %d %d %d: %d %d %d\n", a1.a, a1.b, a1.c, a2.a, a2.b, a2.c, result.a, result.b, result.c);
  CHECK(a1.a == 12);
  CHECK(a1.b == 4951);
  CHECK(a1.c == 127);

  CHECK(a2.a == 1);
  CHECK(a2.b == 9320);
  CHECK(a2.c == 13);

  CHECK(result.a == 13);
  CHECK(result.b == 14271);
  CHECK(result.c == 140);

  return  result;
}

static void
cls_struct_align_gn(sffi_cif* cif __UNUSED__, void* resp, void** args,
		    void* userdata __UNUSED__)
{

  struct cls_struct_align a1, a2;

  a1 = *(struct cls_struct_align*)(args[0]);
  a2 = *(struct cls_struct_align*)(args[1]);

  *(cls_struct_align*)resp = cls_struct_align_fn(a1, a2);
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

  struct cls_struct_align g_dbl = { 12, 4951, 127 };
  struct cls_struct_align f_dbl = { 1, 9320, 13 };
  struct cls_struct_align res_dbl;

  cls_struct_type.size = 0;
  cls_struct_type.alignment = 0;
  cls_struct_type.type = SFFI_TYPE_STRUCT;
  cls_struct_type.elements = cls_struct_fields;

  cls_struct_fields[0] = &sffi_type_uchar;
  cls_struct_fields[1] = &sffi_type_sshort;
  cls_struct_fields[2] = &sffi_type_uchar;
  cls_struct_fields[3] = NULL;

  dbl_arg_types[0] = &cls_struct_type;
  dbl_arg_types[1] = &cls_struct_type;
  dbl_arg_types[2] = NULL;

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 2, &cls_struct_type,
		     dbl_arg_types) == SFFI_OK);

  args_dbl[0] = &g_dbl;
  args_dbl[1] = &f_dbl;
  args_dbl[2] = NULL;

  sffi_call(&cif, SFFI_FN(cls_struct_align_fn), &res_dbl, args_dbl);
  /* { dg-output "12 4951 127 1 9320 13: 13 14271 140" } */
  printf("res: %d %d %d\n", res_dbl.a, res_dbl.b, res_dbl.c);
  /* { dg-output "\nres: 13 14271 140" } */
  CHECK(res_dbl.a == 13);
  CHECK(res_dbl.b == 14271);
  CHECK(res_dbl.c == 140);

  CHECK(sffi_prep_closure_loc(pcl, &cif, cls_struct_align_gn, NULL, code) == SFFI_OK);

  res_dbl = ((cls_struct_align(*)(cls_struct_align, cls_struct_align))(code))(g_dbl, f_dbl);
  /* { dg-output "\n12 4951 127 1 9320 13: 13 14271 140" } */
  printf("res: %d %d %d\n", res_dbl.a, res_dbl.b, res_dbl.c);
  /* { dg-output "\nres: 13 14271 140" } */
  CHECK(res_dbl.a == 13);
  CHECK(res_dbl.b == 14271);
  CHECK(res_dbl.c == 140);

  exit(0);
}
