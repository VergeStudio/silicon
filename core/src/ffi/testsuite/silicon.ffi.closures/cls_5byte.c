#include "ffitest.h"

typedef struct cls_struct_5byte {
  unsigned short a;
  unsigned short b;
  unsigned char c;
} cls_struct_5byte;

static cls_struct_5byte cls_struct_5byte_fn(struct cls_struct_5byte a1,
			    struct cls_struct_5byte a2)
{
  struct cls_struct_5byte result;

  result.a = a1.a + a2.a;
  result.b = a1.b + a2.b;
  result.c = a1.c + a2.c;

  printf("%d %d %d %d %d %d: %d %d %d\n", a1.a, a1.b, a1.c,
	 a2.a, a2.b, a2.c,
	 result.a, result.b, result.c);

  CHECK(a1.a == 127);
  CHECK(a1.b == 120);
  CHECK(a1.c == 1);

  CHECK(a2.a == 12);
  CHECK(a2.b == 128);
  CHECK(a2.c == 9);

  CHECK(result.a == 139);
  CHECK(result.b == 248);
  CHECK(result.c == 10);

  return  result;
}

static void
cls_struct_5byte_gn(sffi_cif* cif __UNUSED__, void* resp, void** args,
		    void* userdata __UNUSED__)
{

  struct cls_struct_5byte a1, a2;

  a1 = *(struct cls_struct_5byte*)(args[0]);
  a2 = *(struct cls_struct_5byte*)(args[1]);

  *(cls_struct_5byte*)resp = cls_struct_5byte_fn(a1, a2);
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

  struct cls_struct_5byte g_dbl = { 127, 120, 1 };
  struct cls_struct_5byte f_dbl = { 12, 128, 9 };
  struct cls_struct_5byte res_dbl = { 0, 0, 0 };

  cls_struct_type.size = 0;
  cls_struct_type.alignment = 0;
  cls_struct_type.type = SFFI_TYPE_STRUCT;
  cls_struct_type.elements = cls_struct_fields;

  cls_struct_fields[0] = &sffi_type_ushort;
  cls_struct_fields[1] = &sffi_type_ushort;
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

  sffi_call(&cif, SFFI_FN(cls_struct_5byte_fn), &res_dbl, args_dbl);

  printf("res: %d %d %d\n", res_dbl.a, res_dbl.b, res_dbl.c);

  CHECK(res_dbl.a == 139);
  CHECK(res_dbl.b == 248);
  CHECK(res_dbl.c == 10);

  res_dbl.a = 0;
  res_dbl.b = 0;
  res_dbl.c = 0;

  CHECK(sffi_prep_closure_loc(pcl, &cif, cls_struct_5byte_gn, NULL, code) == SFFI_OK);

  res_dbl = ((cls_struct_5byte(*)(cls_struct_5byte, cls_struct_5byte))(code))(g_dbl, f_dbl);

  printf("res: %d %d %d\n", res_dbl.a, res_dbl.b, res_dbl.c);

  CHECK(res_dbl.a == 139);
  CHECK(res_dbl.b == 248);
  CHECK(res_dbl.c == 10);

  exit(0);
}
