/* Area:	sffi_call
   Purpose:	Check complex int128 call and return.
   Limitations:	none.
   PR:		none. */

/* { dg-do run } */
#include "ffitest.h"

/* clang defines __SIZEOF_INT128__ but does not support _Complex __int128,
   so exclude it here and fall through to the trivial main() below. */
#if defined(SFFI_TARGET_HAS_INT128) && \
    defined(SFFI_TARGET_HAS_COMPLEX_TYPE) && \
    defined(__SIZEOF_INT128__) && \
    !defined(__clang__)

typedef __int128_t i128;
typedef _Complex __int128 ci128;

static const ci128 val =
    (((i128)0x01020304050607ull << 64) | 0x08090a0b0c0d0e0full) + 
    (((i128)0x10203040506070ull << 64) | 0x8090a0b0c0d0e0f0ull) * 1i;
static const int dummy = 0xdeadbeef;

#define D(X) int X __attribute__((unused))

static ci128 f0(ci128 x)
{
  return x;
}

static ci128 f1(D(a), ci128 x)
{
  return x;
}

static ci128 f2(D(a), D(b), ci128 x)
{
  return x;
}

static ci128 f3(D(a), D(b), D(c), ci128 x)
{
  return x;
}

static ci128 f4(D(a), D(b), D(c), D(d), ci128 x)
{
  return x;
}

static ci128 f5(D(a), D(b), D(c), D(d), D(e), ci128 x)
{
  return x;
}

static ci128 f6(D(a), D(b), D(c), D(d), D(e), D(f), ci128 x)
{
  return x;
}

static ci128 f7(D(a), D(b), D(c), D(d), D(e), D(f), D(g), ci128 x)
{
  return x;
}

static ci128 f8(D(a), D(b), D(c), D(d), D(e), D(f), D(g), D(h), ci128 x)
{
  return x;
}

#define N  9

static void * const funcs[N] = {
  f0, f1, f2, f3, f4, f5, f6, f7, f8
};

static sffi_type sffi_type_ci128 = {
  sizeof(ci128),
  _Alignof(ci128),
  SFFI_TYPE_COMPLEX,
  (sffi_type *[2]){ &sffi_type_sint128, NULL },
};

int main (void)
{
  int i;

  for (i = 0; i < N; i++)
    {
      sffi_cif cif;
      sffi_status s;
      sffi_type *args[N];
      void *values[N];
      ci128 ret;
      int j;

      for (j = 0; j < i; j++)
        {
          args[j] = &sffi_type_sint;
          values[j] = (void *)&dummy;
        }
      args[i] = &sffi_type_ci128;
      values[i] = (void *)&val;

      s = sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, i + 1,
		       &sffi_type_ci128, args);
      CHECK(s == SFFI_OK);

      sffi_call(&cif, SFFI_FN(funcs[i]), &ret, values);
      CHECK(ret == val);
    }

  return 0;
}

#else
int main (void) { return 0; }
#endif
