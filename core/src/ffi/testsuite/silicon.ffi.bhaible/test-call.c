#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sffi.h>
#include "alignof.h"
#include <stdarg.h>

#ifdef DGTEST

#define exit(V) (void)((V) ? (abort(), 1) : exit(0))
int count = 0;
char rbuf1[2048];
char rbuf2[2048];
int _fprintf(FILE *stream, const char *format, ...)
{
  va_list args;
  va_start(args, format);

  switch (count++)
    {
    case 0:
    case 1:
      vsprintf(&rbuf1[strlen(rbuf1)], format, args);
      break;
    case 2:
      printf("%s", rbuf1);
      vsprintf(rbuf2, format, args);
      break;
    case 3:
      vsprintf(&rbuf2[strlen(rbuf2)], format, args);
      printf("%s", rbuf2);
      fflush (stdout);
      if (strcmp (rbuf1, rbuf2)) abort();
      break;
    }

  va_end(args);

  return 0;
}
#define fprintf _fprintf
#endif

#include "testcases.c"

#ifndef ABI_NUM
#define ABI_NUM SFFI_DEFAULT_ABI
#endif

static sffi_type sffi_type_char;
#define sffi_type_slonglong sffi_type_sint64
#define sffi_type_ulonglong sffi_type_uint64

#define SKIP_EXTRA_STRUCTS

#define SFFI_PREP_CIF(cif,argtypes,rettype) \
  if (sffi_prep_cif(&(cif),ABI_NUM,sizeof(argtypes)/sizeof(argtypes[0]),&rettype,argtypes) != SFFI_OK) abort()
#define SFFI_PREP_CIF_NOARGS(cif,rettype) \
  if (sffi_prep_cif(&(cif),ABI_NUM,0,&rettype,NULL) != SFFI_OK) abort()
#define SFFI_CALL(cif,fn,args,retaddr) \
  sffi_call(&(cif),(void(*)(void))(fn),retaddr,args)

long clear_traces_i (long a, long b, long c, long d, long e, long f, long g, long h,
                     long i, long j, long k, long l, long m, long n, long o, long p)
{ return 0; }
float clear_traces_f (float a, float b, float c, float d, float e, float f, float g,
                      float h, float i, float j, float k, float l, float m, float n,
                      float o, float p)
{ return 0.0; }
double clear_traces_d (double a, double b, double c, double d, double e, double f, double g,
                       double h, double i, double j, double k, double l, double m, double n,
                       double o, double p)
{ return 0.0; }
J clear_traces_J (void)
{ J j; j.l1 = j.l2 = 0; return j; }
void clear_traces (void)
{ clear_traces_i(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
  clear_traces_f(0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0);
  clear_traces_d(0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0);
  clear_traces_J();
}

void
  void_tests (void)
{
#if (!defined(DGTEST)) || DGTEST == 1  
  v_v();
  clear_traces();
  {
    sffi_cif cif;
    SFFI_PREP_CIF_NOARGS(cif,sffi_type_void);
    {
      SFFI_CALL(cif,v_v,NULL,NULL);
    }
  }
#endif  
  return;
}
void
  int_tests (void)
{
  int ir;
  sffi_arg retvalue;
#if (!defined(DGTEST)) || DGTEST == 2
  ir = i_v();
  fprintf(out,"->%d\n",ir);
  fflush(out);
  ir = 0; clear_traces();
  {
    sffi_cif cif;
    SFFI_PREP_CIF_NOARGS(cif,sffi_type_sint);
    {
      SFFI_CALL(cif,i_v,NULL,&retvalue);
      ir = retvalue;
    }
  }
  fprintf(out,"->%d\n",ir);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 3
  ir = i_i(i1);
  fprintf(out,"->%d\n",ir);
  fflush(out);
  ir = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_sint);
    {
       void* args[] = { &i1 };
      SFFI_CALL(cif,i_i,args,&retvalue);
      ir = retvalue;
    }
  }
  fprintf(out,"->%d\n",ir);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 4
  ir = i_i2(i1,i2);
  fprintf(out,"->%d\n",ir);
  fflush(out);
  ir = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_sint, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_sint);
    {
       void* args[] = { &i1, &i2 };
      SFFI_CALL(cif,i_i2,args,&retvalue);
      ir = retvalue;
    }
  }
  fprintf(out,"->%d\n",ir);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 5
  ir = i_i4(i1,i2,i3,i4);
  fprintf(out,"->%d\n",ir);
  fflush(out);
  ir = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_sint);
    {
       void* args[] = { &i1, &i2, &i3, &i4 };
      SFFI_CALL(cif,i_i4,args,&retvalue);
      ir = retvalue;
    }
  }
  fprintf(out,"->%d\n",ir);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 6
  ir = i_i8(i1,i2,i3,i4,i5,i6,i7,i8);
  fprintf(out,"->%d\n",ir);
  fflush(out);
  ir = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_sint);
    {
       void* args[] = { &i1, &i2, &i3, &i4, &i5, &i6, &i7, &i8 };
      SFFI_CALL(cif,i_i8,args,&retvalue);
      ir = retvalue;
    }
  }
  fprintf(out,"->%d\n",ir);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 7
  ir = i_i16(i1,i2,i3,i4,i5,i6,i7,i8,i9,i10,i11,i12,i13,i14,i15,i16);
  fprintf(out,"->%d\n",ir);
  fflush(out);
  ir = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_sint);
    {
       void* args[] = { &i1, &i2, &i3, &i4, &i5, &i6, &i7, &i8, &i9, &i10, &i11, &i12, &i13, &i14, &i15, &i16 };
      SFFI_CALL(cif,i_i16,args,&retvalue);
      ir = retvalue;
    }
  }
  fprintf(out,"->%d\n",ir);
  fflush(out);
