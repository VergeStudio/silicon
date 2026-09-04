#include "ffitest.h"

typedef struct A {
  int a, b;
} A;

typedef struct B {
  struct A y;
} B;

static struct B B_fn(int b0, struct B b1)
{
  b1.y.a += b0;
  b1.y.b -= b0;
  return b1;
}

static void
B_gn(sffi_cif* cif __UNUSED__, void* resp, void** args,
     void* userdata __UNUSED__)
{
  int b0;
  struct B b1;

  b0 = *(int*)(args[0]);
  b1 = *(struct B*)(args[1]);

  *(B*)resp = B_fn(b0, b1);
}

int main (void)
{
  sffi_cif cif;
  void *code;
  sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
  void* args_dbl[3];
  sffi_type* cls_struct_fields[3];
  sffi_type* cls_struct_fields1[2];
  sffi_type cls_struct_type, cls_struct_type1;
  sffi_type* dbl_arg_types[3];

  int e_dbl = 12125;
  struct B f_dbl = { { 31625, 16723 } };

  struct B res_dbl;

  cls_struct_type.size = 0;
  cls_struct_type.alignment = 0;
  cls_struct_type.type = SFFI_TYPE_STRUCT;
  cls_struct_type.elements = cls_struct_fields;

  cls_struct_type1.size = 0;
  cls_struct_type1.alignment = 0;
  cls_struct_type1.type = SFFI_TYPE_STRUCT;
  cls_struct_type1.elements = cls_struct_fields1;

  cls_struct_fields[0] = &sffi_type_sint;
  cls_struct_fields[1] = &sffi_type_sint;
  cls_struct_fields[2] = NULL;

  cls_struct_fields1[0] = &cls_struct_type;
  cls_struct_fields1[1] = NULL;

  dbl_arg_types[0] = &sffi_type_sint;
  dbl_arg_types[1] = &cls_struct_type1;
  dbl_arg_types[2] = NULL;

  res_dbl = B_fn(e_dbl, f_dbl);
  printf("0 res: %d %d\n", res_dbl.y.a, res_dbl.y.b);

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 2, &cls_struct_type1,
                    dbl_arg_types) == SFFI_OK);

  args_dbl[0] = &e_dbl;
  args_dbl[1] = &f_dbl;
  args_dbl[2] = NULL;

  sffi_call(&cif, SFFI_FN(B_fn), &res_dbl, args_dbl);
  printf("1 res: %d %d\n", res_dbl.y.a, res_dbl.y.b);

  CHECK( res_dbl.y.a == (f_dbl.y.a + e_dbl));
  CHECK( res_dbl.y.b == (f_dbl.y.b - e_dbl));

  CHECK(sffi_prep_closure_loc(pcl, &cif, B_gn, NULL, code) == SFFI_OK);

  res_dbl = ((B(*)(int, B))(code))(e_dbl, f_dbl);
  printf("2 res: %d %d\n", res_dbl.y.a, res_dbl.y.b);

  CHECK( res_dbl.y.a == (f_dbl.y.a + e_dbl));
  CHECK( res_dbl.y.b == (f_dbl.y.b - e_dbl));

  exit(0);
}
