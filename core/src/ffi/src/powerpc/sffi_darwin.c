#include <sffi.h>
#include <sffi_common.h>

#include <stdlib.h>

struct sffi_aix_trampoline_struct {
    void * code_pointer;	
    void * toc;			
    void * static_chain;	
};

#define PPC_LD_NONE		0
#define PPC_LD_R3		1
#define PPC_LD_R3R4		2
#define PPC_LD_F32		3
#define PPC_LD_F64		4
#define PPC_LD_F128		5
#define PPC_LD_U8		6
#define PPC_LD_S8		7
#define PPC_LD_U16		8
#define PPC_LD_S16		9

#ifndef POWERPC64
# define PPC_LD_U32		PPC_LD_R3
# define PPC_LD_S32		PPC_LD_R3
# define PPC_LD_PTR		PPC_LD_R3
# define PPC_LD_I64		PPC_LD_R3R4
#else
# define PPC_LD_U32		10
# define PPC_LD_S32		11
# define PPC_LD_PTR		PPC_LD_R3
# define PPC_LD_I64		PPC_LD_R3
#endif

extern void sffi_closure_ASM (void);

#if defined (SFFI_GO_CLOSURES)
extern void sffi_go_closure_ASM (void);
#endif

enum {

  FLAG_RETURNS_128BITS	= 1 << (31-31), 
  FLAG_RETURNS_NOTHING	= 1 << (31-30),
  FLAG_RETURNS_FP	= 1 << (31-29),
  FLAG_RETURNS_64BITS	= 1 << (31-28),

  FLAG_RETURNS_STRUCT	= 1 << (31-27), 

  FLAG_ARG_NEEDS_COPY   = 1 << (31- 7),
  FLAG_FP_ARGUMENTS     = 1 << (31- 6), 
  FLAG_4_GPR_ARGUMENTS  = 1 << (31- 5),
  FLAG_RETVAL_REFERENCE = 1 << (31- 4)
};

enum {
  NUM_GPR_ARG_REGISTERS = 8,
  NUM_FPR_ARG_REGISTERS = 13,
  LINKAGE_AREA_GPRS = 6
};

enum { ASM_NEEDS_REGISTERS = 4 }; 

#if defined(POWERPC_DARWIN64)
static void
darwin64_pass_struct_by_value 
  (sffi_type *, char *, unsigned, unsigned *, double **, unsigned long **);
#endif

