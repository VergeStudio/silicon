#include "sffi.h"
#include <tramp.h>
#include <stdlib.h>

#ifndef POWERPC64
#include "sffi_common.h"
#include "sffi_powerpc.h"

#define ASM_NEEDS_REGISTERS 6
#define NUM_GPR_ARG_REGISTERS 8
#define NUM_FPR_ARG_REGISTERS 8

#if HAVE_LONG_DOUBLE_VARIANT && SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE

void SFFI_HIDDEN
sffi_prep_types_sysv (sffi_abi abi)
{
  if ((abi & (SFFI_SYSV | SFFI_SYSV_LONG_DOUBLE_128)) == SFFI_SYSV)
    {
      sffi_type_longdouble.size = 8;
      sffi_type_longdouble.alignment = 8;
    }
  else
    {
      sffi_type_longdouble.size = 16;
      sffi_type_longdouble.alignment = 16;
    }
}
#endif

static int
translate_float (int abi, int type)
{
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
  if (type == SFFI_TYPE_LONGDOUBLE
      && (abi & SFFI_SYSV_LONG_DOUBLE_128) == 0)
    type = SFFI_TYPE_DOUBLE;
#endif
  if ((abi & SFFI_SYSV_SOFT_FLOAT) != 0)
    {
      if (type == SFFI_TYPE_FLOAT)
	type = SFFI_TYPE_UINT32;
      else if (type == SFFI_TYPE_DOUBLE)
	type = SFFI_TYPE_UINT64;
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
      else if (type == SFFI_TYPE_LONGDOUBLE)
	type = SFFI_TYPE_UINT128;
    }
  else if ((abi & SFFI_SYSV_IBM_LONG_DOUBLE) == 0)
    {
      if (type == SFFI_TYPE_LONGDOUBLE)
	type = SFFI_TYPE_STRUCT;
#endif
    }
  return type;
}

