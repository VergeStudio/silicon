#include "sffi.h"
#include <tramp.h>
#include <stdlib.h>

#ifdef POWERPC64
#include "sffi_common.h"
#include "sffi_powerpc.h"

enum {
  NUM_GPR_ARG_REGISTERS64 = 8,
  NUM_FPR_ARG_REGISTERS64 = 13,
  NUM_VEC_ARG_REGISTERS64 = 12,
};
enum { ASM_NEEDS_REGISTERS64 = 4 };

#if HAVE_LONG_DOUBLE_VARIANT && SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE

void SFFI_HIDDEN
sffi_prep_types_linux64 (sffi_abi abi)
{
  if ((abi & (SFFI_LINUX | SFFI_LINUX_LONG_DOUBLE_128)) == SFFI_LINUX)
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

static unsigned int
discover_homogeneous_aggregate (sffi_abi abi,
                                const sffi_type *t,
                                unsigned int *elnum)
{
  switch (t->type)
    {
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
    case SFFI_TYPE_LONGDOUBLE:

      if ((abi & SFFI_LINUX_LONG_DOUBLE_128) == 0)
        {
          *elnum = 1;
          return SFFI_TYPE_DOUBLE;
        }

      else if ((abi & SFFI_LINUX_LONG_DOUBLE_IEEE128) == 0)
        {
          *elnum = 2;
          return SFFI_TYPE_LONGDOUBLE;
        }

#endif
    case SFFI_TYPE_FLOAT:
    case SFFI_TYPE_DOUBLE:
      *elnum = 1;
      return (int) t->type;

#ifdef SFFI_TARGET_HAS_COMPLEX_TYPE
    case SFFI_TYPE_COMPLEX:

      {
	unsigned int inner_elnum = 0;
	unsigned int inner
	  = discover_homogeneous_aggregate (abi, t->elements[0], &inner_elnum);
	if (inner == SFFI_TYPE_FLOAT || inner == SFFI_TYPE_DOUBLE)
	  {
	    *elnum = 2 * inner_elnum;
	    return inner;
	  }
	return 0;
      }
#endif

    case SFFI_TYPE_STRUCT:;
      {
	unsigned int base_elt = 0, total_elnum = 0;
	sffi_type **el = t->elements;
	while (*el)
	  {
	    unsigned int el_elt, el_elnum = 0;
	    el_elt = discover_homogeneous_aggregate (abi, *el, &el_elnum);
	    if (el_elt == 0
		|| (base_elt && base_elt != el_elt))
	      return 0;
	    base_elt = el_elt;
	    total_elnum += el_elnum;
#if _CALL_ELF == 2
	    if (total_elnum > 8)
	      return 0;
#else
	    if (total_elnum > 1)
	      return 0;
#endif
	    el++;
	  }
	*elnum = total_elnum;
	return base_elt;
      }

    default:
      return 0;
    }
}

static sffi_status
sffi_prep_cif_linux64_core (sffi_cif *cif)
{
  sffi_type **ptr;
  unsigned bytes;
  unsigned i, fparg_count = 0, intarg_count = 0, vecarg_count = 0;
  unsigned flags = cif->flags;
  unsigned elt, elnum, rtype;

#if SFFI_TYPE_LONGDOUBLE == SFFI_TYPE_DOUBLE

  if ((cif->abi & SFFI_LINUX_LONG_DOUBLE_128) != 0 ||
      (cif->abi & SFFI_LINUX_LONG_DOUBLE_IEEE128) != 0)
    return SFFI_BAD_ABI;
#elif !defined(__VEC__)

  if ((cif->abi & SFFI_LINUX_LONG_DOUBLE_IEEE128) != 0)
    return SFFI_BAD_ABI;
#else

  if ((cif->abi & SFFI_LINUX_LONG_DOUBLE_128) == 0 &&
      (cif->abi & SFFI_LINUX_LONG_DOUBLE_IEEE128) != 0)
    return SFFI_BAD_ABI;
#endif

#if _CALL_ELF == 2

  bytes = (4 + ASM_NEEDS_REGISTERS64) * sizeof (long);

  bytes += NUM_GPR_ARG_REGISTERS64 * sizeof (long);
#else

  bytes = (6 + ASM_NEEDS_REGISTERS64) * sizeof (long);

  bytes += 2 * NUM_GPR_ARG_REGISTERS64 * sizeof (long);
#endif

  rtype = cif->rtype->type;
#if _CALL_ELF == 2
homogeneous:
#endif
  switch (rtype)
    {
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
    case SFFI_TYPE_LONGDOUBLE:
      if ((cif->abi & SFFI_LINUX_LONG_DOUBLE_IEEE128) != 0)
        {
          flags |= FLAG_RETURNS_VEC;
          break;
        }
      if ((cif->abi & SFFI_LINUX_LONG_DOUBLE_128) != 0)
	flags |= FLAG_RETURNS_128BITS;

#endif
    case SFFI_TYPE_DOUBLE:
      flags |= FLAG_RETURNS_64BITS;

    case SFFI_TYPE_FLOAT:
      flags |= FLAG_RETURNS_FP;
      break;

    case SFFI_TYPE_UINT64:
    case SFFI_TYPE_SINT64:
    case SFFI_TYPE_POINTER:
      flags |= FLAG_RETURNS_64BITS;
      break;

    case SFFI_TYPE_STRUCT:
#if _CALL_ELF == 2
      elt = discover_homogeneous_aggregate (cif->abi, cif->rtype, &elnum);
      if (elt)
        {
          flags |= FLAG_RETURNS_SMST;
          rtype = elt;
          goto homogeneous;
        }
      if (cif->rtype->size <= 16)
        {
          flags |= FLAG_RETURNS_SMST;
          break;
        }
#endif
      intarg_count++;
      flags |= FLAG_RETVAL_REFERENCE;

    case SFFI_TYPE_VOID:
      flags |= FLAG_RETURNS_NOTHING;
      break;

#ifdef SFFI_TARGET_HAS_COMPLEX_TYPE
    case SFFI_TYPE_COMPLEX:
      rtype = cif->rtype->elements[0]->type;
      switch (rtype)
        {
        case SFFI_TYPE_FLOAT:
        case SFFI_TYPE_DOUBLE:

          flags |= FLAG_RETURNS_SMST;
          goto homogeneous;
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
        case SFFI_TYPE_LONGDOUBLE:

          if ((cif->abi & (SFFI_LINUX_LONG_DOUBLE_128
                           | SFFI_LINUX_LONG_DOUBLE_IEEE128)) != 0)
            return SFFI_BAD_TYPEDEF;
          flags |= FLAG_RETURNS_SMST;
          rtype = SFFI_TYPE_DOUBLE;
          goto homogeneous;
#endif
        case SFFI_TYPE_INT:
        case SFFI_TYPE_SINT8:  case SFFI_TYPE_UINT8:
        case SFFI_TYPE_SINT16: case SFFI_TYPE_UINT16:
        case SFFI_TYPE_SINT32: case SFFI_TYPE_UINT32:
        case SFFI_TYPE_SINT64: case SFFI_TYPE_UINT64:
        case SFFI_TYPE_POINTER:

          flags |= FLAG_RETURNS_SMST;
          break;
        default:
          return SFFI_BAD_TYPEDEF;
        }
      break;
#endif

    default:

      break;
    }

  for (ptr = cif->arg_types, i = cif->nargs; i > 0; i--, ptr++)
    {
      unsigned int align;

      switch ((*ptr)->type)
	{
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
	case SFFI_TYPE_LONGDOUBLE:
          if ((cif->abi & SFFI_LINUX_LONG_DOUBLE_IEEE128) != 0)
            {
              vecarg_count++;

              intarg_count = (intarg_count + 3) & ~0x1;
              if (vecarg_count > NUM_VEC_ARG_REGISTERS64)
                flags |= FLAG_ARG_NEEDS_PSAVE;
              break;
            }
	  if ((cif->abi & SFFI_LINUX_LONG_DOUBLE_128) != 0)
	    {
	      fparg_count++;
	      intarg_count++;
	    }

#endif
	case SFFI_TYPE_DOUBLE:
	case SFFI_TYPE_FLOAT:
	  fparg_count++;
	  intarg_count++;
	  if (fparg_count > NUM_FPR_ARG_REGISTERS64)
	    flags |= FLAG_ARG_NEEDS_PSAVE;
	  break;

	case SFFI_TYPE_STRUCT:
	  if ((cif->abi & SFFI_LINUX_STRUCT_ALIGN) != 0)
	    {
	      align = (*ptr)->alignment;
	      if (align > 16)
		align = 16;
	      align = align / 8;
	      if (align > 1)
		intarg_count = SFFI_ALIGN (intarg_count, align);
	    }
	  intarg_count += ((*ptr)->size + 7) / 8;
	  elt = discover_homogeneous_aggregate (cif->abi, *ptr, &elnum);
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
          if (elt == SFFI_TYPE_LONGDOUBLE &&
              (cif->abi & SFFI_LINUX_LONG_DOUBLE_IEEE128) != 0)
            {
              vecarg_count += elnum;
              if (vecarg_count > NUM_VEC_ARG_REGISTERS64)
                flags |= FLAG_ARG_NEEDS_PSAVE;
              break;
            }
	  else
#endif
	  if (elt)
	    {
	      fparg_count += elnum;
	      if (fparg_count > NUM_FPR_ARG_REGISTERS64)
		flags |= FLAG_ARG_NEEDS_PSAVE;
	    }
	  else
	    {
	      if (intarg_count > NUM_GPR_ARG_REGISTERS64)
		flags |= FLAG_ARG_NEEDS_PSAVE;
	    }
	  break;

	case SFFI_TYPE_POINTER:
	case SFFI_TYPE_UINT64:
	case SFFI_TYPE_SINT64:
	case SFFI_TYPE_INT:
	case SFFI_TYPE_UINT32:
	case SFFI_TYPE_SINT32:
	case SFFI_TYPE_UINT16:
	case SFFI_TYPE_SINT16:
	case SFFI_TYPE_UINT8:
	case SFFI_TYPE_SINT8:

	  intarg_count++;
	  if (intarg_count > NUM_GPR_ARG_REGISTERS64)
	    flags |= FLAG_ARG_NEEDS_PSAVE;
	  break;

#ifdef SFFI_TARGET_HAS_COMPLEX_TYPE
	case SFFI_TYPE_COMPLEX:

	  elt = (*ptr)->elements[0]->type;
	  switch (elt)
	    {
	    case SFFI_TYPE_FLOAT:
	    case SFFI_TYPE_DOUBLE:
	      fparg_count += 2;
	      intarg_count += 2;
	      if (fparg_count > NUM_FPR_ARG_REGISTERS64)
		flags |= FLAG_ARG_NEEDS_PSAVE;
	      break;
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
	    case SFFI_TYPE_LONGDOUBLE:
	      if ((cif->abi & (SFFI_LINUX_LONG_DOUBLE_128
			       | SFFI_LINUX_LONG_DOUBLE_IEEE128)) != 0)
		return SFFI_BAD_TYPEDEF;
	      fparg_count += 2;
	      intarg_count += 2;
	      if (fparg_count > NUM_FPR_ARG_REGISTERS64)
		flags |= FLAG_ARG_NEEDS_PSAVE;
	      break;
#endif
	    case SFFI_TYPE_INT:
	    case SFFI_TYPE_SINT8:  case SFFI_TYPE_UINT8:
	    case SFFI_TYPE_SINT16: case SFFI_TYPE_UINT16:
	    case SFFI_TYPE_SINT32: case SFFI_TYPE_UINT32:
	    case SFFI_TYPE_SINT64: case SFFI_TYPE_UINT64:
	    case SFFI_TYPE_POINTER:
	      intarg_count += 2;
	      break;
	    default:
	      return SFFI_BAD_TYPEDEF;
	    }
	  if (intarg_count > NUM_GPR_ARG_REGISTERS64)
	    flags |= FLAG_ARG_NEEDS_PSAVE;
	  break;
#endif

	default:
	  SFFI_ASSERT (0);
	}
    }

  if (fparg_count != 0)
    flags |= FLAG_FP_ARGUMENTS;
  if (intarg_count > 4)
    flags |= FLAG_4_GPR_ARGUMENTS;
  if (vecarg_count != 0)
    flags |= FLAG_VEC_ARGUMENTS;

  if (fparg_count != 0)
    bytes += NUM_FPR_ARG_REGISTERS64 * sizeof (double);

  if (vecarg_count != 0) {
    bytes = (bytes + 15) & ~0xF;
    bytes += NUM_VEC_ARG_REGISTERS64 * sizeof (float128);
  }

#if _CALL_ELF == 2
  if ((flags & FLAG_ARG_NEEDS_PSAVE) != 0)
    bytes += intarg_count * sizeof (long);
#else
  if (intarg_count > NUM_GPR_ARG_REGISTERS64)
    bytes += (intarg_count - NUM_GPR_ARG_REGISTERS64) * sizeof (long);
#endif

  bytes = (bytes + 15) & ~0xF;

  cif->flags = flags;
  cif->bytes = bytes;

  return SFFI_OK;
}

sffi_status SFFI_HIDDEN
sffi_prep_cif_linux64 (sffi_cif *cif)
{
  if ((cif->abi & SFFI_LINUX) != 0)
    cif->nfixedargs = cif->nargs;
#if _CALL_ELF != 2
  else if (cif->abi == SFFI_COMPAT_LINUX64)
    {

      cif->flags |= FLAG_COMPAT;

      cif->abi = SFFI_LINUX | SFFI_LINUX_LONG_DOUBLE_128;
    }
#endif
  else
    return SFFI_BAD_ABI;
  return sffi_prep_cif_linux64_core (cif);
}

sffi_status SFFI_HIDDEN
sffi_prep_cif_linux64_var (sffi_cif *cif,
			  unsigned int nfixedargs,
			  unsigned int ntotalargs MAYBE_UNUSED)
{
  if ((cif->abi & SFFI_LINUX) != 0)
    cif->nfixedargs = nfixedargs;
#if _CALL_ELF != 2
  else if (cif->abi == SFFI_COMPAT_LINUX64)
    {

      cif->flags |= FLAG_COMPAT;

      cif->abi = SFFI_LINUX | SFFI_LINUX_LONG_DOUBLE_128;
    }
#endif
  else
    return SFFI_BAD_ABI;
#if _CALL_ELF == 2
  cif->flags |= FLAG_ARG_NEEDS_PSAVE;
#endif
  return sffi_prep_cif_linux64_core (cif);
}

void SFFI_HIDDEN
sffi_prep_args64 (extended_cif *ecif, unsigned long *const stack)
{
  const unsigned long bytes = ecif->cif->bytes;
  const unsigned long flags = ecif->cif->flags;

  typedef union
  {
    char *c;
    unsigned long *ul;
    float *f;
    double *d;
    float128 *f128;
    size_t p;
  } valp;

  valp stacktop;

  valp gpr_base;
  valp gpr_end;
  valp rest;
  valp next_arg;

  valp fpr_base;
  unsigned int fparg_count;

  valp vec_base;
  unsigned int vecarg_count;

  unsigned int i, words, nargs, nfixedargs;
  sffi_type **ptr;
  double double_tmp;
  union
  {
    void **v;
    char **c;
    signed char **sc;
    unsigned char **uc;
    signed short **ss;
    unsigned short **us;
    signed int **si;
    unsigned int **ui;
    unsigned long **ul;
    float **f;
    double **d;
    float128 **f128;
  } p_argv;
  unsigned long gprvalue;
  unsigned long align;

  stacktop.c = (char *) stack + bytes;
  gpr_base.ul = stacktop.ul - ASM_NEEDS_REGISTERS64 - NUM_GPR_ARG_REGISTERS64;
  gpr_end.ul = gpr_base.ul + NUM_GPR_ARG_REGISTERS64;
#if _CALL_ELF == 2
  rest.ul = stack + 4 + NUM_GPR_ARG_REGISTERS64;
#else
  rest.ul = stack + 6 + NUM_GPR_ARG_REGISTERS64;
#endif
  fpr_base.d = gpr_base.d - NUM_FPR_ARG_REGISTERS64;
  fparg_count = 0;

  if (ecif->cif->flags & FLAG_FP_ARGUMENTS)
    vec_base.p = fpr_base.p & ~0xF;
  else
    vec_base.p = gpr_base.p;
  vec_base.f128 -= NUM_VEC_ARG_REGISTERS64;
  vecarg_count = 0;
  next_arg.ul = gpr_base.ul;

  SFFI_ASSERT (((unsigned long) (char *) stack & 0xF) == 0);
  SFFI_ASSERT (((unsigned long) stacktop.c & 0xF) == 0);
  SFFI_ASSERT (((unsigned long) gpr_base.c & 0xF) == 0);
  SFFI_ASSERT (((unsigned long) gpr_end.c  & 0xF) == 0);
  SFFI_ASSERT (((unsigned long) vec_base.c & 0xF) == 0);
  SFFI_ASSERT ((bytes & 0xF) == 0);

  if (flags & FLAG_RETVAL_REFERENCE)
    *next_arg.ul++ = (unsigned long) (char *) ecif->rvalue;

  p_argv.v = ecif->avalue;
  nargs = ecif->cif->nargs;
#if _CALL_ELF != 2
  nfixedargs = (unsigned) -1;
  if ((flags & FLAG_COMPAT) == 0)
#endif
    nfixedargs = ecif->cif->nfixedargs;
  for (ptr = ecif->cif->arg_types, i = 0;
       i < nargs;
       i++, ptr++, p_argv.v++)
    {
      unsigned int elt, elnum;

      switch ((*ptr)->type)
	{
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
	case SFFI_TYPE_LONGDOUBLE:
          if ((ecif->cif->abi & SFFI_LINUX_LONG_DOUBLE_IEEE128) != 0)
            {
              next_arg.p = SFFI_ALIGN (next_arg.p, 16);
              if (next_arg.ul == gpr_end.ul)
                next_arg.ul = rest.ul;
              if (vecarg_count < NUM_VEC_ARG_REGISTERS64 && i < nfixedargs)
		memcpy (vec_base.f128++, *p_argv.f128, sizeof (float128));
              else
		memcpy (next_arg.f128, *p_argv.f128, sizeof (float128));
              if (++next_arg.f128 == gpr_end.f128)
                next_arg.f128 = rest.f128;
              vecarg_count++;
              SFFI_ASSERT (__LDBL_MANT_DIG__ == 113);
              SFFI_ASSERT (flags & FLAG_VEC_ARGUMENTS);
              break;
            }
	  if ((ecif->cif->abi & SFFI_LINUX_LONG_DOUBLE_128) != 0)
	    {
	      double_tmp = (*p_argv.d)[0];
	      if (fparg_count < NUM_FPR_ARG_REGISTERS64 && i < nfixedargs)
		{
		  *fpr_base.d++ = double_tmp;
# if _CALL_ELF != 2
		  if ((flags & FLAG_COMPAT) != 0)
		    *next_arg.d = double_tmp;
# endif
		}
	      else
		*next_arg.d = double_tmp;
	      if (++next_arg.ul == gpr_end.ul)
		next_arg.ul = rest.ul;
	      fparg_count++;
	      double_tmp = (*p_argv.d)[1];
	      if (fparg_count < NUM_FPR_ARG_REGISTERS64 && i < nfixedargs)
		{
		  *fpr_base.d++ = double_tmp;
# if _CALL_ELF != 2
		  if ((flags & FLAG_COMPAT) != 0)
		    *next_arg.d = double_tmp;
# endif
		}
	      else
		*next_arg.d = double_tmp;
	      if (++next_arg.ul == gpr_end.ul)
		next_arg.ul = rest.ul;
	      fparg_count++;
	      SFFI_ASSERT (__LDBL_MANT_DIG__ == 106);
	      SFFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	      break;
	    }

#endif
	case SFFI_TYPE_DOUBLE:
#if _CALL_ELF != 2
	do_double:
#endif
	  double_tmp = **p_argv.d;
	  if (fparg_count < NUM_FPR_ARG_REGISTERS64 && i < nfixedargs)
	    {
	      *fpr_base.d++ = double_tmp;
#if _CALL_ELF != 2
	      if ((flags & FLAG_COMPAT) != 0)
		*next_arg.d = double_tmp;
#endif
	    }
	  else
	    *next_arg.d = double_tmp;
	  if (++next_arg.ul == gpr_end.ul)
	    next_arg.ul = rest.ul;
	  fparg_count++;
	  SFFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	  break;

	case SFFI_TYPE_FLOAT:
#if _CALL_ELF != 2
	do_float:
#endif
	  double_tmp = **p_argv.f;
	  if (fparg_count < NUM_FPR_ARG_REGISTERS64 && i < nfixedargs)
	    {
	      *fpr_base.d++ = double_tmp;
#if _CALL_ELF != 2
	      if ((flags & FLAG_COMPAT) != 0)
		{
# ifndef __LITTLE_ENDIAN__
		  next_arg.f[1] = (float) double_tmp;
# else
		  next_arg.f[0] = (float) double_tmp;
# endif
		}
#endif
	    }
	  else
	    {
# ifndef __LITTLE_ENDIAN__
	      next_arg.f[1] = (float) double_tmp;
# else
	      next_arg.f[0] = (float) double_tmp;
# endif
	    }
	  if (++next_arg.ul == gpr_end.ul)
	    next_arg.ul = rest.ul;
	  fparg_count++;
	  SFFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	  break;

#ifdef SFFI_TARGET_HAS_COMPLEX_TYPE
	case SFFI_TYPE_COMPLEX:
	  elt = (*ptr)->elements[0]->type;
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE

	  if (elt == SFFI_TYPE_LONGDOUBLE)
	    elt = SFFI_TYPE_DOUBLE;
#endif
	  if (elt == SFFI_TYPE_FLOAT)
	    {
	      float *cval = (float *) *p_argv.v;
	      unsigned int j;
	      for (j = 0; j < 2; j++)
		{
		  double_tmp = cval[j];
		  if (fparg_count < NUM_FPR_ARG_REGISTERS64 && i < nfixedargs)
		    *fpr_base.d++ = double_tmp;
		  else
		    {
# ifndef __LITTLE_ENDIAN__
		      next_arg.f[1] = (float) double_tmp;
# else
		      next_arg.f[0] = (float) double_tmp;
# endif
		    }
		  if (++next_arg.ul == gpr_end.ul)
		    next_arg.ul = rest.ul;
		  fparg_count++;
		}
	      SFFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	    }
	  else if (elt == SFFI_TYPE_DOUBLE)
	    {
	      double *cval = (double *) *p_argv.v;
	      unsigned int j;
	      for (j = 0; j < 2; j++)
		{
		  double_tmp = cval[j];
		  if (fparg_count < NUM_FPR_ARG_REGISTERS64 && i < nfixedargs)
		    *fpr_base.d++ = double_tmp;
		  else
		    *next_arg.d = double_tmp;
		  if (++next_arg.ul == gpr_end.ul)
		    next_arg.ul = rest.ul;
		  fparg_count++;
		}
	      SFFI_ASSERT (flags & FLAG_FP_ARGUMENTS);
	    }
	  else
	    {

	      char *cval = (char *) *p_argv.v;
	      size_t hsize = (*ptr)->elements[0]->size;
	      unsigned int j;
	      for (j = 0; j < 2; j++)
		{
		  char *half = cval + j * hsize;
		  unsigned long gprvalue;
		  switch (elt)
		    {
		    case SFFI_TYPE_UINT8:
		      gprvalue = *(unsigned char *) half; break;
		    case SFFI_TYPE_SINT8:
		      gprvalue = (unsigned long) (long) *(signed char *) half;
		      break;
		    case SFFI_TYPE_UINT16:
		      gprvalue = *(unsigned short *) half; break;
		    case SFFI_TYPE_SINT16:
		      gprvalue = (unsigned long) (long) *(signed short *) half;
		      break;
		    case SFFI_TYPE_UINT32:
		      gprvalue = *(unsigned int *) half; break;
		    case SFFI_TYPE_INT:
		    case SFFI_TYPE_SINT32:
		      gprvalue = (unsigned long) (long) *(signed int *) half;
		      break;
		    case SFFI_TYPE_SINT64:
		    case SFFI_TYPE_UINT64:
		    case SFFI_TYPE_POINTER:
		    default:
		      gprvalue = *(unsigned long *) half; break;
		    }
		  *next_arg.ul++ = gprvalue;
		  if (next_arg.ul == gpr_end.ul)
		    next_arg.ul = rest.ul;
		}
	    }
	  break;
#endif

	case SFFI_TYPE_STRUCT:
	  if ((ecif->cif->abi & SFFI_LINUX_STRUCT_ALIGN) != 0)
	    {
	      align = (*ptr)->alignment;
	      if (align > 16)
		align = 16;
	      if (align > 1)
                {
                  next_arg.p = SFFI_ALIGN (next_arg.p, align);
                  if (next_arg.ul == gpr_end.ul)
                    next_arg.ul = rest.ul;
                }
	    }
	  elt = discover_homogeneous_aggregate (ecif->cif->abi, *ptr, &elnum);
	  if (elt)
	    {
#if _CALL_ELF == 2
	      union {
		void *v;
		float *f;
		double *d;
		float128 *f128;
	      } arg;

	      arg.v = *p_argv.v;
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
              if (elt == SFFI_TYPE_LONGDOUBLE &&
                  (ecif->cif->abi & SFFI_LINUX_LONG_DOUBLE_IEEE128) != 0)
                {
                  do
                    {
                      if (vecarg_count < NUM_VEC_ARG_REGISTERS64
                          && i < nfixedargs)
		        memcpy (vec_base.f128++, arg.f128++, sizeof (float128));
                      else
		        memcpy (next_arg.f128, arg.f128++, sizeof (float128));
                      if (++next_arg.f128 == gpr_end.f128)
                        next_arg.f128 = rest.f128;
                      vecarg_count++;
                    }
                  while (--elnum != 0);
                }
              else
#endif
	      if (elt == SFFI_TYPE_FLOAT)
		{
		  do
		    {
		      double_tmp = *arg.f++;
		      if (fparg_count < NUM_FPR_ARG_REGISTERS64
			  && i < nfixedargs)
			*fpr_base.d++ = double_tmp;
		      else
			*next_arg.f = (float) double_tmp;
		      if (++next_arg.f == gpr_end.f)
			next_arg.f = rest.f;
		      fparg_count++;
		    }
		  while (--elnum != 0);
		  if ((next_arg.p & 7) != 0)
                    if (++next_arg.f == gpr_end.f)
                      next_arg.f = rest.f;
		}
	      else
		do
		  {
		    double_tmp = *arg.d++;
		    if (fparg_count < NUM_FPR_ARG_REGISTERS64 && i < nfixedargs)
		      *fpr_base.d++ = double_tmp;
		    else
		      *next_arg.d = double_tmp;
		    if (++next_arg.d == gpr_end.d)
		      next_arg.d = rest.d;
		    fparg_count++;
		  }
		while (--elnum != 0);
#else
	      if (elt == SFFI_TYPE_FLOAT)
		goto do_float;
	      else
		goto do_double;
#endif
	    }
	  else
	    {
	      words = ((*ptr)->size + 7) / 8;
	      if (next_arg.ul >= gpr_base.ul && next_arg.ul + words > gpr_end.ul)
		{
		  size_t first = gpr_end.c - next_arg.c;
		  memcpy (next_arg.c, *p_argv.c, first);
		  memcpy (rest.c, *p_argv.c + first, (*ptr)->size - first);
		  next_arg.c = rest.c + words * 8 - first;
		}
	      else
		{
		  char *where = next_arg.c;

#ifndef __LITTLE_ENDIAN__

		  if ((*ptr)->size < 8)
		    where += 8 - (*ptr)->size;
#endif
		  memcpy (where, *p_argv.c, (*ptr)->size);
		  next_arg.ul += words;
		  if (next_arg.ul == gpr_end.ul)
		    next_arg.ul = rest.ul;
		}
	    }
	  break;

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
	case SFFI_TYPE_UINT32:
	  gprvalue = **p_argv.ui;
	  goto putgpr;
	case SFFI_TYPE_INT:
	case SFFI_TYPE_SINT32:
	  gprvalue = **p_argv.si;
	  goto putgpr;

	case SFFI_TYPE_UINT64:
	case SFFI_TYPE_SINT64:
	case SFFI_TYPE_POINTER:
	  gprvalue = **p_argv.ul;
	putgpr:
	  *next_arg.ul++ = gprvalue;
	  if (next_arg.ul == gpr_end.ul)
	    next_arg.ul = rest.ul;
	  break;
	}
    }

  SFFI_ASSERT (flags & FLAG_4_GPR_ARGUMENTS
	      || (next_arg.ul >= gpr_base.ul
		  && next_arg.ul <= gpr_base.ul + 4));
}

#if _CALL_ELF == 2
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
#endif

sffi_status SFFI_HIDDEN
sffi_prep_closure_loc_linux64 (sffi_closure *closure,
			      sffi_cif *cif,
			      void (*fun) (sffi_cif *, void *, void **, void *),
			      void *user_data,
			      void *codeloc)
{
  if (cif->abi < SFFI_LINUX || cif->abi >= SFFI_LAST_ABI)
    return SFFI_BAD_ABI;

#ifdef SFFI_EXEC_STATIC_TRAMP
  if (sffi_tramp_is_present(closure))
    {

      void (*dest)(void) = sffi_closure_LINUX64;
      sffi_tramp_set_parms (closure->ftramp, dest, closure);
    }
  else
#endif
    {
#if _CALL_ELF == 2
      unsigned int *tramp = (unsigned int *) &closure->tramp[0];
      tramp[0] = 0xe96c0018;	
      tramp[1] = 0xe98c0010;	
      tramp[2] = 0x7d8903a6;	
      tramp[3] = 0x4e800420;	

      *(void **) &tramp[4] = (void *) sffi_closure_LINUX64;
      *(void **) &tramp[6] = codeloc;
      flush_icache ((char *) tramp, (char *) codeloc, 4 * 4);
#else

      void **tramp = (void **) &closure->tramp[0];
      memcpy (&tramp[0], (void **) sffi_closure_LINUX64, sizeof (void *));
      tramp[1] = codeloc;
      memcpy (&tramp[2], (void **) sffi_closure_LINUX64 + 1, sizeof (void *));
#endif
    }

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  return SFFI_OK;
}

int SFFI_HIDDEN
sffi_closure_helper_LINUX64 (sffi_cif *cif,
			    void (*fun) (sffi_cif *, void *, void **, void *),
			    void *user_data,
			    void *rvalue,
			    unsigned long *pst,
                            sffi_dblfl *pfr,
                            float128 *pvec)
{

  void **avalue;
  sffi_type **arg_types;
  unsigned long i, avn, nfixedargs;
  sffi_dblfl *end_pfr = pfr + NUM_FPR_ARG_REGISTERS64;
  float128 *end_pvec = pvec + NUM_VEC_ARG_REGISTERS64;
  unsigned long align;

  avalue = alloca (cif->nargs * sizeof (void *));

  if (cif->rtype->type == SFFI_TYPE_STRUCT
      && (cif->flags & FLAG_RETURNS_SMST) == 0)
    {
      rvalue = (void *) *pst;
      pst++;
    }

  i = 0;
  avn = cif->nargs;
#if _CALL_ELF != 2
  nfixedargs = (unsigned) -1;
  if ((cif->flags & FLAG_COMPAT) == 0)
#endif
    nfixedargs = cif->nfixedargs;
  arg_types = cif->arg_types;

  while (i < avn)
    {
      unsigned int elt, elnum;

      switch (arg_types[i]->type)
	{
	case SFFI_TYPE_SINT8:
	case SFFI_TYPE_UINT8:
#ifndef __LITTLE_ENDIAN__
	  avalue[i] = (char *) pst + 7;
	  pst++;
	  break;
#endif

	case SFFI_TYPE_SINT16:
	case SFFI_TYPE_UINT16:
#ifndef __LITTLE_ENDIAN__
	  avalue[i] = (char *) pst + 6;
	  pst++;
	  break;
#endif

	case SFFI_TYPE_SINT32:
	case SFFI_TYPE_UINT32:
#ifndef __LITTLE_ENDIAN__
	  avalue[i] = (char *) pst + 4;
	  pst++;
	  break;
#endif

	case SFFI_TYPE_SINT64:
	case SFFI_TYPE_UINT64:
	case SFFI_TYPE_POINTER:
	  avalue[i] = pst;
	  pst++;
	  break;

	case SFFI_TYPE_STRUCT:
	  if ((cif->abi & SFFI_LINUX_STRUCT_ALIGN) != 0)
	    {
	      align = arg_types[i]->alignment;
	      if (align > 16)
		align = 16;
	      if (align > 1)
		pst = (unsigned long *) SFFI_ALIGN ((size_t) pst, align);
	    }
	  elt = discover_homogeneous_aggregate (cif->abi, arg_types[i], &elnum);
	  if (elt)
	    {
#if _CALL_ELF == 2
	      union {
		void *v;
		unsigned long *ul;
		float *f;
		double *d;
		float128 *f128;
		size_t p;
	      } to, from;

#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
              if (elt == SFFI_TYPE_LONGDOUBLE &&
                  (cif->abi & SFFI_LINUX_LONG_DOUBLE_IEEE128) != 0)
                {
                  if (pvec + elnum <= end_pvec)
                    to.v = pvec;
                  else
                    to.v = pst;
                }
              else
#endif
	      if (pfr + elnum <= end_pfr)
		to.v = pfr;
	      else
		to.v = pst;

	      avalue[i] = to.v;
	      from.ul = pst;
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
              if (elt == SFFI_TYPE_LONGDOUBLE &&
                  (cif->abi & SFFI_LINUX_LONG_DOUBLE_IEEE128) != 0)
                {
                  do
                    {
                      if (pvec < end_pvec && i < nfixedargs)
		        memcpy (to.f128, pvec++, sizeof (float128));
                      else
		        memcpy (to.f128, from.f128, sizeof (float128));
                      to.f128++;
                      from.f128++;
                    }
                  while (--elnum != 0);
                }
              else
#endif
	      if (elt == SFFI_TYPE_FLOAT)
		{
		  do
		    {
		      if (pfr < end_pfr && i < nfixedargs)
			{
			  *to.f = (float) pfr->d;
			  pfr++;
			}
		      else
			*to.f = *from.f;
		      to.f++;
		      from.f++;
		    }
		  while (--elnum != 0);
		}
	      else
		{
		  do
		    {
		      if (pfr < end_pfr && i < nfixedargs)
			{
			  *to.d = pfr->d;
			  pfr++;
			}
		      else
			*to.d = *from.d;
		      to.d++;
		      from.d++;
		    }
		  while (--elnum != 0);
		}
#else
	      if (elt == SFFI_TYPE_FLOAT)
		goto do_float;
	      else
		goto do_double;
#endif
	    }
	  else
	    {
#ifndef __LITTLE_ENDIAN__

	      if (arg_types[i]->size < 8)
		avalue[i] = (char *) pst + 8 - arg_types[i]->size;
	      else
#endif
		avalue[i] = pst;
	    }
	  pst += (arg_types[i]->size + 7) / 8;
	  break;

#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
	case SFFI_TYPE_LONGDOUBLE:
          if ((cif->abi & SFFI_LINUX_LONG_DOUBLE_IEEE128) != 0)
            {
              if (((unsigned long) pst & 0xF) != 0)
                ++pst;
              if (pvec < end_pvec && i < nfixedargs)
                avalue[i] = pvec++;
              else
                avalue[i] = pst;
              pst += 2;
              break;
            }
          else if ((cif->abi & SFFI_LINUX_LONG_DOUBLE_128) != 0)
	    {
	      if (pfr + 1 < end_pfr && i + 1 < nfixedargs)
		{
		  avalue[i] = pfr;
		  pfr += 2;
		}
	      else
		{
		  if (pfr < end_pfr && i < nfixedargs)
		    {

		      *pst = *(unsigned long *) pfr;
		      pfr++;
		    }
		  avalue[i] = pst;
		}
	      pst += 2;
	      break;
	    }

#endif
	case SFFI_TYPE_DOUBLE:
#if _CALL_ELF != 2
	do_double:
#endif

	  if (pfr < end_pfr && i < nfixedargs)
	    {
	      avalue[i] = pfr;
	      pfr++;
	    }
	  else
	    avalue[i] = pst;
	  pst++;
	  break;

	case SFFI_TYPE_FLOAT:
#if _CALL_ELF != 2
	do_float:
#endif
	  if (pfr < end_pfr && i < nfixedargs)
	    {

	      pfr->f = (float) pfr->d;
	      avalue[i] = pfr;
	      pfr++;
	    }
	  else
	    {
#ifndef __LITTLE_ENDIAN__
	      avalue[i] = (char *) pst + 4;
#else
	      avalue[i] = pst;
#endif
	    }
	  pst++;
	  break;

#ifdef SFFI_TARGET_HAS_COMPLEX_TYPE
	case SFFI_TYPE_COMPLEX:

	  {
	    unsigned int j;
	    elt = arg_types[i]->elements[0]->type;
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
	    if (elt == SFFI_TYPE_LONGDOUBLE)
	      elt = SFFI_TYPE_DOUBLE;
#endif
	    if (elt == SFFI_TYPE_FLOAT)
	      {
		float *cval = alloca (2 * sizeof (float));
		for (j = 0; j < 2; j++)
		  {
		    if (pfr < end_pfr && i < nfixedargs)
		      {
			cval[j] = (float) pfr->d;
			pfr++;
		      }
		    else
		      {
#ifndef __LITTLE_ENDIAN__
			cval[j] = ((float *) pst)[1];
#else
			cval[j] = ((float *) pst)[0];
#endif
		      }
		    pst++;
		  }
		avalue[i] = cval;
	      }
	    else if (elt == SFFI_TYPE_DOUBLE)
	      {
		double *cval = alloca (2 * sizeof (double));
		for (j = 0; j < 2; j++)
		  {
		    if (pfr < end_pfr && i < nfixedargs)
		      {
			cval[j] = pfr->d;
			pfr++;
		      }
		    else
		      cval[j] = *(double *) pst;
		    pst++;
		  }
		avalue[i] = cval;
	      }
	    else
	      {

		size_t hsize = arg_types[i]->elements[0]->size;
		char *cval = alloca (2 * hsize);
		for (j = 0; j < 2; j++)
		  {
		    char *src;
#ifndef __LITTLE_ENDIAN__
		    src = (char *) pst + (8 - hsize);
#else
		    src = (char *) pst;
#endif
		    memcpy (cval + j * hsize, src, hsize);
		    pst++;
		  }
		avalue[i] = cval;
	      }
	  }
	  break;
#endif

	default:
	  SFFI_ASSERT (0);
	}

      i++;
    }

  (*fun) (cif, rvalue, avalue, user_data);

  switch (cif->rtype->type)
    {
    case SFFI_TYPE_VOID:
      return PPC_LD_NONE;
    case SFFI_TYPE_FLOAT:
      return PPC_LD_F32;
    case SFFI_TYPE_DOUBLE:
      return PPC_LD_F64;
#if SFFI_TYPE_DOUBLE != SFFI_TYPE_LONGDOUBLE
    case SFFI_TYPE_LONGDOUBLE:
      if ((cif->flags & FLAG_RETURNS_VEC) != 0)
	return PPC64_LD_VECTOR;
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
#ifdef SFFI_TARGET_HAS_COMPLEX_TYPE
    case SFFI_TYPE_COMPLEX:
      {
	int inner = cif->rtype->elements[0]->type;
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
	if (inner == SFFI_TYPE_LONGDOUBLE)
	  inner = SFFI_TYPE_DOUBLE;
#endif
	if (inner == SFFI_TYPE_FLOAT)
	  return PPC64_LD_FLOAT_HOMOG;
	if (inner == SFFI_TYPE_DOUBLE)
	  return PPC64_LD_DOUBLE_HOMOG;

	{
	  char *rv = rvalue;
	  unsigned long re, im;
	  switch (inner)
	    {
	    case SFFI_TYPE_UINT8:
	      re = ((unsigned char *) rv)[0];
	      im = ((unsigned char *) rv)[1];
	      break;
	    case SFFI_TYPE_SINT8:
	      re = (unsigned long) (long) ((signed char *) rv)[0];
	      im = (unsigned long) (long) ((signed char *) rv)[1];
	      break;
	    case SFFI_TYPE_UINT16:
	      re = ((unsigned short *) rv)[0];
	      im = ((unsigned short *) rv)[1];
	      break;
	    case SFFI_TYPE_SINT16:
	      re = (unsigned long) (long) ((signed short *) rv)[0];
	      im = (unsigned long) (long) ((signed short *) rv)[1];
	      break;
	    case SFFI_TYPE_UINT32:
	      re = ((unsigned int *) rv)[0];
	      im = ((unsigned int *) rv)[1];
	      break;
	    case SFFI_TYPE_INT:
	    case SFFI_TYPE_SINT32:
	      re = (unsigned long) (long) ((signed int *) rv)[0];
	      im = (unsigned long) (long) ((signed int *) rv)[1];
	      break;
	    case SFFI_TYPE_SINT64:
	    case SFFI_TYPE_UINT64:
	    case SFFI_TYPE_POINTER:
	    default:
	      re = ((unsigned long *) rv)[0];
	      im = ((unsigned long *) rv)[1];
	      break;
	    }
	  ((unsigned long *) rv)[0] = re;
	  ((unsigned long *) rv)[1] = im;
	  return PPC_LD_R3R4;
	}
      }
#endif
    case SFFI_TYPE_STRUCT:
      if ((cif->flags & FLAG_RETURNS_SMST) != 0)
	{
	  if ((cif->flags & (FLAG_RETURNS_FP | FLAG_RETURNS_VEC)) == 0)
	    {

	      switch (cif->rtype->size)
		{
		case 0:
		  return PPC_LD_NONE;
		case 1:
		  return PPC_LD_U8;
		case 2:
		  return PPC_LD_U16;
		case 3:
		  return PPC64_LD_STRUCT_3;
		case 4:
		  return PPC_LD_U32;
		case 5:
		  return PPC64_LD_STRUCT_5;
		case 6:
		  return PPC64_LD_STRUCT_6;
		case 7:
		  return PPC64_LD_STRUCT_7;
		case 8 ... 16:
		  return PPC_LD_R3R4;
		}
	      break;
	    }
	  if ((cif->flags & FLAG_RETURNS_VEC) != 0)
	    return PPC64_LD_VECTOR_HOMOG;
	  if ((cif->flags & FLAG_RETURNS_64BITS) != 0)
	    return PPC64_LD_DOUBLE_HOMOG;
	  return PPC64_LD_FLOAT_HOMOG;
        }
      return PPC_LD_NONE;
    }
  abort();
}
#endif
