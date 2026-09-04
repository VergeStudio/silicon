#include "ffitest.h"

typedef struct A {
  int a;
} A;

static struct A A_fn(int b0, struct A b1)
{
  b1.a += b0;
  return b1;
}

static void
A_gn(sffi_cif* cif __UNUSED__, void* resp, void** args,
     void* userdata __UNUSED__)
{
  int b0;
  struct A b1;

  b0 = *(int*)(args[0]);
  b1 = *(struct A*)(args[1]);

  *(A*)resp = A_fn(b0, b1);
}

int main (void)
{
  printf("123\n");
  sffi_cif cif;
  void *code;
  sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
  void* args_dbl[3];
  sffi_type* cls_struct_fields[2];
  sffi_type cls_struct_type;
  sffi_type* dbl_arg_types[3];

  int e_dbl = 12125;
  struct A f_dbl = { 31625 };

  struct A res_dbl;

  cls_struct_type.size = 0;
  cls_struct_type.alignment = 0;
  cls_struct_type.type = SFFI_TYPE_STRUCT;
  cls_struct_type.elements = cls_struct_fields;

  cls_struct_fields[0] = &sffi_type_sint;
  cls_struct_fields[1] = NULL;

  dbl_arg_types[0] = &sffi_type_sint;
  dbl_arg_types[1] = &cls_struct_type;
  dbl_arg_types[2] = NULL;

  res_dbl = A_fn(e_dbl, f_dbl);
  printf("0 res: %d\n", res_dbl.a);

  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 2, &cls_struct_type,
                    dbl_arg_types) == SFFI_OK);

  args_dbl[0] = &e_dbl;
  args_dbl[1] = &f_dbl;
  args_dbl[2] = NULL;

  sffi_call(&cif, SFFI_FN(A_fn), &res_dbl, args_dbl);
  printf("1 res: %d\n", res_dbl.a);

  CHECK( res_dbl.a == (e_dbl + f_dbl.a));

  CHECK(sffi_prep_closure_loc(pcl, &cif, A_gn, NULL, code) == SFFI_OK);

  res_dbl = ((A(*)(int, A))(code))(e_dbl, f_dbl);
  printf("2 res: %d\n", res_dbl.a);

  CHECK( res_dbl.a == (e_dbl + f_dbl.a));

  exit(0);
}
