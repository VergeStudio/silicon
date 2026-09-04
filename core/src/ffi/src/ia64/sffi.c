

#include <sffi.h>
#include <sffi_common.h>

#include <stdlib.h>
#include <stdbool.h>
#include <float.h>

#include "ia64_flags.h"


#ifdef __hpux
typedef void *PTR64;
#else
typedef void *PTR64 __attribute__((mode(DI)));
#endif


typedef struct
{
  UINT64 x[2] __attribute__((aligned(16)));
} fpreg;




struct ia64_args
{
  fpreg fp_regs[8];	
  UINT64 gp_regs[8];	
  UINT64 other_args[];	
};




static inline void *
endian_adjust (void *addr, size_t len)
{
#ifdef __BIG_ENDIAN__
  return addr + (8 - len);
#else
  return addr;
#endif
}



#ifdef __hpux
#define stf_spill(addr, value)
#else
#define stf_spill(addr, value)	\
  __asm__ ("stf.spill %0 = %1%P0" : "=m" (*addr) : "f"(value));
#endif



#ifdef __hpux
#define ldf_fill(result, addr)
#else
#define ldf_fill(result, addr)	\
  __asm__ ("ldf.fill %0 = %1%P1" : "=f"(result) : "m"(*addr));
#endif



static size_t
hfa_type_size (int type)
{
  switch (type)
    {
    case SFFI_IA64_TYPE_HFA_FLOAT:
      return sizeof(float);
    case SFFI_IA64_TYPE_HFA_DOUBLE:
      return sizeof(double);
    case SFFI_IA64_TYPE_HFA_LDOUBLE:
      return sizeof(__float80);
    default:
      abort ();
    }
}



static void
hfa_type_load (fpreg *fpaddr, int type, void *addr)
{
  switch (type)
    {
    case SFFI_IA64_TYPE_HFA_FLOAT:
      stf_spill (fpaddr, *(float *) addr);
      return;
    case SFFI_IA64_TYPE_HFA_DOUBLE:
      stf_spill (fpaddr, *(double *) addr);
      return;
    case SFFI_IA64_TYPE_HFA_LDOUBLE:
      stf_spill (fpaddr, *(__float80 *) addr);
      return;
    default:
      abort ();
    }
}



static void
hfa_type_store (int type, void *addr, fpreg *fpaddr)
{
  switch (type)
    {
    case SFFI_IA64_TYPE_HFA_FLOAT:
      {
	float result;
	ldf_fill (result, fpaddr);
	*(float *) addr = result;
	break;
      }
    case SFFI_IA64_TYPE_HFA_DOUBLE:
      {
	double result;
	ldf_fill (result, fpaddr);
	*(double *) addr = result;
	break;
      }
    case SFFI_IA64_TYPE_HFA_LDOUBLE:
      {
	__float80 result;
	ldf_fill (result, fpaddr);
	*(__float80 *) addr = result;
	break;
      }
    default:
      abort ();
    }
}



static int
hfa_element_type (sffi_type *type, int nested)
{
  int element = SFFI_TYPE_VOID;

  switch (type->type)
    {
    case SFFI_TYPE_FLOAT:
      
      if (nested)
	element = SFFI_IA64_TYPE_HFA_FLOAT;
      break;

    case SFFI_TYPE_DOUBLE:
      
      if (nested)
	element = SFFI_IA64_TYPE_HFA_DOUBLE;
      break;

    case SFFI_TYPE_LONGDOUBLE:
      
      if (LDBL_MANT_DIG == 64 && nested)
	element = SFFI_IA64_TYPE_HFA_LDOUBLE;
      break;

    case SFFI_TYPE_STRUCT:
      {
	sffi_type **ptr = &type->elements[0];

	for (ptr = &type->elements[0]; *ptr ; ptr++)
	  {
	    int sub_element = hfa_element_type (*ptr, 1);
	    if (sub_element == SFFI_TYPE_VOID)
	      return SFFI_TYPE_VOID;

	    if (element == SFFI_TYPE_VOID)
	      element = sub_element;
	    else if (element != sub_element)
	      return SFFI_TYPE_VOID;
	  }
      }
      break;

    default:
      return SFFI_TYPE_VOID;
    }

  return element;
}