#endif

  return;
}
void
  float_tests (void)
{
  float fr;

#if (!defined(DGTEST)) || DGTEST == 8
  fr = f_f(f1);
  fprintf(out,"->%g\n",fr);
  fflush(out);
  fr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_float };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_float);
    {
       void* args[] = { &f1 };
      SFFI_CALL(cif,f_f,args,&fr);
    }
  }
  fprintf(out,"->%g\n",fr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 9
  fr = f_f2(f1,f2);
  fprintf(out,"->%g\n",fr);
  fflush(out);
  fr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_float, &sffi_type_float };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_float);
    {
       void* args[] = { &f1, &f2 };
      SFFI_CALL(cif,f_f2,args,&fr);
    }
  }
  fprintf(out,"->%g\n",fr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 10
  fr = f_f4(f1,f2,f3,f4);
  fprintf(out,"->%g\n",fr);
  fflush(out);
  fr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_float);
    {
       void* args[] = { &f1, &f2, &f3, &f4 };
      SFFI_CALL(cif,f_f4,args,&fr);
    }
  }
  fprintf(out,"->%g\n",fr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 11
  fr = f_f8(f1,f2,f3,f4,f5,f6,f7,f8);
  fprintf(out,"->%g\n",fr);
  fflush(out);
  fr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_float);
    {
       void* args[] = { &f1, &f2, &f3, &f4, &f5, &f6, &f7, &f8 };
      SFFI_CALL(cif,f_f8,args,&fr);
    }
  }
  fprintf(out,"->%g\n",fr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 12
  fr = f_f16(f1,f2,f3,f4,f5,f6,f7,f8,f9,f10,f11,f12,f13,f14,f15,f16);
  fprintf(out,"->%g\n",fr);
  fflush(out);
  fr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_float);
    {
       void* args[] = { &f1, &f2, &f3, &f4, &f5, &f6, &f7, &f8, &f9, &f10, &f11, &f12, &f13, &f14, &f15, &f16 };
      SFFI_CALL(cif,f_f16,args,&fr);
    }
  }
  fprintf(out,"->%g\n",fr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 13
  fr = f_f24(f1,f2,f3,f4,f5,f6,f7,f8,f9,f10,f11,f12,f13,f14,f15,f16,f17,f18,f19,f20,f21,f22,f23,f24);
  fprintf(out,"->%g\n",fr);
  fflush(out);
  fr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_float);
    {
       void* args[] = { &f1, &f2, &f3, &f4, &f5, &f6, &f7, &f8, &f9, &f10, &f11, &f12, &f13, &f14, &f15, &f16, &f17, &f18, &f19, &f20, &f21, &f22, &f23, &f24 };
      SFFI_CALL(cif,f_f24,args,&fr);
    }
  }
  fprintf(out,"->%g\n",fr);
  fflush(out);
#endif
}
void
  double_tests (void)
{
  double dr;

#if (!defined(DGTEST)) || DGTEST == 14

  dr = d_d(d1);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_double };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &d1 };
      SFFI_CALL(cif,d_d,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 15
  dr = d_d2(d1,d2);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_double, &sffi_type_double };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &d1, &d2 };
      SFFI_CALL(cif,d_d2,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 16
  dr = d_d4(d1,d2,d3,d4);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &d1, &d2, &d3, &d4 };
      SFFI_CALL(cif,d_d4,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 17
  dr = d_d8(d1,d2,d3,d4,d5,d6,d7,d8);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &d1, &d2, &d3, &d4, &d5, &d6, &d7, &d8 };
      SFFI_CALL(cif,d_d8,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 18
  dr = d_d16(d1,d2,d3,d4,d5,d6,d7,d8,d9,d10,d11,d12,d13,d14,d15,d16);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &d1, &d2, &d3, &d4, &d5, &d6, &d7, &d8, &d9, &d10, &d11, &d12, &d13, &d14, &d15, &d16 };
      SFFI_CALL(cif,d_d16,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif  
  return;
}
void
  pointer_tests (void)
{
  void* vpr;

#if (!defined(DGTEST)) || DGTEST == 19
  vpr = vp_vpdpcpsp(&uc1,&d2,str3,&I4);
  fprintf(out,"->0x%p\n",vpr);
  fflush(out);
  vpr = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_pointer, &sffi_type_pointer, &sffi_type_pointer, &sffi_type_pointer };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_pointer);
    {
      void* puc1 = &uc1;
      void* pd2 = &d2;
      void* pstr3 = str3;
      void* pI4 = &I4;
       void* args[] = { &puc1, &pd2, &pstr3, &pI4 };
      SFFI_CALL(cif,vp_vpdpcpsp,args,&vpr);
    }
  }
  fprintf(out,"->0x%p\n",vpr);
  fflush(out);