void
sffi_prep_args (extended_cif *ecif, unsigned long *const stack)
{
  const unsigned bytes = ecif->cif->bytes;
  const unsigned flags = ecif->cif->flags;
  const unsigned nargs = ecif->cif->nargs;
#if !defined(POWERPC_DARWIN64) 
  const sffi_abi abi = ecif->cif->abi;
#endif

  unsigned long *const stacktop = stack + (bytes / sizeof(unsigned long));

  double *fpr_base = (double *) (stacktop - ASM_NEEDS_REGISTERS) - NUM_FPR_ARG_REGISTERS;
  int gp_count = 0, fparg_count = 0;

  unsigned long *next_arg = stack + LINKAGE_AREA_GPRS; 

  int i;
  double double_tmp;
  void **p_argv = ecif->avalue;
  unsigned long gprvalue;
  sffi_type** ptr = ecif->cif->arg_types;
#if !defined(POWERPC_DARWIN64) 
  char *dest_cpy;
#endif
  unsigned size_al = 0;

  SFFI_ASSERT(((unsigned) (char *) stack & 0xF) == 0);
  SFFI_ASSERT(((unsigned) (char *) stacktop & 0xF) == 0);
  SFFI_ASSERT((bytes & 0xF) == 0);

  if (flags & FLAG_RETVAL_REFERENCE)
    *next_arg++ = (unsigned long) (char *) ecif->rvalue;

  for (i = nargs; i > 0; i--, ptr++, p_argv++)
    {
      switch ((*ptr)->type)
	{

	case SFFI_TYPE_FLOAT:
	  double_tmp = *(float *) *p_argv;
	  if (fparg_count < NUM_FPR_ARG_REGISTERS)
	    *fpr_base++ = double_tmp;
#if defined(POWERPC_DARWIN)
	  *(float *)next_arg = *(float *) *p_argv;
#else
	  *(double *)next_arg = double_tmp;
#endif
	  next_arg++;
	  gp_count++;
	  fparg_count++;
	  SFFI_ASSERT(flags & FLAG_FP_ARGUMENTS);
	  break;

	case SFFI_TYPE_DOUBLE:
	  double_tmp = *(double *) *p_argv;
	  if (fparg_count < NUM_FPR_ARG_REGISTERS)
	    *fpr_base++ = double_tmp;
	  *(double *)next_arg = double_tmp;
#ifdef POWERPC64
	  next_arg++;
	  gp_count++;
#else
	  next_arg += 2;
	  gp_count += 2;
#endif
	  fparg_count++;
	  SFFI_ASSERT(flags & FLAG_FP_ARGUMENTS);
	  break;

#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE

	case SFFI_TYPE_LONGDOUBLE:
#  if defined(POWERPC64) && !defined(POWERPC_DARWIN64)

	  if (fparg_count < NUM_FPR_ARG_REGISTERS)
	    *(long double *) fpr_base++ = *(long double *) *p_argv;
	  else
	    *(long double *) next_arg = *(long double *) *p_argv;
	  next_arg += 2;
	  fparg_count += 2;
#  else
	  double_tmp = ((double *) *p_argv)[0];
	  if (fparg_count < NUM_FPR_ARG_REGISTERS)
	    *fpr_base++ = double_tmp;
	  *(double *) next_arg = double_tmp;
#    if defined(POWERPC_DARWIN64)
	  next_arg++;
	  gp_count++;
#    else
	  next_arg += 2;
	  gp_count += 2;
#    endif
	  fparg_count++;
	  double_tmp = ((double *) *p_argv)[1];
	  if (fparg_count < NUM_FPR_ARG_REGISTERS)
	    *fpr_base++ = double_tmp;
	  *(double *) next_arg = double_tmp;
#    if defined(POWERPC_DARWIN64)
	  next_arg++;
	  gp_count++;
#    else
	  next_arg += 2;
	  gp_count += 2;
#    endif
	  fparg_count++;
#  endif
	  SFFI_ASSERT(flags & FLAG_FP_ARGUMENTS);
	  break;
#endif
	case SFFI_TYPE_UINT64:
	case SFFI_TYPE_SINT64:
#ifdef POWERPC64
	  gprvalue = *(long long *) *p_argv;
	  goto putgpr;
#else
	  *(long long *) next_arg = *(long long *) *p_argv;
	  next_arg += 2;
	  gp_count += 2;
#endif
	  break;
	case SFFI_TYPE_POINTER:
	  gprvalue = *(unsigned long *) *p_argv;
	  goto putgpr;
	case SFFI_TYPE_UINT8:
	  gprvalue = *(unsigned char *) *p_argv;
	  goto putgpr;
	case SFFI_TYPE_SINT8:
	  gprvalue = *(signed char *) *p_argv;
	  goto putgpr;
	case SFFI_TYPE_UINT16:
	  gprvalue = *(unsigned short *) *p_argv;
	  goto putgpr;
	case SFFI_TYPE_SINT16:
	  gprvalue = *(signed short *) *p_argv;
	  goto putgpr;

	case SFFI_TYPE_STRUCT:
	  size_al = (*ptr)->size;
#if defined(POWERPC_DARWIN64)
	  next_arg = (unsigned long *)SFFI_ALIGN((char *)next_arg, (*ptr)->alignment);
	  darwin64_pass_struct_by_value (*ptr, (char *) *p_argv, 
					 (unsigned) size_al,
					 (unsigned int *) &fparg_count,
					 &fpr_base, &next_arg);
#else
	  dest_cpy = (char *) next_arg;

	  if ((*ptr)->elements[0]->type == SFFI_TYPE_DOUBLE)
	    size_al = SFFI_ALIGN((*ptr)->size, 8);

#  if defined(POWERPC64) 
	  SFFI_ASSERT (abi != SFFI_DARWIN);
	  memcpy ((char *) dest_cpy, (char *) *p_argv, size_al);
	  next_arg += (size_al + 7) / 8;
#  else

	  if (size_al < 3 && abi == SFFI_DARWIN)
	    dest_cpy += 4 - size_al;

	  memcpy((char *) dest_cpy, (char *) *p_argv, size_al);
	  next_arg += (size_al + 3) / 4;
#  endif
#endif
	  break;

	case SFFI_TYPE_INT:
	case SFFI_TYPE_SINT32:
	  gprvalue = *(signed int *) *p_argv;
	  goto putgpr;

	case SFFI_TYPE_UINT32:
	  gprvalue = *(unsigned int *) *p_argv;
	putgpr:
	  *next_arg++ = gprvalue;
	  gp_count++;
	  break;
	default:
	  break;
	}
    }

}

#if defined(POWERPC_DARWIN64)

static void
darwin64_scan_struct_for_floats (sffi_type *s, unsigned *nfpr)
{
  int i;

  SFFI_ASSERT (s->type == SFFI_TYPE_STRUCT)

  for (i = 0; s->elements[i] != NULL; i++)
    {
      sffi_type *p = s->elements[i];
      switch (p->type)
	{
	  case SFFI_TYPE_STRUCT:
	    darwin64_scan_struct_for_floats (p, nfpr);
	    break;
	  case SFFI_TYPE_LONGDOUBLE:
	    (*nfpr) += 2;
	    break;
	  case SFFI_TYPE_DOUBLE:
	  case SFFI_TYPE_FLOAT:
	    (*nfpr) += 1;
	    break;
	  default:
	    break;    
	}
    }
}

