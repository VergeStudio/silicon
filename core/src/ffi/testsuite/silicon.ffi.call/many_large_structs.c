


#include "ffitest.h"

#define NARGS 16
#define SSIZE 32

typedef struct { unsigned char b[SSIZE]; } big_struct;


static int ABI_ATTR
sum_bytes (big_struct s0, big_struct s1, big_struct s2, big_struct s3,
	   big_struct s4, big_struct s5, big_struct s6, big_struct s7,
	   big_struct s8, big_struct s9, big_struct s10, big_struct s11,
	   big_struct s12, big_struct s13, big_struct s14, big_struct s15)
{
  big_struct *all[NARGS];
  int i, j, sum = 0;

  all[0] = &s0;   all[1] = &s1;   all[2] = &s2;   all[3] = &s3;
  all[4] = &s4;   all[5] = &s5;   all[6] = &s6;   all[7] = &s7;
  all[8] = &s8;   all[9] = &s9;   all[10] = &s10; all[11] = &s11;
  all[12] = &s12; all[13] = &s13; all[14] = &s14; all[15] = &s15;

  for (i = 0; i < NARGS; i++)
    for (j = 0; j < SSIZE; j++)
      sum += all[i]->b[j];

  return sum;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[NARGS];
  void *values[NARGS];
  sffi_type bs_type;
  sffi_type *bs_elements[SSIZE + 1];
  big_struct in[NARGS];
  sffi_arg result = 0;
  int i, j, expected = 0;

  bs_type.size = 0;
  bs_type.alignment = 0;
  bs_type.type = SFFI_TYPE_STRUCT;
  for (i = 0; i < SSIZE; i++)
    bs_elements[i] = &sffi_type_uchar;
  bs_elements[SSIZE] = NULL;
  bs_type.elements = bs_elements;

  
  for (i = 0; i < NARGS; i++)
    {
      for (j = 0; j < SSIZE; j++)
	{
	  in[i].b[j] = (unsigned char) (i + 1);
	  expected += (i + 1);
	}
      args[i] = &bs_type;
      values[i] = &in[i];
    }

  CHECK(sffi_prep_cif(&cif, ABI_NUM, NARGS, &sffi_type_sint, args) == SFFI_OK);

  sffi_call(&cif, SFFI_FN(sum_bytes), &result, values);

  CHECK((int) result == expected);

  exit(0);
}