#endif  
  return;
}
void
  mixed_number_tests (void)
{
  uchar ucr;
  ushort usr;
  float fr;
  double dr;
  long long llr;

#if (!defined(DGTEST)) || DGTEST == 20
  ucr = uc_ucsil(uc1, us2, ui3, ul4);
  fprintf(out,"->%u\n",ucr);
  fflush(out);
  ucr = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_uchar, &sffi_type_ushort, &sffi_type_uint, &sffi_type_ulong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_uchar);
    {
      sffi_arg r;
       void* args[] = { &uc1, &us2, &ui3, &ul4 };
      SFFI_CALL(cif,uc_ucsil,args,&r);
      ucr = (uchar) r;
    }
  }
  fprintf(out,"->%u\n",ucr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 21

  dr = d_iidd(i1,i2,d3,d4);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_sint, &sffi_type_sint, &sffi_type_double, &sffi_type_double };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &i1, &i2, &d3, &d4 };
      SFFI_CALL(cif,d_iidd,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 22
  dr = d_iiidi(i1,i2,i3,d4,i5);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_double, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &i1, &i2, &i3, &d4, &i5 };
      SFFI_CALL(cif,d_iiidi,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 23
  dr = d_idid(i1,d2,i3,d4);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_sint, &sffi_type_double, &sffi_type_sint, &sffi_type_double };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &i1, &d2, &i3, &d4 };
      SFFI_CALL(cif,d_idid,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 24
  dr = d_fdi(f1,d2,i3);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_float, &sffi_type_double, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &f1, &d2, &i3 };
      SFFI_CALL(cif,d_fdi,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 25
  usr = us_cdcd(c1,d2,c3,d4);
  fprintf(out,"->%u\n",usr);
  fflush(out);
  usr = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_char, &sffi_type_double, &sffi_type_char, &sffi_type_double };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_ushort);
    {
      sffi_arg rint;
       void* args[] = { &c1, &d2, &c3, &d4 };
      SFFI_CALL(cif,us_cdcd,args,&rint);
      usr = (ushort) rint;
    }
  }
  fprintf(out,"->%u\n",usr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 26

  llr = ll_iiilli(i1,i2,i3,ll1,i13);
  fprintf(out,"->0x%lx%08lx\n",(long)(llr>>32),(long)(llr&0xffffffff));
  fflush(out);
  llr = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_sint, &sffi_type_sint, &sffi_type_sint, &sffi_type_slonglong, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_slonglong);
    {
       void* args[] = { &i1, &i2, &i3, &ll1, &i13 };
      SFFI_CALL(cif,ll_iiilli,args,&llr);
    }
  }
  fprintf(out,"->0x%lx%08lx\n",(long)(llr>>32),(long)(llr&0xffffffff));
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 27
  llr = ll_flli(f13,ll1,i13);
  fprintf(out,"->0x%lx%08lx\n",(long)(llr>>32),(long)(llr&0xffffffff));
  fflush(out);
  llr = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_float, &sffi_type_slonglong, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_slonglong);
    {
       void* args[] = { &f13, &ll1, &i13 };
      SFFI_CALL(cif,ll_flli,args,&llr);
    }
  }
  fprintf(out,"->0x%lx%08lx\n",(long)(llr>>32),(long)(llr&0xffffffff));
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 28
  fr = f_fi(f1,i9);
  fprintf(out,"->%g\n",fr);
  fflush(out);
  fr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_float, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_float);
    {
       void* args[] = { &f1, &i9 };
      SFFI_CALL(cif,f_fi,args,&fr);
    }
  }
  fprintf(out,"->%g\n",fr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 29
  fr = f_f2i(f1,f2,i9);
  fprintf(out,"->%g\n",fr);
  fflush(out);
  fr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_float, &sffi_type_float, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_float);
    {
       void* args[] = { &f1, &f2, &i9 };
      SFFI_CALL(cif,f_f2i,args,&fr);
    }
  }
  fprintf(out,"->%g\n",fr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 30
  fr = f_f3i(f1,f2,f3,i9);
  fprintf(out,"->%g\n",fr);
  fflush(out);
  fr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_float);
    {
       void* args[] = { &f1, &f2, &f3, &i9 };
      SFFI_CALL(cif,f_f3i,args,&fr);
    }
  }
  fprintf(out,"->%g\n",fr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 31
  fr = f_f4i(f1,f2,f3,f4,i9);
  fprintf(out,"->%g\n",fr);
  fflush(out);
  fr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_float);
    {
       void* args[] = { &f1, &f2, &f3, &f4, &i9 };
      SFFI_CALL(cif,f_f4i,args,&fr);
    }
  }
  fprintf(out,"->%g\n",fr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 32
  fr = f_f7i(f1,f2,f3,f4,f5,f6,f7,i9);
  fprintf(out,"->%g\n",fr);
  fflush(out);
  fr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_float);
    {
       void* args[] = { &f1, &f2, &f3, &f4, &f5, &f6, &f7, &i9 };
      SFFI_CALL(cif,f_f7i,args,&fr);
    }
  }
  fprintf(out,"->%g\n",fr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 33
  fr = f_f8i(f1,f2,f3,f4,f5,f6,f7,f8,i9);
  fprintf(out,"->%g\n",fr);
  fflush(out);
  fr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_float);
    {
       void* args[] = { &f1, &f2, &f3, &f4, &f5, &f6, &f7, &f8, &i9 };
      SFFI_CALL(cif,f_f8i,args,&fr);
    }
  }
  fprintf(out,"->%g\n",fr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 34
  fr = f_f12i(f1,f2,f3,f4,f5,f6,f7,f8,f9,f10,f11,f12,i9);
  fprintf(out,"->%g\n",fr);
  fflush(out);
  fr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_float);
    {
       void* args[] = { &f1, &f2, &f3, &f4, &f5, &f6, &f7, &f8, &f9, &f10, &f11, &f12, &i9 };
      SFFI_CALL(cif,f_f12i,args,&fr);
    }
  }
  fprintf(out,"->%g\n",fr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 35
  fr = f_f13i(f1,f2,f3,f4,f5,f6,f7,f8,f9,f10,f11,f12,f13,i9);
  fprintf(out,"->%g\n",fr);
  fflush(out);
  fr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_float);
    {
       void* args[] = { &f1, &f2, &f3, &f4, &f5, &f6, &f7, &f8, &f9, &f10, &f11, &f12, &f13, &i9 };
      SFFI_CALL(cif,f_f13i,args,&fr);
    }
  }
  fprintf(out,"->%g\n",fr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 36
  dr = d_di(d1,i9);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_double, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &d1, &i9 };
      SFFI_CALL(cif,d_di,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 37
  dr = d_d2i(d1,d2,i9);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_double, &sffi_type_double, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &d1, &d2, &i9 };
      SFFI_CALL(cif,d_d2i,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 38
  dr = d_d3i(d1,d2,d3,i9);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &d1, &d2, &d3, &i9 };
      SFFI_CALL(cif,d_d3i,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 39
  dr = d_d4i(d1,d2,d3,d4,i9);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &d1, &d2, &d3, &d4, &i9 };
      SFFI_CALL(cif,d_d4i,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 40
  dr = d_d7i(d1,d2,d3,d4,d5,d6,d7,i9);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &d1, &d2, &d3, &d4, &d5, &d6, &d7, &i9 };
      SFFI_CALL(cif,d_d7i,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 41
  dr = d_d8i(d1,d2,d3,d4,d5,d6,d7,d8,i9);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &d1, &d2, &d3, &d4, &d5, &d6, &d7, &d8, &i9 };
      SFFI_CALL(cif,d_d8i,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 42
  dr = d_d12i(d1,d2,d3,d4,d5,d6,d7,d8,d9,d10,d11,d12,i9);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &d1, &d2, &d3, &d4, &d5, &d6, &d7, &d8, &d9, &d10, &d11, &d12, &i9 };
      SFFI_CALL(cif,d_d12i,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif

#if (!defined(DGTEST)) || DGTEST == 43
  dr = d_d13i(d1,d2,d3,d4,d5,d6,d7,d8,d9,d10,d11,d12,d13,i9);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_sint };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &d1, &d2, &d3, &d4, &d5, &d6, &d7, &d8, &d9, &d10, &d11, &d12, &d13, &i9 };
      SFFI_CALL(cif,d_d13i,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif  
  return;
}
void
  small_structure_return_tests (void)
{
#if (!defined(DGTEST)) || DGTEST == 44
  {
    Size1 r = S1_v();
    fprintf(out,"->{%c}\n",r.x1);
    fflush(out);
    memset(&r,0,sizeof(r)); clear_traces();
    {
      sffi_type* sffi_type_Size1_elements[] = { &sffi_type_char, NULL };
      sffi_type sffi_type_Size1;
      sffi_type_Size1.type = SFFI_TYPE_STRUCT;
      sffi_type_Size1.size = sizeof(Size1);
      sffi_type_Size1.alignment = alignof_slot(Size1);
      sffi_type_Size1.elements = sffi_type_Size1_elements;
      sffi_cif cif;
      SFFI_PREP_CIF_NOARGS(cif,sffi_type_Size1);
      {
        SFFI_CALL(cif,S1_v,NULL,&r);
      }
    }
    fprintf(out,"->{%c}\n",r.x1);
    fflush(out);
  }
#endif
#if (!defined(DGTEST)) || DGTEST == 45
  {
    Size2 r = S2_v();
    fprintf(out,"->{%c%c}\n",r.x1,r.x2);
    fflush(out);
    memset(&r,0,sizeof(r)); clear_traces();
    {
      sffi_type* sffi_type_Size2_elements[] = { &sffi_type_char, &sffi_type_char, NULL };
      sffi_type sffi_type_Size2;
      sffi_type_Size2.type = SFFI_TYPE_STRUCT;
      sffi_type_Size2.size = sizeof(Size2);
      sffi_type_Size2.alignment = alignof_slot(Size2);
      sffi_type_Size2.elements = sffi_type_Size2_elements;
      sffi_cif cif;
      SFFI_PREP_CIF_NOARGS(cif,sffi_type_Size2);
      {
        SFFI_CALL(cif,S2_v,NULL,&r);
      }
    }
    fprintf(out,"->{%c%c}\n",r.x1,r.x2);
    fflush(out);
  }
#endif
#if (!defined(DGTEST)) || DGTEST == 46
  {
    Size3 r = S3_v();
    fprintf(out,"->{%c%c%c}\n",r.x1,r.x2,r.x3);
    fflush(out);
    memset(&r,0,sizeof(r)); clear_traces();
    {
      sffi_type* sffi_type_Size3_elements[] = { &sffi_type_char, &sffi_type_char, &sffi_type_char, NULL };
      sffi_type sffi_type_Size3;
      sffi_type_Size3.type = SFFI_TYPE_STRUCT;
      sffi_type_Size3.size = sizeof(Size3);
      sffi_type_Size3.alignment = alignof_slot(Size3);
      sffi_type_Size3.elements = sffi_type_Size3_elements;
      sffi_cif cif;
      SFFI_PREP_CIF_NOARGS(cif,sffi_type_Size3);
      {
        SFFI_CALL(cif,S3_v,NULL,&r);
      }
    }
    fprintf(out,"->{%c%c%c}\n",r.x1,r.x2,r.x3);
    fflush(out);
  }
#endif
#if (!defined(DGTEST)) || DGTEST == 47
  {
    Size4 r = S4_v();
    fprintf(out,"->{%c%c%c%c}\n",r.x1,r.x2,r.x3,r.x4);
    fflush(out);
    memset(&r,0,sizeof(r)); clear_traces();
    {
      sffi_type* sffi_type_Size4_elements[] = { &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, NULL };
      sffi_type sffi_type_Size4;
      sffi_type_Size4.type = SFFI_TYPE_STRUCT;
      sffi_type_Size4.size = sizeof(Size4);
      sffi_type_Size4.alignment = alignof_slot(Size4);
      sffi_type_Size4.elements = sffi_type_Size4_elements;
      sffi_cif cif;
      SFFI_PREP_CIF_NOARGS(cif,sffi_type_Size4);
      {
        SFFI_CALL(cif,S4_v,NULL,&r);
      }
    }
    fprintf(out,"->{%c%c%c%c}\n",r.x1,r.x2,r.x3,r.x4);
    fflush(out);
  }
#endif
#if (!defined(DGTEST)) || DGTEST == 48
  {
    Size7 r = S7_v();
    fprintf(out,"->{%c%c%c%c%c%c%c}\n",r.x1,r.x2,r.x3,r.x4,r.x5,r.x6,r.x7);
    fflush(out);
    memset(&r,0,sizeof(r)); clear_traces();
    {
      sffi_type* sffi_type_Size7_elements[] = { &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, NULL };
      sffi_type sffi_type_Size7;
      sffi_type_Size7.type = SFFI_TYPE_STRUCT;
      sffi_type_Size7.size = sizeof(Size7);
      sffi_type_Size7.alignment = alignof_slot(Size7);
      sffi_type_Size7.elements = sffi_type_Size7_elements;
      sffi_cif cif;
      SFFI_PREP_CIF_NOARGS(cif,sffi_type_Size7);
      {
        SFFI_CALL(cif,S7_v,NULL,&r);
      }
    }
    fprintf(out,"->{%c%c%c%c%c%c%c}\n",r.x1,r.x2,r.x3,r.x4,r.x5,r.x6,r.x7);
    fflush(out);
  }
#endif
#if (!defined(DGTEST)) || DGTEST == 49
  {
    Size8 r = S8_v();
    fprintf(out,"->{%c%c%c%c%c%c%c%c}\n",r.x1,r.x2,r.x3,r.x4,r.x5,r.x6,r.x7,r.x8);
    fflush(out);
    memset(&r,0,sizeof(r)); clear_traces();
    {
      sffi_type* sffi_type_Size8_elements[] = { &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, NULL };
      sffi_type sffi_type_Size8;
      sffi_type_Size8.type = SFFI_TYPE_STRUCT;
      sffi_type_Size8.size = sizeof(Size8);
      sffi_type_Size8.alignment = alignof_slot(Size8);
      sffi_type_Size8.elements = sffi_type_Size8_elements;
      sffi_cif cif;
      SFFI_PREP_CIF_NOARGS(cif,sffi_type_Size8);
      {
        SFFI_CALL(cif,S8_v,NULL,&r);
      }
    }
    fprintf(out,"->{%c%c%c%c%c%c%c%c}\n",r.x1,r.x2,r.x3,r.x4,r.x5,r.x6,r.x7,r.x8);
    fflush(out);
  }
#endif
#if (!defined(DGTEST)) || DGTEST == 50
  {
    Size12 r = S12_v();
    fprintf(out,"->{%c%c%c%c%c%c%c%c%c%c%c%c}\n",r.x1,r.x2,r.x3,r.x4,r.x5,r.x6,r.x7,r.x8,r.x9,r.x10,r.x11,r.x12);
    fflush(out);
    memset(&r,0,sizeof(r)); clear_traces();
    {
      sffi_type* sffi_type_Size12_elements[] = { &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, NULL };
      sffi_type sffi_type_Size12;
      sffi_type_Size12.type = SFFI_TYPE_STRUCT;
      sffi_type_Size12.size = sizeof(Size12);
      sffi_type_Size12.alignment = alignof_slot(Size12);
      sffi_type_Size12.elements = sffi_type_Size12_elements;
      sffi_cif cif;
      SFFI_PREP_CIF_NOARGS(cif,sffi_type_Size12);
      {
        SFFI_CALL(cif,S12_v,NULL,&r);
      }
    }
    fprintf(out,"->{%c%c%c%c%c%c%c%c%c%c%c%c}\n",r.x1,r.x2,r.x3,r.x4,r.x5,r.x6,r.x7,r.x8,r.x9,r.x10,r.x11,r.x12);
    fflush(out);
  }
#endif
#if (!defined(DGTEST)) || DGTEST == 51  
  {
    Size15 r = S15_v();
    fprintf(out,"->{%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c}\n",r.x1,r.x2,r.x3,r.x4,r.x5,r.x6,r.x7,r.x8,r.x9,r.x10,r.x11,r.x12,r.x13,r.x14,r.x15);
    fflush(out);
    memset(&r,0,sizeof(r)); clear_traces();
    {
      sffi_type* sffi_type_Size15_elements[] = { &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, NULL };
      sffi_type sffi_type_Size15;
      sffi_type_Size15.type = SFFI_TYPE_STRUCT;
      sffi_type_Size15.size = sizeof(Size15);
      sffi_type_Size15.alignment = alignof_slot(Size15);
      sffi_type_Size15.elements = sffi_type_Size15_elements;
      sffi_cif cif;
      SFFI_PREP_CIF_NOARGS(cif,sffi_type_Size15);
      {
        SFFI_CALL(cif,S15_v,NULL,&r);
      }
    }
    fprintf(out,"->{%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c}\n",r.x1,r.x2,r.x3,r.x4,r.x5,r.x6,r.x7,r.x8,r.x9,r.x10,r.x11,r.x12,r.x13,r.x14,r.x15);
    fflush(out);
  }
#endif
#if (!defined(DGTEST)) || DGTEST == 52  
  {
    Size16 r = S16_v();
    fprintf(out,"->{%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c}\n",r.x1,r.x2,r.x3,r.x4,r.x5,r.x6,r.x7,r.x8,r.x9,r.x10,r.x11,r.x12,r.x13,r.x14,r.x15,r.x16);
    fflush(out);
    memset(&r,0,sizeof(r)); clear_traces();
    {
      sffi_type* sffi_type_Size16_elements[] = { &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, &sffi_type_char, NULL };
      sffi_type sffi_type_Size16;
      sffi_type_Size16.type = SFFI_TYPE_STRUCT;
      sffi_type_Size16.size = sizeof(Size16);
      sffi_type_Size16.alignment = alignof_slot(Size16);
      sffi_type_Size16.elements = sffi_type_Size16_elements;
      sffi_cif cif;
      SFFI_PREP_CIF_NOARGS(cif,sffi_type_Size16);
      {
        SFFI_CALL(cif,S16_v,NULL,&r);
      }
    }
    fprintf(out,"->{%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c}\n",r.x1,r.x2,r.x3,r.x4,r.x5,r.x6,r.x7,r.x8,r.x9,r.x10,r.x11,r.x12,r.x13,r.x14,r.x15,r.x16);
    fflush(out);
  }
#endif
}
void
  structure_tests (void)
{
  Int Ir;
  Char Cr;
  Float Fr;
  Double Dr;
  J Jr;
#ifndef SKIP_EXTRA_STRUCTS  
  T Tr;
  X Xr;
#endif  

#if (!defined(DGTEST)) || DGTEST == 53  
  Ir = I_III(I1,I2,I3);
  fprintf(out,"->{%d}\n",Ir.x);
  fflush(out);
  Ir.x = 0; clear_traces();
  {
    sffi_type* sffi_type_Int_elements[] = { &sffi_type_sint, NULL };
    sffi_type sffi_type_Int;
    sffi_type_Int.type = SFFI_TYPE_STRUCT;
    sffi_type_Int.size = sizeof(Int);
    sffi_type_Int.alignment = alignof_slot(Int);
    sffi_type_Int.elements = sffi_type_Int_elements;
    sffi_type* argtypes[] = { &sffi_type_Int, &sffi_type_Int, &sffi_type_Int };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_Int);
    {
       void* args[] = { &I1, &I2, &I3 };
      SFFI_CALL(cif,I_III,args,&Ir);
    }
  }
  fprintf(out,"->{%d}\n",Ir.x);
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 54
  Cr = C_CdC(C1,d2,C3);
  fprintf(out,"->{'%c'}\n",Cr.x);
  fflush(out);
  Cr.x = '\0'; clear_traces();
  {
    sffi_type* sffi_type_Char_elements[] = { &sffi_type_char, NULL };
    sffi_type sffi_type_Char;
    sffi_type_Char.type = SFFI_TYPE_STRUCT;
    sffi_type_Char.size = sizeof(Char);
    sffi_type_Char.alignment = alignof_slot(Char);
    sffi_type_Char.elements = sffi_type_Char_elements;
    sffi_type* argtypes[] = { &sffi_type_Char, &sffi_type_double, &sffi_type_Char };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_Char);
    {
       void* args[] = { &C1, &d2, &C3 };
      SFFI_CALL(cif,C_CdC,args,&Cr);
    }
  }
  fprintf(out,"->{'%c'}\n",Cr.x);
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 55
  Fr = F_Ffd(F1,f2,d3);
  fprintf(out,"->{%g}\n",Fr.x);
  fflush(out);
  Fr.x = 0.0; clear_traces();
  {
    sffi_type* sffi_type_Float_elements[] = { &sffi_type_float, NULL };
    sffi_type sffi_type_Float;
    sffi_type_Float.type = SFFI_TYPE_STRUCT;
    sffi_type_Float.size = sizeof(Float);
    sffi_type_Float.alignment = alignof_slot(Float);
    sffi_type_Float.elements = sffi_type_Float_elements;
    sffi_type* argtypes[] = { &sffi_type_Float, &sffi_type_float, &sffi_type_double };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_Float);
    {
       void* args[] = { &F1, &f2, &d3 };
      SFFI_CALL(cif,F_Ffd,args,&Fr);
    }
  }
  fprintf(out,"->{%g}\n",Fr.x);
  fflush(out);