static int
darwin64_struct_size_exceeds_gprs_p (sffi_type *s, char *src, unsigned *nfpr)
{
  unsigned struct_offset=0, i;

  for (i = 0; s->elements[i] != NULL; i++)
    {
      char *item_base;
      sffi_type *p = s->elements[i];

      if (i > 0)
        struct_offset = SFFI_ALIGN(struct_offset, p->alignment);

      item_base = src + struct_offset;

      switch (p->type)
	{
	  case SFFI_TYPE_STRUCT:
	    if (darwin64_struct_size_exceeds_gprs_p (p, item_base, nfpr))
	      return 1;
	    break;
	  case SFFI_TYPE_LONGDOUBLE:
	    if (*nfpr >= NUM_FPR_ARG_REGISTERS)
	      return 1;
	    (*nfpr) += 1;
	    item_base += 8;

	  case SFFI_TYPE_DOUBLE:
	    if (*nfpr >= NUM_FPR_ARG_REGISTERS)
	      return 1;
	    (*nfpr) += 1;
	    break;
	  case SFFI_TYPE_FLOAT:
	    if (*nfpr >= NUM_FPR_ARG_REGISTERS)
	      return 1;
	    (*nfpr) += 1;
	    break;
	  default:

	    if ((unsigned long)item_base >= 8*8) 
	      return 1;
	    break;    
	}

      struct_offset += p->size;
    }
  return 0;
}

int 
darwin64_struct_ret_by_value_p (sffi_type *s)
{
  unsigned nfp = 0;

  SFFI_ASSERT (s && s->type == SFFI_TYPE_STRUCT);

  if (s->size > 168)
    return 0;

  darwin64_scan_struct_for_floats (s, &nfp);
  if (nfp > 13)
    return 0;

  if (s->size <= 64)
    return 1;

  nfp = 0;
  if (darwin64_struct_size_exceeds_gprs_p (s, NULL, &nfp))
    return 0;

  return 1;
}

void
darwin64_pass_struct_floats (sffi_type *s, char *src, 
			     unsigned *nfpr, double **fprs)
{
  int i;
  double *fpr_base = *fprs;
  unsigned struct_offset = 0;

  for (i = 0; s->elements[i] != NULL; i++)
    {
      char *item_base;
      sffi_type *p = s->elements[i];

      if (i > 0)
        struct_offset = SFFI_ALIGN(struct_offset, p->alignment);
      item_base = src + struct_offset;

      switch (p->type)
	{
	  case SFFI_TYPE_STRUCT:
	    darwin64_pass_struct_floats (p, item_base, nfpr,
					   &fpr_base);
	    break;
	  case SFFI_TYPE_LONGDOUBLE:
	    if (*nfpr < NUM_FPR_ARG_REGISTERS)
	      *fpr_base++ = *(double *)item_base;
	    (*nfpr) += 1;
	    item_base += 8;

	  case SFFI_TYPE_DOUBLE:
	    if (*nfpr < NUM_FPR_ARG_REGISTERS)
	      *fpr_base++ = *(double *)item_base;
	    (*nfpr) += 1;
	    break;
	  case SFFI_TYPE_FLOAT:
	    if (*nfpr < NUM_FPR_ARG_REGISTERS)
	      *fpr_base++ = (double) *(float *)item_base;
	    (*nfpr) += 1;
	    break;
	  default:
	    break;    
	}

      struct_offset += p->size;
    }

  *fprs = fpr_base;
}

static void
darwin64_pass_struct_by_value (sffi_type *s, char *src, unsigned size,
			       unsigned *nfpr, double **fprs, unsigned long **arg)
{
  unsigned long *next_arg = *arg;
  char *dest_cpy = (char *)next_arg;

  SFFI_ASSERT (s->type == SFFI_TYPE_STRUCT)

  if (!size)
    return;

  if (size < 3
      || (size == 4 
	  && s->elements[0] 
	  && s->elements[0]->type != SFFI_TYPE_FLOAT))
    {

      *next_arg = 0UL; 
      dest_cpy += 8 - size;
      memcpy ((char *) dest_cpy, src, size);
      next_arg++;
    }
  else if (size == 16)
    {
      memcpy ((char *) dest_cpy, src, size);
      next_arg += 2;
    }
  else
    {

      memcpy ((char *) dest_cpy, src, size);
      darwin64_pass_struct_floats (s, src, nfpr, fprs);
      next_arg += (size+7)/8;
    }

  *arg = next_arg;
}