static sffi_status
sffi_prep_cif_sysv_core (sffi_cif *cif)
{
  sffi_type **ptr;
  unsigned bytes;
  unsigned i, fpr_count = 0, gpr_count = 0, stack_count = 0;
  unsigned flags = cif->flags;
  unsigned struct_copy_size = 0;
  unsigned type = cif->rtype->type;
  unsigned size = cif->rtype->size;

  bytes = (2 + ASM_NEEDS_REGISTERS) * sizeof (int);

  bytes += NUM_GPR_ARG_REGISTERS * sizeof (int);

  type = translate_float (cif->abi, type);

  switch (type)
    {
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
    case SFFI_TYPE_LONGDOUBLE:
      flags |= FLAG_RETURNS_128BITS;

#endif
    case SFFI_TYPE_DOUBLE:
      flags |= FLAG_RETURNS_64BITS;

    case SFFI_TYPE_FLOAT:
      flags |= FLAG_RETURNS_FP;
#ifdef __NO_FPRS__
      return SFFI_BAD_ABI;
#endif
      break;

    case SFFI_TYPE_UINT128:
      flags |= FLAG_RETURNS_128BITS;

    case SFFI_TYPE_UINT64:
    case SFFI_TYPE_SINT64:
      flags |= FLAG_RETURNS_64BITS;
      break;

    case SFFI_TYPE_STRUCT:

      if ((cif->abi & SFFI_SYSV_STRUCT_RET) != 0 && size <= 8)
	{
	  flags |= FLAG_RETURNS_SMST;
	  break;
	}
      gpr_count++;
      flags |= FLAG_RETVAL_REFERENCE;

    case SFFI_TYPE_VOID:
      flags |= FLAG_RETURNS_NOTHING;
      break;

    default:

      break;
    }

  for (ptr = cif->arg_types, i = cif->nargs; i > 0; i--, ptr++)
    {
      unsigned short typenum = (*ptr)->type;

      typenum = translate_float (cif->abi, typenum);

      switch (typenum)
	{
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
	case SFFI_TYPE_LONGDOUBLE:
	  if (fpr_count >= NUM_FPR_ARG_REGISTERS - 1)
	    {
	      fpr_count = NUM_FPR_ARG_REGISTERS;

	      stack_count += stack_count & 1;
	      stack_count += 4;
	    }
	  else
	    fpr_count += 2;
#ifdef __NO_FPRS__
	  return SFFI_BAD_ABI;
#endif
	  break;
#endif

	case SFFI_TYPE_DOUBLE:
	  if (fpr_count >= NUM_FPR_ARG_REGISTERS)
	    {

	      stack_count += stack_count & 1;
	      stack_count += 2;
	    }
	  else
	    fpr_count += 1;
#ifdef __NO_FPRS__
	  return SFFI_BAD_ABI;
#endif
	  break;

	case SFFI_TYPE_FLOAT:
	  if (fpr_count >= NUM_FPR_ARG_REGISTERS)

	    stack_count += 1;
	  else
	    fpr_count += 1;
#ifdef __NO_FPRS__
	  return SFFI_BAD_ABI;
#endif
	  break;

	case SFFI_TYPE_UINT128:

	  if (gpr_count >= NUM_GPR_ARG_REGISTERS - 3)
	    gpr_count = NUM_GPR_ARG_REGISTERS;
	  if (gpr_count >= NUM_GPR_ARG_REGISTERS)
	    stack_count += 4;
	  else
	    gpr_count += 4;
	  break;

	case SFFI_TYPE_UINT64:
	case SFFI_TYPE_SINT64:

	  gpr_count += gpr_count & 1;
	  if (gpr_count >= NUM_GPR_ARG_REGISTERS)
	    {
	      stack_count += stack_count & 1;
	      stack_count += 2;
	    }
	  else
	    gpr_count += 2;
	  break;

	case SFFI_TYPE_STRUCT:

	  struct_copy_size += ((*ptr)->size + 15) & ~0xF;

	case SFFI_TYPE_POINTER:
	case SFFI_TYPE_INT:
	case SFFI_TYPE_UINT32:
	case SFFI_TYPE_SINT32:
	case SFFI_TYPE_UINT16:
	case SFFI_TYPE_SINT16:
	case SFFI_TYPE_UINT8:
	case SFFI_TYPE_SINT8:

	  if (gpr_count >= NUM_GPR_ARG_REGISTERS)
	    stack_count += 1;
	  else
	    gpr_count += 1;
	  break;

	default:
	  SFFI_ASSERT (0);
	}
    }

  if (fpr_count != 0)
    flags |= FLAG_FP_ARGUMENTS;
  if (gpr_count > 4)
    flags |= FLAG_4_GPR_ARGUMENTS;
  if (struct_copy_size != 0)
    flags |= FLAG_ARG_NEEDS_COPY;

  if (fpr_count != 0)
    bytes += NUM_FPR_ARG_REGISTERS * sizeof (double);

  bytes += stack_count * sizeof (int);

  bytes = (bytes + 15) & ~0xF;

  bytes += struct_copy_size;

  cif->flags = flags;
  cif->bytes = bytes;

  return SFFI_OK;
}

sffi_status SFFI_HIDDEN
sffi_prep_cif_sysv (sffi_cif *cif)
{
  if ((cif->abi & SFFI_SYSV) == 0)
    {

      cif->flags |= FLAG_COMPAT;
      switch (cif->abi)
	{
	default:
	  return SFFI_BAD_ABI;

	case SFFI_COMPAT_SYSV:
	  cif->abi = SFFI_SYSV | SFFI_SYSV_STRUCT_RET | SFFI_SYSV_LONG_DOUBLE_128;
	  break;

	case SFFI_COMPAT_GCC_SYSV:
	  cif->abi = SFFI_SYSV | SFFI_SYSV_LONG_DOUBLE_128;
	  break;

	case SFFI_COMPAT_LINUX:
	  cif->abi = (SFFI_SYSV | SFFI_SYSV_IBM_LONG_DOUBLE
		      | SFFI_SYSV_LONG_DOUBLE_128);
	  break;

	case SFFI_COMPAT_LINUX_SOFT_FLOAT:
	  cif->abi = (SFFI_SYSV | SFFI_SYSV_SOFT_FLOAT | SFFI_SYSV_IBM_LONG_DOUBLE
		      | SFFI_SYSV_LONG_DOUBLE_128);
	  break;
	}
    }
  return sffi_prep_cif_sysv_core (cif);
}