#endif  
#if (!defined(DGTEST)) || DGTEST == 56  
  Dr = D_fDd(f1,D2,d3);
  fprintf(out,"->{%g}\n",Dr.x);
  fflush(out);
  Dr.x = 0.0; clear_traces();
  {
    sffi_type* sffi_type_Double_elements[] = { &sffi_type_double, NULL };
    sffi_type sffi_type_Double;
    sffi_type_Double.type = SFFI_TYPE_STRUCT;
    sffi_type_Double.size = sizeof(Double);
    sffi_type_Double.alignment = alignof_slot(Double);
    sffi_type_Double.elements = sffi_type_Double_elements;
    sffi_type* argtypes[] = { &sffi_type_float, &sffi_type_Double, &sffi_type_double };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_Double);
    {
       void* args[] = { &f1, &D2, &d3 };
      SFFI_CALL(cif,D_fDd,args,&Dr);
    }
  }
  fprintf(out,"->{%g}\n",Dr.x);
  fflush(out);
#endif  
#if (!defined(DGTEST)) || DGTEST == 57  
  Dr = D_Dfd(D1,f2,d3);
  fprintf(out,"->{%g}\n",Dr.x);
  fflush(out);
  Dr.x = 0.0; clear_traces();
  {
    sffi_type* sffi_type_Double_elements[] = { &sffi_type_double, NULL };
    sffi_type sffi_type_Double;
    sffi_type_Double.type = SFFI_TYPE_STRUCT;
    sffi_type_Double.size = sizeof(Double);
    sffi_type_Double.alignment = alignof_slot(Double);
    sffi_type_Double.elements = sffi_type_Double_elements;
    sffi_type* argtypes[] = { &sffi_type_Double, &sffi_type_float, &sffi_type_double };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_Double);
    {
       void* args[] = { &D1, &f2, &d3 };
      SFFI_CALL(cif,D_Dfd,args,&Dr);
    }
  }
  fprintf(out,"->{%g}\n",Dr.x);
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 58  
  Jr = J_JiJ(J1,i2,J2);
  fprintf(out,"->{%ld,%ld}\n",Jr.l1,Jr.l2);
  fflush(out);
  Jr.l1 = Jr.l2 = 0; clear_traces();
  {
    sffi_type* sffi_type_J_elements[] = { &sffi_type_slong, &sffi_type_slong, NULL };
    sffi_type sffi_type_J;
    sffi_type_J.type = SFFI_TYPE_STRUCT;
    sffi_type_J.size = sizeof(J);
    sffi_type_J.alignment = alignof_slot(J);
    sffi_type_J.elements = sffi_type_J_elements;
    sffi_type* argtypes[] = { &sffi_type_J, &sffi_type_sint, &sffi_type_J };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_J);
    {
       void* args[] = { &J1, &i2, &J2 };
      SFFI_CALL(cif,J_JiJ,args,&Jr);
    }
  }
  fprintf(out,"->{%ld,%ld}\n",Jr.l1,Jr.l2);
  fflush(out);