double *
darwin64_struct_floats_to_mem (sffi_type *s, char *dest, double *fprs, unsigned *nf)
{
  int i;
  unsigned struct_offset = 0;

  for (i = 0; s->elements[i] != NULL; i++)
    {
      char *item_base;
      sffi_type *p = s->elements[i];

      if (i > 0)
        struct_offset = SFFI_ALIGN(struct_offset, p->alignment);
      item_base = dest + struct_offset;

      switch (p->type)
	{
	  case SFFI_TYPE_STRUCT:
	    fprs = darwin64_struct_floats_to_mem (p, item_base, fprs, nf);
	    break;
	  case SFFI_TYPE_LONGDOUBLE:
	    if (*nf < NUM_FPR_ARG_REGISTERS)
	      {
		*(double *)item_base = *fprs++ ;
		(*nf) += 1;
	      }
	    item_base += 8;

	  case SFFI_TYPE_DOUBLE:
	    if (*nf < NUM_FPR_ARG_REGISTERS)
	      {
		*(double *)item_base = *fprs++ ;
		(*nf) += 1;
	      }
	    break;
	  case SFFI_TYPE_FLOAT:
	    if (*nf < NUM_FPR_ARG_REGISTERS)
	      {
		*(float *)item_base = (float) *fprs++ ;
		(*nf) += 1;
	      }
	    break;
	  default:
	    break;    
	}

      struct_offset += p->size;
    }
  return fprs;
}

#endif

static void
darwin_adjust_aggregate_sizes (sffi_type *s)
{
  int i;

  if (s->type != SFFI_TYPE_STRUCT)
    return;

  s->size = 0;
  for (i = 0; s->elements[i] != NULL; i++)
    {
      sffi_type *p;
      int align;

      p = s->elements[i];
      if (p->type == SFFI_TYPE_STRUCT)
	darwin_adjust_aggregate_sizes (p);
#if defined(POWERPC_DARWIN64)

      align = p->alignment;
#else

      if (i == 0)
	align = p->alignment;
      else if (p->alignment == 16 || p->alignment < 4)

	align = p->alignment;
      else

	align = 4;
#endif

      s->size = SFFI_ALIGN(s->size, align) + p->size;
    }

  s->size = SFFI_ALIGN(s->size, s->alignment);

  if (s->elements[0]->type == SFFI_TYPE_UINT64
      || s->elements[0]->type == SFFI_TYPE_SINT64
      || s->elements[0]->type == SFFI_TYPE_DOUBLE
      || s->elements[0]->alignment == 8)
    s->alignment = s->alignment > 8 ? s->alignment : 8;

}

static int
aix_adjust_aggregate_sizes (sffi_type *s, int outer_most_type_or_first_member)
{
  int i, nested_first_member=0, final_align, rc=0;

  if (s->type != SFFI_TYPE_STRUCT)
    return 0;

  s->size = 0;
  for (i = 0; s->elements[i] != NULL; i++)
    {
      sffi_type p;
      int align;

      p = *(s->elements[i]);
      if (i == 0)
        nested_first_member = aix_adjust_aggregate_sizes(&p, outer_most_type_or_first_member);
      else
        aix_adjust_aggregate_sizes(&p, 0);
      align = p.alignment;
      if (i != 0 && p.type == SFFI_TYPE_DOUBLE)
        align = 4;
      s->size = SFFI_ALIGN(s->size, align) + p.size;
    }

  final_align=s->alignment;
  if ((s->elements[0]->type == SFFI_TYPE_UINT64
          || s->elements[0]->type == SFFI_TYPE_SINT64
          || s->elements[0]->type == SFFI_TYPE_DOUBLE
          || s->elements[0]->alignment == 8 || nested_first_member)) {
      final_align = s->alignment > 8 ? s->alignment : 8;
      rc=1;

      if (outer_most_type_or_first_member)
        s->alignment=final_align;
  }

  s->size = SFFI_ALIGN(s->size, final_align);
  return rc;
}