static sffi_status
sffi_prep_cif_machdep_core(sffi_cif *cif)
{
  int flags;

  
  cif->bytes += offsetof(struct ia64_args, gp_regs[0]);
  if (cif->bytes < sizeof(struct ia64_args))
    cif->bytes = sizeof(struct ia64_args);

  
  flags = cif->rtype->type;
  switch (cif->rtype->type)
    {
    case SFFI_TYPE_LONGDOUBLE:
      
      if (LDBL_MANT_DIG != 64)
	flags = SFFI_IA64_TYPE_SMALL_STRUCT | (16 << 8);
      break;

    case SFFI_TYPE_STRUCT:
      {
        size_t size = cif->rtype->size;
  	int hfa_type = hfa_element_type (cif->rtype, 0);

	if (hfa_type != SFFI_TYPE_VOID)
	  {
	    size_t nelts = size / hfa_type_size (hfa_type);
	    if (nelts <= 8)
	      flags = hfa_type | (size << 8);
	  }
	else
	  {
	    if (size <= 32)
	      flags = SFFI_IA64_TYPE_SMALL_STRUCT | (size << 8);
	  }
      }
      break;

    default:
      break;
    }
  cif->flags = flags;

  return SFFI_OK;
}

sffi_status
sffi_prep_cif_machdep(sffi_cif *cif)
{
  cif->nfixedargs = cif->nargs;
  return sffi_prep_cif_machdep_core(cif);
}

sffi_status
sffi_prep_cif_machdep_var(sffi_cif *cif,
			 unsigned int nfixedargs,
			 unsigned int ntotalargs MAYBE_UNUSED)
{
  cif->nfixedargs = nfixedargs;
  return sffi_prep_cif_machdep_core(cif);
}

extern int sffi_call_unix (struct ia64_args *, PTR64, void (*)(void), UINT64);