#endif
#ifndef SKIP_EXTRA_STRUCTS
#if (!defined(DGTEST)) || DGTEST == 59
  Tr = T_TcT(T1,' ',T2);
  fprintf(out,"->{\"%c%c%c\"}\n",Tr.c[0],Tr.c[1],Tr.c[2]);
  fflush(out);
  Tr.c[0] = Tr.c[1] = Tr.c[2] = 0; clear_traces();
  {
    sffi_type* sffi_type_T_elements[] = { ??, NULL };
    sffi_type sffi_type_T;
    sffi_type_T.type = SFFI_TYPE_STRUCT;
    sffi_type_T.size = sizeof(T);
    sffi_type_T.alignment = alignof_slot(T);
    sffi_type_T.elements = sffi_type_T_elements;
    sffi_type* argtypes[] = { &sffi_type_T, &sffi_type_char, &sffi_type_T };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_T);
    {
      char space = ' ';
       void* args[] = { &T1, &space, &T2 };
      SFFI_CALL(cif,T_TcT,args,&Tr);
    }
  }
  fprintf(out,"->{\"%c%c%c\"}\n",Tr.c[0],Tr.c[1],Tr.c[2]);
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 60
  Xr = X_BcdB(B1,c2,d3,B2);
  fprintf(out,"->{\"%s\",'%c'}\n",Xr.c,Xr.c1);
  fflush(out);
  Xr.c[0]=Xr.c1='\0'; clear_traces();
  {
    sffi_type* sffi_type_X_elements[] = { ??, NULL };
    sffi_type sffi_type_X;
    sffi_type_X.type = SFFI_TYPE_STRUCT;
    sffi_type_X.size = sizeof(X);
    sffi_type_X.alignment = alignof_slot(X);
    sffi_type_X.elements = sffi_type_X_elements;
    sffi_type* argtypes[] = { &sffi_type_X, &sffi_type_char, &sffi_type_double, &sffi_type_X };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_X);
    {
       void* args[] = { &B1, &c2, &d3, &B2 };
      SFFI_CALL(cif,X_BcdB,args,&Xr);
    }
  }
  fprintf(out,"->{\"%s\",'%c'}\n",Xr.c,Xr.c1);
  fflush(out);