sffi_status
sffi_prep_cif_machdep (sffi_cif *cif)
{

  unsigned i;
  sffi_type **ptr;
  unsigned bytes;
  unsigned fparg_count = 0, intarg_count = 0;
  unsigned flags = 0;
  unsigned size_al = 0;

  if (cif->abi == SFFI_DARWIN)
    {
      darwin_adjust_aggregate_sizes (cif->rtype);
      for (i = 0; i < cif->nargs; i++)
	darwin_adjust_aggregate_sizes (cif->arg_types[i]);
    }

  if (cif->abi == SFFI_AIX)
    {
      aix_adjust_aggregate_sizes (cif->rtype, 1);
      for (i = 0; i < cif->nargs; i++)
	aix_adjust_aggregate_sizes (cif->arg_types[i], 1);
    }

  bytes = (LINKAGE_AREA_GPRS + ASM_NEEDS_REGISTERS) * sizeof(unsigned long);

  switch (cif->rtype->type)
    {

#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
    case SFFI_TYPE_LONGDOUBLE:
      flags |= FLAG_RETURNS_128BITS;
      flags |= FLAG_RETURNS_FP;
      break;
#endif

    case SFFI_TYPE_DOUBLE:
      flags |= FLAG_RETURNS_64BITS;

    case SFFI_TYPE_FLOAT:
      flags |= FLAG_RETURNS_FP;
      break;

    case SFFI_TYPE_UINT64:
    case SFFI_TYPE_SINT64:
#ifdef POWERPC64
    case SFFI_TYPE_POINTER:
#endif
      flags |= FLAG_RETURNS_64BITS;
      break;

    case SFFI_TYPE_STRUCT:
#if defined(POWERPC_DARWIN64)
      {

	if (darwin64_struct_ret_by_value_p (cif->rtype))
	  {
	    unsigned nfpr = 0;
	    flags |= FLAG_RETURNS_STRUCT;
	    if (cif->rtype->size != 16)
	      darwin64_scan_struct_for_floats (cif->rtype, &nfpr) ;
	    else
	      flags |= FLAG_RETURNS_128BITS;

	    if (nfpr)
	      flags |= FLAG_RETURNS_FP;
	  }
	else 
	  {
	    flags |= FLAG_RETVAL_REFERENCE;
	    flags |= FLAG_RETURNS_NOTHING;
	    intarg_count++;
	  }
      }
#elif defined(DARWIN_PPC)
      if (cif->rtype->size <= 4)
	flags |= FLAG_RETURNS_STRUCT;
      else 
	{
	  flags |= FLAG_RETVAL_REFERENCE;
	  flags |= FLAG_RETURNS_NOTHING;
	  intarg_count++;
	}
#else 
      flags |= FLAG_RETVAL_REFERENCE;
      flags |= FLAG_RETURNS_NOTHING;
      intarg_count++;
#endif
      break;
    case SFFI_TYPE_VOID:
      flags |= FLAG_RETURNS_NOTHING;
      break;

    default:

      break;
    }

  for (ptr = cif->arg_types, i = cif->nargs; i > 0; i--, ptr++)
    {
      unsigned align_words;
      switch ((*ptr)->type)
	{
	case SFFI_TYPE_FLOAT:
	case SFFI_TYPE_DOUBLE:
	  fparg_count++;
#if !defined(POWERPC_DARWIN64)

	  if (fparg_count > NUM_FPR_ARG_REGISTERS
	      && (intarg_count & 0x01) != 0)
	    intarg_count++;
#endif
	  break;

#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
	case SFFI_TYPE_LONGDOUBLE:
	  fparg_count += 2;

	  if (fparg_count >= NUM_FPR_ARG_REGISTERS)
#if defined (POWERPC64)
	    intarg_count = SFFI_ALIGN(intarg_count, 2);
#else
	    intarg_count = SFFI_ALIGN(intarg_count, 4);
#endif
	  break;
#endif

	case SFFI_TYPE_UINT64:
	case SFFI_TYPE_SINT64:
#if defined(POWERPC64)
	  intarg_count++;
#else

	  if (intarg_count == NUM_GPR_ARG_REGISTERS-1
	      || (intarg_count >= NUM_GPR_ARG_REGISTERS 
	          && (intarg_count & 0x01) != 0))
	    intarg_count++;
	  intarg_count += 2;
#endif
	  break;

	case SFFI_TYPE_STRUCT:
	  size_al = (*ptr)->size;
#if defined(POWERPC_DARWIN64)
	  align_words = (*ptr)->alignment >> 3;
	  if (align_words)
	    intarg_count = SFFI_ALIGN(intarg_count, align_words);

	  intarg_count += (size_al + 7) / 8;

	  if (size_al != 16)

	    darwin64_scan_struct_for_floats (*ptr, &fparg_count) ;
#else
	  align_words = (*ptr)->alignment >> 2;
	  if (align_words)
	    intarg_count = SFFI_ALIGN(intarg_count, align_words);

#  ifdef POWERPC64
	  intarg_count += (size_al + 7) / 8;
#  else
	  intarg_count += (size_al + 3) / 4;
#  endif
#endif
	  break;

	default:

	  intarg_count++;
	  break;
	}
    }

  if (fparg_count != 0)
    flags |= FLAG_FP_ARGUMENTS;

#if defined(POWERPC_DARWIN64)

  if (fparg_count != 0 
      || ((flags & FLAG_RETURNS_STRUCT)
	   && (flags & FLAG_RETURNS_FP)))
    bytes += NUM_FPR_ARG_REGISTERS * sizeof(double);
#else

  if (fparg_count != 0)
    bytes += NUM_FPR_ARG_REGISTERS * sizeof(double);
#endif

#ifdef POWERPC64
  if ((intarg_count + fparg_count) > NUM_GPR_ARG_REGISTERS)
    bytes += (intarg_count + fparg_count) * sizeof(long);
#else
  if ((intarg_count + 2 * fparg_count) > NUM_GPR_ARG_REGISTERS)
    bytes += (intarg_count + 2 * fparg_count) * sizeof(long);
#endif
  else
    bytes += NUM_GPR_ARG_REGISTERS * sizeof(long);

  bytes = SFFI_ALIGN(bytes, 16) ;

  cif->flags = flags;
  cif->bytes = bytes;

  return SFFI_OK;
}

