
#include "ffitest.h"
#include <complex.h>

typedef struct cls_struct_align {
  unsigned char a;
  _Complex T_C_TYPE b;
  unsigned char c;
} cls_struct_align;

cls_struct_align cls_struct_align_fn(
	struct cls_struct_align a1, struct cls_struct_align a2)
{
  struct cls_struct_align result;

  result.a = a1.a + a2.a;
  result.b = a1.b + a2.b;
  result.c = a1.c + a2.c;

  printf("%d %f,%fi %d %d %f,%fi %d: %d %f,%fi %d\n",
	 a1.a, T_CONV creal (a1.b), T_CONV cimag (a1.b), a1.c,
	 a2.a, T_CONV creal (a2.b), T_CONV cimag (a2.b), a2.c,
	 result.a, T_CONV creal (result.b), T_CONV cimag (result.b), result.c);

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
  void* args_c[5];
  sffi_type* cls_struct_fields[4];
  sffi_type cls_struct_type;
  sffi_type* c_arg_types[5];

  struct cls_struct_align g_c = { 12, 4951 + 7 * I, 127 };
  struct cls_struct_align f_c = { 1, 9320 + 1 * I, 13 };
  struct cls_struct_align res_c;

  cls_struct_type.size = 0;
  cls_struct_type.alignment = 0;
  cls_struct_type.type = SFFI_TYPE_STRUCT;
  cls_struct_type.elements = cls_struct_fields;

  cls_struct_fields[0] = &sffi_type_uchar;
  cls_struct_fields[1] = &T_FFI_TYPE;
  cls_struct_fields[2] = &sffi_type_uchar;
  cls_struct_fields[3] = NULL;

  c_arg_types[0] = &cls_struct_type;
  c_arg_types[1] = &cls_struct_type;
  c_arg_types[2] = NULL;

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 2, &cls_struct_type,
		     c_arg_types) == SFFI_OK);

  args_c[0] = &g_c;
  args_c[1] = &f_c;
  args_c[2] = NULL;

  sffi_call(&cif, SFFI_FN(cls_struct_align_fn), &res_c, args_c);
  
  printf("res: %d %f,%fi %d\n",
	 res_c.a, T_CONV  creal (res_c.b), T_CONV  cimag (res_c.b), res_c.c);
  

  CHECK(sffi_prep_closure_loc(pcl, &cif, cls_struct_align_gn, NULL, code) == SFFI_OK);

  res_c = ((cls_struct_align(*)(cls_struct_align, cls_struct_align))(code))(g_c, f_c);
  
  printf("res: %d %f,%fi %d\n",
	 res_c.a, T_CONV  creal (res_c.b), T_CONV  cimag (res_c.b), res_c.c);
  

  exit(0);
}