#endif
#endif

  return;
}

void
  gpargs_boundary_tests (void)
{
  sffi_type* sffi_type_K_elements[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, NULL };
  sffi_type sffi_type_K;
  sffi_type* sffi_type_L_elements[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, NULL };
  sffi_type sffi_type_L;
  long lr;
  long long llr;
  float fr;
  double dr;

  sffi_type_K.type = SFFI_TYPE_STRUCT;
  sffi_type_K.size = sizeof(K);
  sffi_type_K.alignment = alignof_slot(K);
  sffi_type_K.elements = sffi_type_K_elements;

  sffi_type_L.type = SFFI_TYPE_STRUCT;
  sffi_type_L.size = sizeof(L);
  sffi_type_L.alignment = alignof_slot(L);
  sffi_type_L.elements = sffi_type_L_elements;

#if (!defined(DGTEST)) || DGTEST == 61  
  lr = l_l0K(K1,l9);
  fprintf(out,"->%ld\n",lr);
  fflush(out);
  lr = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_K, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_slong);
    {
       void* args[] = { &K1, &l9 };
      SFFI_CALL(cif,l_l0K,args,&lr);
    }
  }
  fprintf(out,"->%ld\n",lr);
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 62  
  lr = l_l1K(l1,K1,l9);
  fprintf(out,"->%ld\n",lr);
  fflush(out);
  lr = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_slong, &sffi_type_K, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_slong);
    {
       void* args[] = { &l1, &K1, &l9 };
      SFFI_CALL(cif,l_l1K,args,&lr);
    }
  }
  fprintf(out,"->%ld\n",lr);
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 63  
  lr = l_l2K(l1,l2,K1,l9);
  fprintf(out,"->%ld\n",lr);
  fflush(out);
  lr = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_K, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_slong);
    {
       void* args[] = { &l1, &l2, &K1, &l9 };
      SFFI_CALL(cif,l_l2K,args,&lr);
    }
  }
  fprintf(out,"->%ld\n",lr);
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 64  
  lr = l_l3K(l1,l2,l3,K1,l9);
  fprintf(out,"->%ld\n",lr);
  fflush(out);
  lr = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_K, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_slong);
    {
       void* args[] = { &l1, &l2, &l3, &K1, &l9 };
      SFFI_CALL(cif,l_l3K,args,&lr);
    }
  }
  fprintf(out,"->%ld\n",lr);
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 65
  lr = l_l4K(l1,l2,l3,l4,K1,l9);
  fprintf(out,"->%ld\n",lr);
  fflush(out);
  lr = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_K, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_slong);
    {
       void* args[] = { &l1, &l2, &l3, &l4, &K1, &l9 };
      SFFI_CALL(cif,l_l4K,args,&lr);
    }
  }
  fprintf(out,"->%ld\n",lr);
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 66
  lr = l_l5K(l1,l2,l3,l4,l5,K1,l9);
  fprintf(out,"->%ld\n",lr);
  fflush(out);
  lr = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_K, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_slong);
    {
       void* args[] = { &l1, &l2, &l3, &l4, &l5, &K1, &l9 };
      SFFI_CALL(cif,l_l5K,args,&lr);
    }
  }
  fprintf(out,"->%ld\n",lr);
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 67  
  lr = l_l6K(l1,l2,l3,l4,l5,l6,K1,l9);
  fprintf(out,"->%ld\n",lr);
  fflush(out);
  lr = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_K, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_slong);
    {
       void* args[] = { &l1, &l2, &l3, &l4, &l5, &l6, &K1, &l9 };
      SFFI_CALL(cif,l_l6K,args,&lr);
    }
  }
  fprintf(out,"->%ld\n",lr);
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 68  
  fr = f_f17l3L(f1,f2,f3,f4,f5,f6,f7,f8,f9,f10,f11,f12,f13,f14,f15,f16,f17,l6,l7,l8,L1);
  fprintf(out,"->%g\n",fr);
  fflush(out);
  fr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_float, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_L };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_float);
    {
       void* args[] = { &f1, &f2, &f3, &f4, &f5, &f6, &f7, &f8, &f9, &f10, &f11, &f12, &f13, &f14, &f15, &f16, &f17, &l6, &l7, &l8, &L1 };
      SFFI_CALL(cif,f_f17l3L,args,&fr);
    }
  }
  fprintf(out,"->%g\n",fr);
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 69  
  dr = d_d17l3L(d1,d2,d3,d4,d5,d6,d7,d8,d9,d10,d11,d12,d13,d14,d15,d16,d17,l6,l7,l8,L1);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_double, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_L };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &d1, &d2, &d3, &d4, &d5, &d6, &d7, &d8, &d9, &d10, &d11, &d12, &d13, &d14, &d15, &d16, &d17, &l6, &l7, &l8, &L1 };
      SFFI_CALL(cif,d_d17l3L,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 70  
  llr = ll_l2ll(l1,l2,ll1,l9);
  fprintf(out,"->0x%lx%08lx\n",(long)(llr>>32),(long)(llr&0xffffffff));
  fflush(out);
  llr = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_slonglong, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_slonglong);
    {
       void* args[] = { &l1, &l2, &ll1, &l9 };
      SFFI_CALL(cif,ll_l2ll,args,&llr);
    }
  }
  fprintf(out,"->0x%lx%08lx\n",(long)(llr>>32),(long)(llr&0xffffffff));
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 71
  llr = ll_l3ll(l1,l2,l3,ll1,l9);
  fprintf(out,"->0x%lx%08lx\n",(long)(llr>>32),(long)(llr&0xffffffff));
  fflush(out);
  llr = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slonglong, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_slonglong);
    {
       void* args[] = { &l1, &l2, &l3, &ll1, &l9 };
      SFFI_CALL(cif,ll_l3ll,args,&llr);
    }
  }
  fprintf(out,"->0x%lx%08lx\n",(long)(llr>>32),(long)(llr&0xffffffff));
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 72  
  llr = ll_l4ll(l1,l2,l3,l4,ll1,l9);
  fprintf(out,"->0x%lx%08lx\n",(long)(llr>>32),(long)(llr&0xffffffff));
  fflush(out);
  llr = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slonglong, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_slonglong);
    {
       void* args[] = { &l1, &l2, &l3, &l4, &ll1, &l9 };
      SFFI_CALL(cif,ll_l4ll,args,&llr);
    }
  }
  fprintf(out,"->0x%lx%08lx\n",(long)(llr>>32),(long)(llr&0xffffffff));
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 73  
  llr = ll_l5ll(l1,l2,l3,l4,l5,ll1,l9);
  fprintf(out,"->0x%lx%08lx\n",(long)(llr>>32),(long)(llr&0xffffffff));
  fflush(out);
  llr = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slonglong, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_slonglong);
    {
       void* args[] = { &l1, &l2, &l3, &l4, &l5, &ll1, &l9 };
      SFFI_CALL(cif,ll_l5ll,args,&llr);
    }
  }
  fprintf(out,"->0x%lx%08lx\n",(long)(llr>>32),(long)(llr&0xffffffff));
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 74  
  llr = ll_l6ll(l1,l2,l3,l4,l5,l6,ll1,l9);
  fprintf(out,"->0x%lx%08lx\n",(long)(llr>>32),(long)(llr&0xffffffff));
  fflush(out);
  llr = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slonglong, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_slonglong);
    {
       void* args[] = { &l1, &l2, &l3, &l4, &l5, &l6, &ll1, &l9 };
      SFFI_CALL(cif,ll_l6ll,args,&llr);
    }
  }
  fprintf(out,"->0x%lx%08lx\n",(long)(llr>>32),(long)(llr&0xffffffff));
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 75  
  llr = ll_l7ll(l1,l2,l3,l4,l5,l6,l7,ll1,l9);
  fprintf(out,"->0x%lx%08lx\n",(long)(llr>>32),(long)(llr&0xffffffff));
  fflush(out);
  llr = 0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slonglong, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_slonglong);
    {
       void* args[] = { &l1, &l2, &l3, &l4, &l5, &l6, &l7, &ll1, &l9 };
      SFFI_CALL(cif,ll_l7ll,args,&llr);
    }
  }
  fprintf(out,"->0x%lx%08lx\n",(long)(llr>>32),(long)(llr&0xffffffff));
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 76  
  dr = d_l2d(l1,l2,d2,l9);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_double, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &l1, &l2, &d2, &l9 };
      SFFI_CALL(cif,d_l2d,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 77  
  dr = d_l3d(l1,l2,l3,d2,l9);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_double, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &l1, &l2, &l3, &d2, &l9 };
      SFFI_CALL(cif,d_l3d,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 78  
  dr = d_l4d(l1,l2,l3,l4,d2,l9);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_double, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &l1, &l2, &l3, &l4, &d2, &l9 };
      SFFI_CALL(cif,d_l4d,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 79  
  dr = d_l5d(l1,l2,l3,l4,l5,d2,l9);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_double, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &l1, &l2, &l3, &l4, &l5, &d2, &l9 };
      SFFI_CALL(cif,d_l5d,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 80  
  dr = d_l6d(l1,l2,l3,l4,l5,l6,d2,l9);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_double, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &l1, &l2, &l3, &l4, &l5, &l6, &d2, &l9 };
      SFFI_CALL(cif,d_l6d,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif
#if (!defined(DGTEST)) || DGTEST == 81  
  dr = d_l7d(l1,l2,l3,l4,l5,l6,l7,d2,l9);
  fprintf(out,"->%g\n",dr);
  fflush(out);
  dr = 0.0; clear_traces();
  {
    sffi_type* argtypes[] = { &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_slong, &sffi_type_double, &sffi_type_slong };
    sffi_cif cif;
    SFFI_PREP_CIF(cif,argtypes,sffi_type_double);
    {
       void* args[] = { &l1, &l2, &l3, &l4, &l5, &l6, &l7, &d2, &l9 };
      SFFI_CALL(cif,d_l7d,args,&dr);
    }
  }
  fprintf(out,"->%g\n",dr);
  fflush(out);
#endif
  return;
}

int
  main (void)
{
  sffi_type_char = (char)(-1) < 0 ? sffi_type_schar : sffi_type_uchar;
  out = stdout;

  void_tests();
  int_tests();
  float_tests();
  double_tests();
  pointer_tests();
  mixed_number_tests();
  small_structure_return_tests();
  structure_tests();
  gpargs_boundary_tests();

  exit(0);
}