extern void sffi_call_AIX(extended_cif *, long, unsigned, unsigned *,
			 void (*fn)(void), void (*fn2)(void));

#if defined (SFFI_GO_CLOSURES)
extern void sffi_call_go_AIX(extended_cif *, long, unsigned, unsigned *,
			    void (*fn)(void), void (*fn2)(void), void *closure);
#endif

extern void sffi_call_DARWIN(extended_cif *, long, unsigned, unsigned *,
			    void (*fn)(void), void (*fn2)(void), sffi_type*);

void
sffi_call (sffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;

  if ((rvalue == NULL) &&
      (cif->rtype->type == SFFI_TYPE_STRUCT))
    {
      ecif.rvalue = alloca (cif->rtype->size);
    }
  else
    ecif.rvalue = rvalue;

  switch (cif->abi)
    {
    case SFFI_AIX:
      sffi_call_AIX(&ecif, -(long)cif->bytes, cif->flags, ecif.rvalue, fn,
		   SFFI_FN(sffi_prep_args));
      break;
    case SFFI_DARWIN:
      sffi_call_DARWIN(&ecif, -(long)cif->bytes, cif->flags, ecif.rvalue, fn,
		      SFFI_FN(sffi_prep_args), cif->rtype);
      break;
    default:
      SFFI_ASSERT(0);
      break;
    }
}

#if defined (SFFI_GO_CLOSURES)
void
sffi_call_go (sffi_cif *cif, void (*fn) (void), void *rvalue, void **avalue,
	     void *closure)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;

  if ((rvalue == NULL) &&
      (cif->rtype->type == SFFI_TYPE_STRUCT))
    {
      ecif.rvalue = alloca (cif->rtype->size);
    }
  else
    ecif.rvalue = rvalue;

  switch (cif->abi)
    {
    case SFFI_AIX:
      sffi_call_go_AIX(&ecif, -(long)cif->bytes, cif->flags, ecif.rvalue, fn,
		      SFFI_FN(sffi_prep_args), closure);
      break;
    default:
      SFFI_ASSERT(0);
      break;
    }
}
#endif

static void flush_icache(char *);
static void flush_range(char *, int);

typedef struct aix_fd_struct {
  void *code_pointer;
  void *toc;
} aix_fd;

sffi_status
sffi_prep_closure_loc (sffi_closure* closure,
		      sffi_cif* cif,
		      void (*fun)(sffi_cif*, void*, void**, void*),
		      void *user_data,
		      void *codeloc)
{
  unsigned int *tramp;
  struct sffi_aix_trampoline_struct *tramp_aix;
  aix_fd *fd;

  switch (cif->abi)
    {
      case SFFI_DARWIN:

	SFFI_ASSERT (cif->abi == SFFI_DARWIN);

	tramp = (unsigned int *) &closure->tramp[0];
#if defined(POWERPC_DARWIN64)
	tramp[0] = 0x7c0802a6;  
	tramp[1] = 0x429f0015;  

	tramp[6] = 0x7d6802a6;  
	tramp[7] = 0xe98b0000;  
	tramp[8] = 0x7c0803a6;  
	tramp[9] = 0x7d8903a6;  
	tramp[10] = 0xe96b0008;  
	tramp[11] = 0x4e800420;  

	*((unsigned long *)&tramp[2]) = (unsigned long) sffi_closure_ASM; 
	*((unsigned long *)&tramp[4]) = (unsigned long) codeloc; 
#else
	tramp[0] = 0x7c0802a6;  
	tramp[1] = 0x429f000d;  
	tramp[4] = 0x7d6802a6;  
	tramp[5] = 0x818b0000;  
	tramp[6] = 0x7c0803a6;  
	tramp[7] = 0x7d8903a6;  
	tramp[8] = 0x816b0004;  
	tramp[9] = 0x4e800420;  
	tramp[2] = (unsigned long) sffi_closure_ASM; 
	tramp[3] = (unsigned long) codeloc; 
#endif
	closure->cif = cif;
	closure->fun = fun;
	closure->user_data = user_data;

	flush_range(codeloc, SFFI_TRAMPOLINE_SIZE);

	break;

    case SFFI_AIX:

      tramp_aix = (struct sffi_aix_trampoline_struct *) (closure->tramp);
      fd = (aix_fd *)(void *)sffi_closure_ASM;

      SFFI_ASSERT (cif->abi == SFFI_AIX);

      tramp_aix->code_pointer = fd->code_pointer;
      tramp_aix->toc = fd->toc;
      tramp_aix->static_chain = codeloc;
      closure->cif = cif;
      closure->fun = fun;
      closure->user_data = user_data;
      break;

    default:
      return SFFI_BAD_ABI;
      break;
    }
  return SFFI_OK;
}

