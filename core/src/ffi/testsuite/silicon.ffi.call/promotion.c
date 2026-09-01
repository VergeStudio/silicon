/* Area:	sffi_call
   Purpose:	Promotion test.
   Limitations:	none.
   PR:		none.
   Originator:	From the original ffitest.c  */

/* { dg-do run } */
#include "ffitest.h"
static int promotion(signed char sc, signed short ss,
		     unsigned char uc, unsigned short us)
{
  int r = (int) sc + (int) ss + (int) uc + (int) us;

  return r;
}

int main (void)
{
  sffi_cif cif;
  sffi_type *args[MAX_ARGS];
  void *values[MAX_ARGS];
  sffi_arg rint;
  signed char sc;
  unsigned char uc;
  signed short ss;
  unsigned short us;
  unsigned long ul;

  args[0] = &sffi_type_schar;
  args[1] = &sffi_type_sshort;
  args[2] = &sffi_type_uchar;
  args[3] = &sffi_type_ushort;
  values[0] = &sc;
  values[1] = &ss;
  values[2] = &uc;
  values[3] = &us;

  /* Initialize the cif */
  CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 4,
		     &sffi_type_sint, args) == SFFI_OK);

  us = 0;
  ul = 0;

  for (sc = (signed char) -127;
       sc <= (signed char) 120; sc += 1)
    for (ss = -30000; ss <= 30000; ss += 10000)
      for (uc = (unsigned char) 0;
	   uc <= (unsigned char) 200; uc += 20)
	for (us = 0; us <= 60000; us += 10000)
	  {
	    ul++;
	    sffi_call(&cif, SFFI_FN(promotion), &rint, values);
	    CHECK((int)rint == (signed char) sc + (signed short) ss +
		  (unsigned char) uc + (unsigned short) us);
	  }
  printf("%lu promotion tests run\n", ul);
  exit(0);
}