void SFFI_HIDDEN
sffi_prep_args_SYSV (extended_cif *ecif, unsigned *const stack)
{
  const unsigned bytes = ecif->cif->bytes;
  const unsigned flags = ecif->cif->flags;

  typedef union
  {
    char *c;
    unsigned *u;
    long long *ll;
    float *f;
    double *d;
  } valp;

  valp stacktop;

  valp gpr_base;
  valp gpr_end;

#ifndef __NO_FPRS__

  valp fpr_base;
  valp fpr_end;
#endif

  valp copy_space;

  valp next_arg;

  int i;
  sffi_type **ptr;
#ifndef __NO_FPRS__
  double double_tmp;
#endif
  union
  {
    void **v;
    char **c;
    signed char **sc;
    unsigned char **uc;
    signed short **ss;
    unsigned short **us;
    unsigned int **ui;
    long long **ll;
    float **f;
    double **d;
  } p_argv;
  size_t struct_copy_size;
  unsigned gprvalue;

  stacktop.c = (char *) stack + bytes;
  gpr_end.u = stacktop.u - ASM_NEEDS_REGISTERS;
  gpr_base.u = gpr_end.u - NUM_GPR_ARG_REGISTERS;
#ifndef __NO_FPRS__
  fpr_end.d = gpr_base.d;
  fpr_base.d = fpr_end.d - NUM_FPR_ARG_REGISTERS;
  copy_space.c = ((flags & FLAG_FP_ARGUMENTS) ? fpr_base.c : gpr_base.c);
#else
  copy_space.c = gpr_base.c;
#endif
  next_arg.u = stack + 2;

  SFFI_ASSERT (((unsigned long) (char *) stack & 0xF) == 0);
  SFFI_ASSERT (((unsigned long) copy_space.c & 0xF) == 0);
  SFFI_ASSERT (((unsigned long) stacktop.c & 0xF) == 0);
  SFFI_ASSERT ((bytes & 0xF) == 0);
  SFFI_ASSERT (copy_space.c >= next_arg.c);

  if (flags & FLAG_RETVAL_REFERENCE)
    *gpr_base.u++ = (unsigned) (char *) ecif->rvalue;

  p_argv.v = ecif->avalue;
  for (ptr = ecif->cif->arg_types, i = ecif->cif->nargs;
       i > 0;
       i--, ptr++, p_argv.v++)
    {
      unsigned int typenum = (*ptr)->type;

      typenum = translate_float (ecif->cif->abi, typenum);

      switch (typenum)
	{
#ifndef __NO_FPRS__
# if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
	case SFFI_TYPE_LONGDOUBLE:
	  double_tmp = (*p_argv.d)[0];

	  if (fpr_base.d >= fpr_end.d - 1)
	    {
	      fpr_base.d = fpr_end.d;
	      if (((next_arg.u - stack) & 1) != 0)
		next_arg.u += 1;
	      *next_arg.d = double_tmp;
	      next_arg.u += 2;
	      double_tmp = (*p_argv.d)[1];
	      *next_arg.d = double_tmp;
	      next_arg.u += 2;
	    }
	  else
	    {
	      *fpr_base.d++ = double_tmp;
	      double_tmp = (*p_argv.d)[1];
	      *fpr_base.d++ = double_tmp;
	    }
	  SFFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	  break;
# endif
	case SFFI_TYPE_DOUBLE:
	  double_tmp = **p_argv.d;

	  if (fpr_base.d >= fpr_end.d)
	    {
	      if (((next_arg.u - stack) & 1) != 0)
		next_arg.u += 1;
	      *next_arg.d = double_tmp;
	      next_arg.u += 2;
	    }
	  else
	    *fpr_base.d++ = double_tmp;
	  SFFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	  break;

	case SFFI_TYPE_FLOAT:
	  double_tmp = **p_argv.f;
	  if (fpr_base.d >= fpr_end.d)
	    {
	      *next_arg.f = (float) double_tmp;
	      next_arg.u += 1;
	    }
	  else
	    *fpr_base.d++ = double_tmp;
	  SFFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	  break;
#endif 

	case SFFI_TYPE_UINT128:

	  if (gpr_base.u >= gpr_end.u - 3)
	    {
	      unsigned int ii;
	      gpr_base.u = gpr_end.u;
	      for (ii = 0; ii < 4; ii++)
		{
		  unsigned int int_tmp = (*p_argv.ui)[ii];
		  *next_arg.u++ = int_tmp;
		}
	    }
	  else
	    {
	      unsigned int ii;
	      for (ii = 0; ii < 4; ii++)
		{
		  unsigned int int_tmp = (*p_argv.ui)[ii];
		  *gpr_base.u++ = int_tmp;
		}
	    }
	  break;

	case SFFI_TYPE_UINT64:
	case SFFI_TYPE_SINT64:
	  if (gpr_base.u >= gpr_end.u - 1)
	    {
	      gpr_base.u = gpr_end.u;
	      if (((next_arg.u - stack) & 1) != 0)
		next_arg.u++;
	      *next_arg.ll = **p_argv.ll;
	      next_arg.u += 2;
	    }
	  else
	    {

	      if (((gpr_end.u - gpr_base.u) & 1) != 0)
		gpr_base.u++;
	      *gpr_base.ll++ = **p_argv.ll;
	    }
	  break;

	case SFFI_TYPE_STRUCT:
	  struct_copy_size = ((*ptr)->size + 15) & ~0xF;
	  copy_space.c -= struct_copy_size;
	  memcpy (copy_space.c, *p_argv.c, (*ptr)->size);

	  gprvalue = (unsigned long) copy_space.c;

	  SFFI_ASSERT (copy_space.c > next_arg.c);
	  SFFI_ASSERT (flags & FLAG_ARG_NEEDS_COPY);
	  goto putgpr;

	case SFFI_TYPE_UINT8:
	  gprvalue = **p_argv.uc;
	  goto putgpr;
	case SFFI_TYPE_SINT8:
	  gprvalue = **p_argv.sc;
	  goto putgpr;
	case SFFI_TYPE_UINT16:
	  gprvalue = **p_argv.us;
	  goto putgpr;
	case SFFI_TYPE_SINT16:
	  gprvalue = **p_argv.ss;
	  goto putgpr;

	case SFFI_TYPE_INT:
	case SFFI_TYPE_UINT32:
	case SFFI_TYPE_SINT32:
	case SFFI_TYPE_POINTER:

	  gprvalue = **p_argv.ui;

	putgpr:
	  if (gpr_base.u >= gpr_end.u)
	    *next_arg.u++ = gprvalue;
	  else
	    *gpr_base.u++ = gprvalue;
	  break;
	}
    }

  SFFI_ASSERT (copy_space.c >= next_arg.c);
  SFFI_ASSERT (gpr_base.u <= gpr_end.u);
#ifndef __NO_FPRS__
  SFFI_ASSERT (fpr_base.u <= fpr_end.u);
#endif
  SFFI_ASSERT (((flags & FLAG_4_GPR_ARGUMENTS) != 0)
	      == (gpr_end.u - gpr_base.u < 4));
}