#if defined (SFFI_GO_CLOSURES)
sffi_status
sffi_prep_go_closure (sffi_go_closure* closure,
		     sffi_cif* cif,
		     void (*fun)(sffi_cif*, void*, void**, void*))
{
  switch (cif->abi)
    {
      case SFFI_AIX:

        SFFI_ASSERT (cif->abi == SFFI_AIX);

        closure->tramp = (void *)sffi_go_closure_ASM;
        closure->cif = cif;
        closure->fun = fun;
        return SFFI_OK;

      default:
        return SFFI_BAD_ABI;
        break;
    }
  return SFFI_OK;
}
#endif

static void
flush_icache(char *addr)
{
#ifndef _AIX
  __asm__ volatile (
		"dcbf 0,%0\n"
		"\tsync\n"
		"\ticbi 0,%0\n"
		"\tsync\n"
		"\tisync"
		: : "r"(addr) : "memory");
#endif
}

static void
flush_range(char * addr1, int size)
{
#define MIN_LINE_SIZE 32
  int i;
  for (i = 0; i < size; i += MIN_LINE_SIZE)
    flush_icache(addr1+i);
  flush_icache(addr1+size-1);
}

typedef union
{
  float f;
  double d;
} sffi_dblfl;

int
sffi_closure_helper_DARWIN (sffi_closure *, void *,
			   unsigned long *, sffi_dblfl *);

#if defined (SFFI_GO_CLOSURES)
int
sffi_go_closure_helper_DARWIN (sffi_go_closure*, void *,
			      unsigned long *, sffi_dblfl *);
#endif