void
sffi_call(sffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  struct ia64_args *stack;
  long i, avn, gpcount, fpcount;
  sffi_type **p_arg;

  SFFI_ASSERT (cif->abi == SFFI_UNIX);

  
  if (rvalue == NULL && cif->rtype->type != SFFI_TYPE_VOID)
    rvalue = alloca (cif->rtype->size);
    
  
  stack = alloca (cif->bytes);

  gpcount = fpcount = 0;
  avn = cif->nargs;
  for (i = 0, p_arg = cif->arg_types; i < avn; i++, p_arg++)
    {
      switch ((*p_arg)->type)
	{
	case SFFI_TYPE_SINT8:
	  stack->gp_regs[gpcount++] = *(SINT8 *)avalue[i];
	  break;
	case SFFI_TYPE_UINT8:
	  stack->gp_regs[gpcount++] = *(UINT8 *)avalue[i];
	  break;
	case SFFI_TYPE_SINT16:
	  stack->gp_regs[gpcount++] = *(SINT16 *)avalue[i];
	  break;
	case SFFI_TYPE_UINT16:
	  stack->gp_regs[gpcount++] = *(UINT16 *)avalue[i];
	  break;
	case SFFI_TYPE_SINT32:
	  stack->gp_regs[gpcount++] = *(SINT32 *)avalue[i];
	  break;
	case SFFI_TYPE_UINT32:
	  stack->gp_regs[gpcount++] = *(UINT32 *)avalue[i];
	  break;
	case SFFI_TYPE_SINT64:
	case SFFI_TYPE_UINT64:
	  stack->gp_regs[gpcount++] = *(UINT64 *)avalue[i];
	  break;

	case SFFI_TYPE_POINTER:
	  stack->gp_regs[gpcount++] = (UINT64)(PTR64) *(void **)avalue[i];
	  break;

	case SFFI_TYPE_FLOAT:
	  if (gpcount < 8 && fpcount < 8)
	    stf_spill (&stack->fp_regs[fpcount++], *(float *)avalue[i]);
	  {
	    UINT32 tmp;
	    memcpy (&tmp, avalue[i], sizeof (UINT32));
	    stack->gp_regs[gpcount++] = tmp;
	  }
	  break;

	case SFFI_TYPE_DOUBLE:
	  if (gpcount < 8 && fpcount < 8)
	    stf_spill (&stack->fp_regs[fpcount++], *(double *)avalue[i]);
	  memcpy (&stack->gp_regs[gpcount++], avalue[i], sizeof (UINT64));
	  break;

	case SFFI_TYPE_LONGDOUBLE:
	  if (gpcount & 1)
	    gpcount++;
	  if (LDBL_MANT_DIG == 64 && gpcount < 8 && fpcount < 8)
	    stf_spill (&stack->fp_regs[fpcount++], *(__float80 *)avalue[i]);
	  memcpy (&stack->gp_regs[gpcount], avalue[i], 16);
	  gpcount += 2;
	  break;

	case SFFI_TYPE_STRUCT:
	  {
	    size_t size = (*p_arg)->size;
	    size_t align = (*p_arg)->alignment;
	    int hfa_type = hfa_element_type (*p_arg, 0);

	    SFFI_ASSERT (align <= 16);
	    if (align == 16 && (gpcount & 1))
	      gpcount++;

	    if (hfa_type != SFFI_TYPE_VOID)
	      {
		size_t hfa_size = hfa_type_size (hfa_type);
		size_t offset = 0;
		size_t gp_offset = gpcount * 8;

		while (fpcount < 8
		       && offset < size
		       && gp_offset < 8 * 8)
		  {
		    hfa_type_load (&stack->fp_regs[fpcount], hfa_type,
				   avalue[i] + offset);
		    offset += hfa_size;
		    gp_offset += hfa_size;
		    fpcount += 1;
		  }
	      }

	    memcpy (&stack->gp_regs[gpcount], avalue[i], size);
	    gpcount += (size + 7) / 8;
	  }
	  break;

	default:
	  abort ();
	}
    }

  sffi_call_unix (stack, rvalue, fn, cif->flags);
}



extern void sffi_closure_unix ();

sffi_status
sffi_prep_closure_loc (sffi_closure* closure,
		      sffi_cif* cif,
		      void (*fun)(sffi_cif*,void*,void**,void*),
		      void *user_data,
		      void *codeloc)
{
  
  struct ia64_fd
  {
    UINT64 code_pointer;
    UINT64 gp;
  };

  struct sffi_ia64_trampoline_struct
  {
    UINT64 code_pointer;	
    UINT64 fake_gp;		
    UINT64 real_gp;		
  };

  struct sffi_ia64_trampoline_struct *tramp;
  struct ia64_fd *fd;

  if (cif->abi != SFFI_UNIX)
    return SFFI_BAD_ABI;

  tramp = (struct sffi_ia64_trampoline_struct *)closure->tramp;
  fd = (struct ia64_fd *)(void *)sffi_closure_unix;

  tramp->code_pointer = fd->code_pointer;
  tramp->real_gp = fd->gp;
  tramp->fake_gp = (UINT64)(PTR64)codeloc;
  closure->cif = cif;
  closure->user_data = user_data;
  closure->fun = fun;

  return SFFI_OK;
}