#define MIN_CACHE_LINE_SIZE 8

static void
flush_icache (char *wraddr, char *xaddr, int size)
{
  int i;
  for (i = 0; i < size; i += MIN_CACHE_LINE_SIZE)
    __asm__ volatile ("icbi 0,%0;" "dcbf 0,%1;"
		      : : "r" (xaddr + i), "r" (wraddr + i) : "memory");
  __asm__ volatile ("icbi 0,%0;" "dcbf 0,%1;" "sync;" "isync;"
		    : : "r"(xaddr + size - 1), "r"(wraddr + size - 1)
		    : "memory");
}

sffi_status SFFI_HIDDEN
sffi_prep_closure_loc_sysv (sffi_closure *closure,
			   sffi_cif *cif,
			   void (*fun) (sffi_cif *, void *, void **, void *),
			   void *user_data,
			   void *codeloc)
{
  if (cif->abi < SFFI_SYSV || cif->abi >= SFFI_LAST_ABI)
    return SFFI_BAD_ABI;

#ifdef SFFI_EXEC_STATIC_TRAMP
  if (sffi_tramp_is_present(closure))
    {

      void (*dest)(void) = sffi_closure_SYSV;
      sffi_tramp_set_parms (closure->ftramp, dest, closure);
    }
  else
#endif
    {
      unsigned int *tramp = (unsigned int *) &closure->tramp[0];
      tramp[0] = 0x7c0802a6;  
      tramp[1] = 0x429f0005;  
      tramp[2] = 0x7d6802a6;  
      tramp[3] = 0x7c0803a6;  
      tramp[4] = 0x800b0018;  
      tramp[5] = 0x816b001c;  
      tramp[6] = 0x7c0903a6;  
      tramp[7] = 0x4e800420;  
      *(void **) &tramp[8] = (void *) sffi_closure_SYSV; 
      *(void **) &tramp[9] = codeloc;			

      flush_icache ((char *)tramp, (char *)codeloc, 8 * 4);
    }

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  return SFFI_OK;
}