static int
sffi_closure_helper_common (sffi_cif* cif,
			   void (*fun)(sffi_cif*, void*, void**, void*),
			   void *user_data, void *rvalue,
			   unsigned long *pgr, sffi_dblfl *pfr)
{

  typedef double ldbits[2];

  union ldu
  {
    ldbits lb;
    long double ld;
  };

  void **          avalue;
  sffi_type **      arg_types;
  long             i, avn;
  sffi_dblfl *      end_pfr = pfr + NUM_FPR_ARG_REGISTERS;
  unsigned         size_al;
#if defined(POWERPC_DARWIN64)
  unsigned 	   fpsused = 0;
#endif

  avalue = alloca (cif->nargs * sizeof(void *));

  if (cif->rtype->type == SFFI_TYPE_STRUCT)
    {
#if defined(POWERPC_DARWIN64)
      if (!darwin64_struct_ret_by_value_p (cif->rtype))
	{

	  rvalue = (void *) *pgr;
	  pgr++;
	}
#elif defined(DARWIN_PPC)
      if (cif->rtype->size > 4)
	{
	  rvalue = (void *) *pgr;
	  pgr++;
	}
#else 
      rvalue = (void *) *pgr;
      pgr++;
#endif
    }

  i = 0;
  avn = cif->nargs;
  arg_types = cif->arg_types;

  while (i < avn)
    {
      switch (arg_types[i]->type)
	{
	case SFFI_TYPE_SINT8:
	case SFFI_TYPE_UINT8:
#if  defined(POWERPC64)
	  avalue[i] = (char *) pgr + 7;
#else
	  avalue[i] = (char *) pgr + 3;
#endif
	  pgr++;
	  break;

	case SFFI_TYPE_SINT16:
	case SFFI_TYPE_UINT16:
#if  defined(POWERPC64)
	  avalue[i] = (char *) pgr + 6;
#else
	  avalue[i] = (char *) pgr + 2;
#endif
	  pgr++;
	  break;

	case SFFI_TYPE_SINT32:
	case SFFI_TYPE_UINT32:
#if  defined(POWERPC64)
	  avalue[i] = (char *) pgr + 4;
#else
	case SFFI_TYPE_POINTER:
	  avalue[i] = pgr;
#endif
	  pgr++;
	  break;

	case SFFI_TYPE_STRUCT:
	  size_al = arg_types[i]->size;
#if defined(POWERPC_DARWIN64)
	  pgr = (unsigned long *)SFFI_ALIGN((char *)pgr, arg_types[i]->alignment);
	  if (size_al < 3 || size_al == 4)
	    {
	      avalue[i] = ((char *)pgr)+8-size_al;
	      if (arg_types[i]->elements[0]->type == SFFI_TYPE_FLOAT
		  && fpsused < NUM_FPR_ARG_REGISTERS)
		{
		  *(float *)pgr = (float) *(double *)pfr;
		  pfr++;
		  fpsused++;
		}
	    }
	  else 
	    {
	      if (size_al != 16)
		pfr = (sffi_dblfl *) 
		    darwin64_struct_floats_to_mem (arg_types[i], (char *)pgr,
						   (double *)pfr, &fpsused);
	      avalue[i] = pgr;
	    }
	  pgr += (size_al + 7) / 8;
#else

	  if (arg_types[i]->elements[0]->type == SFFI_TYPE_DOUBLE)
	    size_al = SFFI_ALIGN(arg_types[i]->size, 8);
#  if defined(POWERPC64)
	  SFFI_ASSERT (cif->abi != SFFI_DARWIN);
	  avalue[i] = pgr;
	  pgr += (size_al + 7) / 8;
#  else

	  if (size_al < 3 && cif->abi == SFFI_DARWIN)
	    avalue[i] = (char*) pgr + 4 - size_al;
	  else
	    avalue[i] = pgr;
	  pgr += (size_al + 3) / 4;
#  endif
#endif
	  break;

	case SFFI_TYPE_SINT64:
	case SFFI_TYPE_UINT64:
#if  defined(POWERPC64)
	case SFFI_TYPE_POINTER:
	  avalue[i] = pgr;
	  pgr++;
	  break;
#else

	  avalue[i] = pgr;
	  pgr += 2;
	  break;
#endif

	case SFFI_TYPE_FLOAT:

	  if (pfr < end_pfr)
	    {
	      double temp = pfr->d;
	      pfr->f = (float) temp;
	      avalue[i] = pfr;
	      pfr++;
	    }
	  else
	    {
	      avalue[i] = pgr;
	    }
	  pgr++;
	  break;

	case SFFI_TYPE_DOUBLE:

	  if (pfr < end_pfr)
	    {
	      avalue[i] = pfr;
	      pfr++;
	    }
	  else
	    {
	      avalue[i] = pgr;
	    }
#ifdef POWERPC64
	  pgr++;
#else
	  pgr += 2;
#endif
	  break;

#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE

	case SFFI_TYPE_LONGDOUBLE:
#ifdef POWERPC64
	  if (pfr + 1 < end_pfr)
	    {
	      avalue[i] = pfr;
	      pfr += 2;
	    }
	  else
	    {
	      if (pfr < end_pfr)
		{
		  *pgr = *(unsigned long *) pfr;
		  pfr++;
		}
	      avalue[i] = pgr;
	    }
	  pgr += 2;
#else  

	  if (pfr + 1 < end_pfr)
	    {
	      avalue[i] = pfr;
	      pfr += 2;
	    }

	  else if (pfr + 1 == end_pfr)
	    {
	      union ldu temp_ld;
	      memcpy (&temp_ld.lb[0], pfr, sizeof(ldbits));
	      memcpy (&temp_ld.lb[1], pgr + 2, sizeof(ldbits));
	      avalue[i] = &temp_ld.ld;
	      pfr++;
	    }
	  else
	    {
	      avalue[i] = pgr;
	    }
	  pgr += 4;
#endif  
	  break;
#endif
	default:
	  SFFI_ASSERT(0);
	}
      i++;
    }

  (fun) (cif, rvalue, avalue, user_data);

  switch (cif->rtype->type)
    {
    case SFFI_TYPE_VOID:
    case SFFI_TYPE_STRUCT:
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
    default:
      abort();
    }
}

int
sffi_closure_helper_DARWIN (sffi_closure *closure, void *rvalue,
			   unsigned long *pgr, sffi_dblfl *pfr)
{
  return sffi_closure_helper_common (closure->cif, closure->fun,
				    closure->user_data, rvalue, pgr, pfr);
}

#if defined (SFFI_GO_CLOSURES)
int
sffi_go_closure_helper_DARWIN (sffi_go_closure *closure, void *rvalue,
			      unsigned long *pgr, sffi_dblfl *pfr)
{
  return sffi_closure_helper_common (closure->cif, closure->fun,
				    closure, rvalue, pgr, pfr);
}
#endif
