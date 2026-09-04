

#include <sffi.h>
#include <sffi_common.h>

#include <stdlib.h>
#include <stdio.h>

#define ROUND_UP(v, a)  (((size_t)(v) + (a) - 1) & ~((a) - 1))

#define MIN_STACK_SIZE  64
#define FIRST_ARG_SLOT  9
#define DEBUG_LEVEL   0

#define fldw(addr, fpreg) \
  __asm__ volatile ("fldw 0(%0), %%" #fpreg "L" : : "r"(addr) : #fpreg)
#define fstw(fpreg, addr) \
  __asm__ volatile ("fstw %%" #fpreg "L, 0(%0)" : : "r"(addr))
#define fldd(addr, fpreg) \
  __asm__ volatile ("fldd 0(%0), %%" #fpreg : : "r"(addr) : #fpreg)
#define fstd(fpreg, addr) \
  __asm__ volatile ("fstd %%" #fpreg "L, 0(%0)" : : "r"(addr))

#define debug(lvl, x...) do { if (lvl <= DEBUG_LEVEL) { printf(x); } } while (0)

static inline int sffi_struct_type(sffi_type *t)
{
  size_t sz = t->size;

  

  if (sz <= 8)
    return -sz;
  else
    return SFFI_TYPE_STRUCT; 
}





void sffi_prep_args_pa32(UINT32 *stack, extended_cif *ecif, unsigned bytes)
{
  register unsigned int i;
  register sffi_type **p_arg;
  register void **p_argv;
  unsigned int slot = FIRST_ARG_SLOT;
  char *dest_cpy;
  size_t len;

  debug(1, "%s: stack = %p, ecif = %p, bytes = %u\n", __FUNCTION__, stack,
	ecif, bytes);

  p_arg = ecif->cif->arg_types;
  p_argv = ecif->avalue;

  for (i = 0; i < ecif->cif->nargs; i++)
    {
      int type = (*p_arg)->type;

      switch (type)
	{
	case SFFI_TYPE_SINT8:
	  *(SINT32 *)(stack - slot) = *(SINT8 *)(*p_argv);
	  break;

	case SFFI_TYPE_UINT8:
	  *(UINT32 *)(stack - slot) = *(UINT8 *)(*p_argv);
	  break;

	case SFFI_TYPE_SINT16:
	  *(SINT32 *)(stack - slot) = *(SINT16 *)(*p_argv);
	  break;

	case SFFI_TYPE_UINT16:
	  *(UINT32 *)(stack - slot) = *(UINT16 *)(*p_argv);
	  break;

	case SFFI_TYPE_UINT32:
	case SFFI_TYPE_SINT32:
	case SFFI_TYPE_POINTER:
	  debug(3, "Storing UINT32 %u in slot %u\n", *(UINT32 *)(*p_argv),
		slot);
	  *(UINT32 *)(stack - slot) = *(UINT32 *)(*p_argv);
	  break;

	case SFFI_TYPE_UINT64:
	case SFFI_TYPE_SINT64:
	  
	  slot += (slot & 1) ? 1 : 2;
	  *(UINT64 *)(stack - slot) = *(UINT64 *)(*p_argv);
	  break;

	case SFFI_TYPE_FLOAT:
	  
	  debug(3, "Storing UINT32(float) in slot %u\n", slot);
	  *(UINT32 *)(stack - slot) = *(UINT32 *)(*p_argv);
	  switch (slot - FIRST_ARG_SLOT)
	    {
	    
	    case 0: fldw(stack - slot, fr4); break;
	    case 1: fldw(stack - slot, fr5); break;
	    case 2: fldw(stack - slot, fr6); break;
	    case 3: fldw(stack - slot, fr7); break;
	    }
	  break;

	case SFFI_TYPE_DOUBLE:
	  
	  slot += (slot & 1) ? 1 : 2;
	  debug(3, "Storing UINT64(double) at slot %u\n", slot);
	  *(UINT64 *)(stack - slot) = *(UINT64 *)(*p_argv);
	  switch (slot - FIRST_ARG_SLOT)
	    {
	      
	      case 1: fldd(stack - slot, fr5); break;
	      case 3: fldd(stack - slot, fr7); break;
	    }
	  break;

#ifdef PA_HPUX
	case SFFI_TYPE_LONGDOUBLE:
	  
	  *(UINT32 *)(stack - slot) = (UINT32)(*p_argv);
	  break;
#endif

	case SFFI_TYPE_STRUCT:

	  

	  len = (*p_arg)->size;
	  if (len <= 4)
	    {
	      dest_cpy = (char *)(stack - slot) + 4 - len;
	      memcpy(dest_cpy, (char *)*p_argv, len);
	    }
	  else if (len <= 8)
	    {
	      slot += (slot & 1) ? 1 : 2;
	      dest_cpy = (char *)(stack - slot) + 8 - len;
	      memcpy(dest_cpy, (char *)*p_argv, len);
	    }
	  else
	    *(UINT32 *)(stack - slot) = (UINT32)(*p_argv);
	  break;

	default:
	  SFFI_ASSERT(0);
	}

      slot++;
      p_arg++;
      p_argv++;
    }

  
  {
    unsigned int n;

    debug(5, "Stack setup:\n");
    for (n = 0; n < (bytes + 3) / 4; n++)
      {
	if ((n%4) == 0) { debug(5, "\n%08x: ", (unsigned int)(stack - n)); }
	debug(5, "%08x ", *(stack - n));
      }
    debug(5, "\n");
  }

  SFFI_ASSERT(slot * 4 <= bytes);

  return;
}

static void sffi_size_stack_pa32(sffi_cif *cif)
{
  sffi_type **ptr;
  int i;
  int z = 0; 

  for (ptr = cif->arg_types, i = 0; i < cif->nargs; ptr++, i++)
    {
      int type = (*ptr)->type;

      switch (type)
	{
	case SFFI_TYPE_DOUBLE:
	case SFFI_TYPE_UINT64:
	case SFFI_TYPE_SINT64:
	  z += 2 + (z & 1); 
	  break;

#ifdef PA_HPUX
	case SFFI_TYPE_LONGDOUBLE:
	  z += 1; 
	  break;
#endif

	case SFFI_TYPE_STRUCT:
	  
	  {
	    size_t len = (*ptr)->size;
	    if (len <= 4 || len > 8)
	      z += 1;
	    else
	      z += 2 + (z & 1);
	  }
	  break;

	default: 
	  z++;
	}
    }

  
  if (z <= 6)
    cif->bytes = MIN_STACK_SIZE; 
  else
    cif->bytes = 64 + ROUND_UP((z - 6) * sizeof(UINT32), MIN_STACK_SIZE);

  debug(3, "Calculated stack size is %u bytes\n", cif->bytes);
}


sffi_status sffi_prep_cif_machdep(sffi_cif *cif)
{
  
  switch (cif->rtype->type)
    {
    case SFFI_TYPE_VOID:
    case SFFI_TYPE_FLOAT:
    case SFFI_TYPE_DOUBLE:
      cif->flags = (unsigned) cif->rtype->type;
      break;

#ifdef PA_HPUX
    case SFFI_TYPE_LONGDOUBLE:
      
      cif->flags = SFFI_TYPE_STRUCT;
      break;
#endif

    case SFFI_TYPE_STRUCT:
      
      cif->flags = sffi_struct_type(cif->rtype);
      break;

    case SFFI_TYPE_UINT64:
    case SFFI_TYPE_SINT64:
      cif->flags = SFFI_TYPE_UINT64;
      break;

    default:
      cif->flags = SFFI_TYPE_INT;
      break;
    }

  
  switch (cif->abi)
    {
    case SFFI_PA32:
      sffi_size_stack_pa32(cif);
      break;

    default:
      SFFI_ASSERT(0);
      break;
    }

  return SFFI_OK;
}

extern void sffi_call_pa32(void (*)(UINT32 *, extended_cif *, unsigned),
			  extended_cif *, unsigned, unsigned, unsigned *,
			  void (*fn)(void));

void sffi_call(sffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  extended_cif ecif;
  size_t i, nargs = cif->nargs;
  sffi_type **arg_types = cif->arg_types;
  void **avalue_copy = NULL;

  ecif.cif = cif;

  
  for (i = 0; i < nargs; i++)
    {
      sffi_type *at = arg_types[i];
      int size = at->size;
      if (at->type == SFFI_TYPE_STRUCT && size > 8)
	{
	  char *argcopy = alloca (size);
	  if (avalue_copy == NULL)
	    {
	      avalue_copy = alloca (nargs * sizeof (void *));
	      memcpy (avalue_copy, avalue, nargs * sizeof (void *));
	      avalue = avalue_copy;
	    }
	  memcpy (argcopy, avalue[i], size);
	  avalue[i] = argcopy;
	}
    }
  ecif.avalue = avalue;

  

  if (rvalue == NULL
#ifdef PA_HPUX
      && (cif->rtype->type == SFFI_TYPE_STRUCT
	  || cif->rtype->type == SFFI_TYPE_LONGDOUBLE))
#else
      && cif->rtype->type == SFFI_TYPE_STRUCT)
#endif
    {
      ecif.rvalue = alloca(cif->rtype->size);
    }
  else
    ecif.rvalue = rvalue;


  switch (cif->abi)
    {
    case SFFI_PA32:
      debug(3, "Calling sffi_call_pa32: ecif=%p, bytes=%u, flags=%u, rvalue=%p, fn=%p\n", &ecif, cif->bytes, cif->flags, ecif.rvalue, (void *)fn);
      sffi_call_pa32(sffi_prep_args_pa32, &ecif, cif->bytes,
		     cif->flags, ecif.rvalue, fn);
      break;

    default:
      SFFI_ASSERT(0);
      break;
    }
}

#if SFFI_CLOSURES

sffi_status sffi_closure_inner_pa32(sffi_closure *closure, UINT32 *stack)
{
  sffi_cif *cif;
  void **avalue;
  void *rvalue;
  
  union { double rd; UINT32 ret[2]; } u;
  sffi_type **p_arg;
  char *tmp;
  int i, avn;
  unsigned int slot = FIRST_ARG_SLOT;
  register UINT32 r28 __asm__("r28");

  cif = closure->cif;

  
  if (cif->flags == SFFI_TYPE_STRUCT)
    rvalue = (void *)r28;
  else
    rvalue = &u;

  avalue = (void **)alloca(cif->nargs * SFFI_SIZEOF_ARG);
  avn = cif->nargs;
  p_arg = cif->arg_types;

  for (i = 0; i < avn; i++)
    {
      int type = (*p_arg)->type;

      switch (type)
	{
	case SFFI_TYPE_SINT8:
	case SFFI_TYPE_UINT8:
	case SFFI_TYPE_SINT16:
	case SFFI_TYPE_UINT16:
	case SFFI_TYPE_SINT32:
	case SFFI_TYPE_UINT32:
	case SFFI_TYPE_POINTER:
	  avalue[i] = (char *)(stack - slot) + sizeof(UINT32) - (*p_arg)->size;
	  break;

	case SFFI_TYPE_SINT64:
	case SFFI_TYPE_UINT64:
	  slot += (slot & 1) ? 1 : 2;
	  avalue[i] = (void *)(stack - slot);
	  break;

	case SFFI_TYPE_FLOAT:
#ifdef PA_LINUX
	  
	  switch (slot - FIRST_ARG_SLOT)
	    {
	    case 0: fstw(fr4, (void *)(stack - slot)); break;
	    case 1: fstw(fr5, (void *)(stack - slot)); break;
	    case 2: fstw(fr6, (void *)(stack - slot)); break;
	    case 3: fstw(fr7, (void *)(stack - slot)); break;
	    }
#endif
	  avalue[i] = (void *)(stack - slot);
	  break;

	case SFFI_TYPE_DOUBLE:
	  slot += (slot & 1) ? 1 : 2;
#ifdef PA_LINUX
	  
	  switch (slot - FIRST_ARG_SLOT)
	    {
	    case 1: fstd(fr5, (void *)(stack - slot)); break;
	    case 3: fstd(fr7, (void *)(stack - slot)); break;
	    }
#endif
	  avalue[i] = (void *)(stack - slot);
	  break;

#ifdef PA_HPUX
	case SFFI_TYPE_LONGDOUBLE:
	  
	  avalue[i] = (void *) *(stack - slot);
	  break;
#endif

	case SFFI_TYPE_STRUCT:
	  
	  if((*p_arg)->size <= 4)
	    {
	      avalue[i] = (void *)(stack - slot) + sizeof(UINT32) -
		(*p_arg)->size;
	    }
	  else if ((*p_arg)->size <= 8)
	    {
	      slot += (slot & 1) ? 1 : 2;
	      avalue[i] = (void *)(stack - slot) + sizeof(UINT64) -
		(*p_arg)->size;
	    }
	  else
	    avalue[i] = (void *) *(stack - slot);
	  break;

	default:
	  SFFI_ASSERT(0);
	}

      slot++;
      p_arg++;
    }

  
  (closure->fun) (cif, rvalue, avalue, closure->user_data);

  debug(3, "after calling function, ret[0] = %08x, ret[1] = %08x\n", u.ret[0],
	u.ret[1]);

  
  switch (cif->flags)
    {
    case SFFI_TYPE_UINT8:
      *(stack - FIRST_ARG_SLOT) = (UINT8)u.ret[0];
      break;
    case SFFI_TYPE_SINT8:
      *(stack - FIRST_ARG_SLOT) = (SINT8)u.ret[0];
      break;
    case SFFI_TYPE_UINT16:
      *(stack - FIRST_ARG_SLOT) = (UINT16)u.ret[0];
      break;
    case SFFI_TYPE_SINT16:
      *(stack - FIRST_ARG_SLOT) = (SINT16)u.ret[0];
      break;
    case SFFI_TYPE_INT:
    case SFFI_TYPE_SINT32:
    case SFFI_TYPE_UINT32:
      *(stack - FIRST_ARG_SLOT) = u.ret[0];
      break;
    case SFFI_TYPE_SINT64:
    case SFFI_TYPE_UINT64:
      *(stack - FIRST_ARG_SLOT) = u.ret[0];
      *(stack - FIRST_ARG_SLOT - 1) = u.ret[1];
      break;

    case SFFI_TYPE_DOUBLE:
      fldd(rvalue, fr4);
      break;

    case SFFI_TYPE_FLOAT:
      fldw(rvalue, fr4);
      break;

    case SFFI_TYPE_STRUCT:
      
      break;

    case SFFI_TYPE_SMALL_STRUCT1:
    case SFFI_TYPE_SMALL_STRUCT2:
    case SFFI_TYPE_SMALL_STRUCT3:
    case SFFI_TYPE_SMALL_STRUCT4:
      tmp = (void*)(stack -  FIRST_ARG_SLOT);
      tmp += 4 - cif->rtype->size;
      memcpy((void*)tmp, &u, cif->rtype->size);
      break;

    case SFFI_TYPE_SMALL_STRUCT5:
    case SFFI_TYPE_SMALL_STRUCT6:
    case SFFI_TYPE_SMALL_STRUCT7:
    case SFFI_TYPE_SMALL_STRUCT8:
      {
	unsigned int ret2[2];
	int off;

	
	switch (cif->flags)
	  {
	    case SFFI_TYPE_SMALL_STRUCT5: off = 3; break;
	    case SFFI_TYPE_SMALL_STRUCT6: off = 2; break;
	    case SFFI_TYPE_SMALL_STRUCT7: off = 1; break;
	    default: off = 0; break;
	  }

	memset (ret2, 0, sizeof (ret2));
	memcpy ((char *)ret2 + off, &u, 8 - off);

	*(stack - FIRST_ARG_SLOT) = ret2[0];
	*(stack - FIRST_ARG_SLOT - 1) = ret2[1];
      }
      break;

    case SFFI_TYPE_POINTER:
    case SFFI_TYPE_VOID:
      break;

    default:
      debug(0, "assert with cif->flags: %d\n",cif->flags);
      SFFI_ASSERT(0);
      break;
    }
  return SFFI_OK;
}



extern void sffi_closure_pa32(void);

sffi_status
sffi_prep_closure_loc (sffi_closure* closure,
		      sffi_cif* cif,
		      void (*fun)(sffi_cif*,void*,void**,void*),
		      void *user_data,
		      void *codeloc)
{
  
  struct pa32_fd
  {
    UINT32 code_pointer;
    UINT32 gp;
  };

  struct sffi_pa32_trampoline_struct
  {
     UINT32 code_pointer;        
     UINT32 fake_gp;             
     UINT32 real_gp;             
  };

  struct sffi_pa32_trampoline_struct *tramp;
  struct pa32_fd *fd;

  if (cif->abi != SFFI_PA32)
    return SFFI_BAD_ABI;

  
  fd = (struct pa32_fd *)((UINT32)sffi_closure_pa32 & ~3);

  
  tramp = (struct sffi_pa32_trampoline_struct *)closure->tramp;
  tramp->code_pointer = fd->code_pointer;
  tramp->fake_gp = (UINT32)codeloc & ~3;
  tramp->real_gp = fd->gp;

  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;

  return SFFI_OK;
}
#endif