int
sffi_closure_helper_SYSV (sffi_cif *cif,
			 void (*fun) (sffi_cif *, void *, void **, void *),
			 void *user_data,
			 void *rvalue,
			 unsigned long *pgr,
			 sffi_dblfl *pfr,
			 unsigned long *pst)
{

  void **          avalue;
  sffi_type **      arg_types;
  long             i, avn;
#ifndef __NO_FPRS__
  long             nf = 0;   
#endif
  long             ng = 0;   

  unsigned       size     = cif->rtype->size;
  unsigned short rtypenum = cif->rtype->type;

  avalue = alloca (cif->nargs * sizeof (void *));

  rtypenum = translate_float (cif->abi, rtypenum);

  if (rtypenum == SFFI_TYPE_STRUCT
      && !((cif->abi & SFFI_SYSV_STRUCT_RET) != 0 && size <= 8))
    {
      rvalue = (void *) *pgr;
      ng++;
      pgr++;
    }

  i = 0;
  avn = cif->nargs;
  arg_types = cif->arg_types;

  while (i < avn) {
    unsigned short typenum = arg_types[i]->type;

    typenum = translate_float (cif->abi, typenum);

    switch (typenum)
      {
#ifndef __NO_FPRS__
      case SFFI_TYPE_FLOAT:

	if (nf < NUM_FPR_ARG_REGISTERS)
	  {

	    double temp = pfr->d;
	    pfr->f = (float) temp;
	    avalue[i] = pfr;
	    nf++;
	    pfr++;
	  }
	else
	  {
	    avalue[i] = pst;
	    pst += 1;
	  }
	break;

      case SFFI_TYPE_DOUBLE:
	if (nf < NUM_FPR_ARG_REGISTERS)
	  {
	    avalue[i] = pfr;
	    nf++;
	    pfr++;
	  }
	else
	  {
	    if (((long) pst) & 4)
	      pst++;
	    avalue[i] = pst;
	    pst += 2;
	  }
	break;

# if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
      case SFFI_TYPE_LONGDOUBLE:
	if (nf < NUM_FPR_ARG_REGISTERS - 1)
	  {
	    avalue[i] = pfr;
	    pfr += 2;
	    nf += 2;
	  }
	else
	  {
	    if (((long) pst) & 4)
	      pst++;
	    avalue[i] = pst;
	    pst += 4;
	    nf = 8;
	  }
	break;
# endif
#endif

      case SFFI_TYPE_UINT128:

	if (ng < NUM_GPR_ARG_REGISTERS - 3)
	  {
	    avalue[i] = pgr;
	    pgr += 4;
	    ng += 4;
	  }
	else
	  {
	    avalue[i] = pst;
	    pst += 4;
	    ng = 8+4;
	  }
	break;

      case SFFI_TYPE_SINT8:
      case SFFI_TYPE_UINT8:
#ifndef __LITTLE_ENDIAN__
	if (ng < NUM_GPR_ARG_REGISTERS)
	  {
	    avalue[i] = (char *) pgr + 3;
	    ng++;
	    pgr++;
	  }
	else
	  {
	    avalue[i] = (char *) pst + 3;
	    pst++;
	  }
	break;
#endif

      case SFFI_TYPE_SINT16:
      case SFFI_TYPE_UINT16:
#ifndef __LITTLE_ENDIAN__
	if (ng < NUM_GPR_ARG_REGISTERS)
	  {
	    avalue[i] = (char *) pgr + 2;
	    ng++;
	    pgr++;
	  }
	else
	  {
	    avalue[i] = (char *) pst + 2;
	    pst++;
	  }
	break;
#endif

      case SFFI_TYPE_SINT32:
      case SFFI_TYPE_UINT32:
      case SFFI_TYPE_POINTER:
	if (ng < NUM_GPR_ARG_REGISTERS)
	  {
	    avalue[i] = pgr;
	    ng++;
	    pgr++;
	  }
	else
	  {
	    avalue[i] = pst;
	    pst++;
	  }
	break;

      case SFFI_TYPE_STRUCT:

	if (ng < NUM_GPR_ARG_REGISTERS)
	  {
	    avalue[i] = (void *) *pgr;
	    ng++;
	    pgr++;
	  }
	else
	  {
	    avalue[i] = (void *) *pst;
	    pst++;
	  }
	break;

      case SFFI_TYPE_SINT64:
      case SFFI_TYPE_UINT64:

	if (ng < NUM_GPR_ARG_REGISTERS - 1)
	  {
	    if (ng & 1)
	      {

		ng++;
		pgr++;
	      }
	    avalue[i] = pgr;
	    ng += 2;
	    pgr += 2;
	  }
	else
	  {
	    if (((long) pst) & 4)
	      pst++;
	    avalue[i] = pst;
	    pst += 2;
	    ng = NUM_GPR_ARG_REGISTERS;
	  }
	break;

      default:
	SFFI_ASSERT (0);
      }

    i++;
  }

  (*fun) (cif, rvalue, avalue, user_data);

  switch (rtypenum)
    {
    case SFFI_TYPE_VOID:
      return PPC_LD_NONE;
    case SFFI_TYPE_FLOAT:
      return PPC_LD_F32;
    case SFFI_TYPE_DOUBLE:
      return PPC_LD_F64;
#if SFFI_TYPE_DOUBLE != SFFI_TYPE_LONGDOUBLE
    case SFFI_TYPE_LONGDOUBLE:
      return PPC_LD_F128;
#endif
    case SFFI_TYPE_UINT8:
      return PPC_LD_U8;
    case SFFI_TYPE_SINT8:
      return PPC_LD_S8;
    case SFFI_TYPE_UINT16:
      return PPC_LD_U16;
    case SFFI_TYPE_SINT16:
      return PPC_LD_S16;
    case SFFI_TYPE_UINT32:
      return PPC_LD_U32;
    case SFFI_TYPE_INT:
    case SFFI_TYPE_SINT32:
      return PPC_LD_S32;
    case SFFI_TYPE_POINTER:
      return PPC_LD_PTR;
    case SFFI_TYPE_UINT64:
    case SFFI_TYPE_SINT64:
      return PPC_LD_I64;
    case SFFI_TYPE_UINT128:
      return PPC32_LD_R3R6;
    case SFFI_TYPE_STRUCT:
      if (cif->abi & SFFI_SYSV_STRUCT_RET)
	switch (size)
	  {
	  case 1:
	    return PPC_LD_U8;
	  case 2:
	    return PPC_LD_U16;
	  case 3:
	    return PPC32_SYSV_LD_STRUCT_3;
	  case 4:
	    return PPC_LD_U32;
	  case 5:
	    return PPC32_SYSV_LD_STRUCT_5;
	  case 6:
	    return PPC32_SYSV_LD_STRUCT_6;
	  case 7:
	    return PPC32_SYSV_LD_STRUCT_7;
	  case 8:
	    return PPC_LD_I64;
	  }
      return PPC_LD_NONE;
    default:
      abort();
    }
}
#endif