UINT64
sffi_closure_unix_inner (sffi_closure *closure, struct ia64_args *stack,
			void *rvalue, void *r8)
{
  sffi_cif *cif;
  void **avalue;
  sffi_type **p_arg;
  long i, avn, gpcount, fpcount, nfixedargs;

  cif = closure->cif;
  avn = cif->nargs;
  nfixedargs = cif->nfixedargs;
  avalue = alloca (avn * sizeof (void *));

  
  if (cif->flags == SFFI_TYPE_STRUCT)
    rvalue = r8;

  gpcount = fpcount = 0;
  for (i = 0, p_arg = cif->arg_types; i < avn; i++, p_arg++)
    {
      int named = i < nfixedargs;
      switch ((*p_arg)->type)
	{
	case SFFI_TYPE_SINT8:
	case SFFI_TYPE_UINT8:
	  avalue[i] = endian_adjust(&stack->gp_regs[gpcount++], 1);
	  break;
	case SFFI_TYPE_SINT16:
	case SFFI_TYPE_UINT16:
	  avalue[i] = endian_adjust(&stack->gp_regs[gpcount++], 2);
	  break;
	case SFFI_TYPE_SINT32:
	case SFFI_TYPE_UINT32:
	  avalue[i] = endian_adjust(&stack->gp_regs[gpcount++], 4);
	  break;
	case SFFI_TYPE_SINT64:
	case SFFI_TYPE_UINT64:
	  avalue[i] = &stack->gp_regs[gpcount++];
	  break;
	case SFFI_TYPE_POINTER:
	  avalue[i] = endian_adjust(&stack->gp_regs[gpcount++], sizeof(void*));
	  break;

	case SFFI_TYPE_FLOAT:
	  if (named && gpcount < 8 && fpcount < 8)
	    {
	      fpreg *addr = &stack->fp_regs[fpcount++];
	      float result;
	      avalue[i] = addr;
	      ldf_fill (result, addr);
	      *(float *)addr = result;
	    }
	  else
	    avalue[i] = endian_adjust(&stack->gp_regs[gpcount], 4);
	  gpcount++;
	  break;

	case SFFI_TYPE_DOUBLE:
	  if (named && gpcount < 8 && fpcount < 8)
	    {
	      fpreg *addr = &stack->fp_regs[fpcount++];
	      double result;
	      avalue[i] = addr;
	      ldf_fill (result, addr);
	      *(double *)addr = result;
	    }
	  else
	    avalue[i] = &stack->gp_regs[gpcount];
	  gpcount++;
	  break;

	case SFFI_TYPE_LONGDOUBLE:
	  if (gpcount & 1)
	    gpcount++;
	  if (LDBL_MANT_DIG == 64 && named && gpcount < 8 && fpcount < 8)
	    {
	      fpreg *addr = &stack->fp_regs[fpcount++];
	      __float80 result;
	      avalue[i] = addr;
	      ldf_fill (result, addr);
	      *(__float80 *)addr = result;
	    }
	  else
	    avalue[i] = &stack->gp_regs[gpcount];
	  gpcount += 2;
	  break;

	case SFFI_TYPE_STRUCT:
	  {
	    size_t size = (*p_arg)->size;
	    size_t align = (*p_arg)->alignment;
	    int hfa_type = hfa_element_type (*p_arg, 0);

	    SFFI_ASSERT (align <= 16);
	    if (align == 16 && (gpcount & 1))
	      gpcount++;

	    if (hfa_type != SFFI_TYPE_VOID)
	      {
		size_t hfa_size = hfa_type_size (hfa_type);
		size_t offset = 0;
		size_t gp_offset = gpcount * 8;
		void *addr = alloca (size);

		avalue[i] = addr;

		while (fpcount < 8
		       && offset < size
		       && gp_offset < 8 * 8)
		  {
		    hfa_type_store (hfa_type, addr + offset,
				    &stack->fp_regs[fpcount]);
		    offset += hfa_size;
		    gp_offset += hfa_size;
		    fpcount += 1;
		  }

		if (offset < size)
		  memcpy (addr + offset, (char *)stack->gp_regs + gp_offset,
			  size - offset);
	      }
	    else
	      avalue[i] = &stack->gp_regs[gpcount];

	    gpcount += (size + 7) / 8;
	  }
	  break;

	default:
	  abort ();
	}
    }

  closure->fun (cif, rvalue, avalue, closure->user_data);

  return cif->flags;
}
