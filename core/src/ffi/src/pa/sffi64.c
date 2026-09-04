

#include <sffi.h>
#include <sffi_common.h>

#include <stdlib.h>
#include <stdio.h>

#define ROUND_UP(v, a)  (((size_t)(v) + (a) - 1) & ~((a) - 1))

#define FIRST_ARG_SLOT  0
#define DEBUG_LEVEL   0

#define fldw(addr, fpreg) \
  __asm__ volatile ("fldw 4(%0), %%" #fpreg "R" : : "r"(addr) : #fpreg)
#define fstw(fpreg, addr) \
  __asm__ volatile ("fstw %%" #fpreg "R, 4(%0)" : : "r"(addr))
#define fldd(addr, fpreg) \
  __asm__ volatile ("fldd 0(%0), %%" #fpreg "L" : : "r"(addr) : #fpreg)
#define fstd(fpreg, addr) \
  __asm__ volatile ("fstd %%" #fpreg "L, 0(%0)" : : "r"(addr))

#define debug(lvl, x...) do { if (lvl <= DEBUG_LEVEL) { printf(x); } } while (0)

static inline int sffi_struct_type(sffi_type *t)
{
  int sz = t->size;

  

  if (sz <= 16)
    return -sz;
  else
    return SFFI_TYPE_STRUCT;
}





void sffi_prep_args_pa64(UINT64 *stack, extended_cif *ecif, unsigned bytes)
{
  register unsigned int i;
  register sffi_type **p_arg;
  register void **p_argv;
  unsigned int slot = FIRST_ARG_SLOT;
  size_t len;

  debug(1, "%s: stack = %p, ecif = %p, bytes = %u\n", __FUNCTION__, stack,
	ecif, bytes);

  p_arg = ecif->cif->arg_types;
  p_argv = ecif->avalue;

  for (i = 0; i < ecif->cif->nargs; i++)
    {
      int type = (*p_arg)->type;

      len = (*p_arg)->size;

      switch (type)
	{
	case SFFI_TYPE_SINT8:
	  *(SINT64 *)(stack + slot) = *(SINT8 *)(*p_argv);
	  break;

	case SFFI_TYPE_UINT8:
	  *(UINT64 *)(stack + slot) = *(UINT8 *)(*p_argv);
	  break;

	case SFFI_TYPE_SINT16:
	  *(SINT64 *)(stack + slot) = *(SINT16 *)(*p_argv);
	  break;

	case SFFI_TYPE_UINT16:
	  *(UINT64 *)(stack + slot) = *(UINT16 *)(*p_argv);
	  break;

	case SFFI_TYPE_SINT32:
	  *(SINT64 *)(stack + slot) = *(SINT32 *)(*p_argv);
	  break;

	case SFFI_TYPE_UINT32:
	  *(UINT64 *)(stack + slot) = *(UINT32 *)(*p_argv);
	  break;

	case SFFI_TYPE_UINT64:
	case SFFI_TYPE_SINT64:
	case SFFI_TYPE_POINTER:
	  debug(3, "Storing UINT64 %lu in slot %u\n", *(UINT64 *)(*p_argv),
		slot);
	  *(UINT64 *)(stack + slot) = *(UINT64 *)(*p_argv);
	  break;

	case SFFI_TYPE_FLOAT:
	  
	  debug(3, "Storing UINT32(float) in slot %u\n", slot);
	  *(UINT64 *)(stack + slot) = *(UINT32 *)(*p_argv);
	  switch (slot - FIRST_ARG_SLOT)
	    {
	    
	    case 0: fldw(stack + slot, fr4); break;
	    case 1: fldw(stack + slot, fr5); break;
	    case 2: fldw(stack + slot, fr6); break;
	    case 3: fldw(stack + slot, fr7); break;
	    case 4: fldw(stack + slot, fr8); break;
	    case 5: fldw(stack + slot, fr9); break;
	    case 6: fldw(stack + slot, fr10); break;
	    case 7: fldw(stack + slot, fr11); break;
	    }
	  break;

	case SFFI_TYPE_DOUBLE:
	  debug(3, "Storing UINT64(double) at slot %u\n", slot);
	  *(UINT64 *)(stack + slot) = *(UINT64 *)(*p_argv);
	  switch (slot - FIRST_ARG_SLOT)
	    {
	    
	    case 0: fldd(stack + slot, fr4); break;
	    case 1: fldd(stack + slot, fr5); break;
	    case 2: fldd(stack + slot, fr6); break;
	    case 3: fldd(stack + slot, fr7); break;
	    case 4: fldd(stack + slot, fr8); break;
	    case 5: fldd(stack + slot, fr9); break;
	    case 6: fldd(stack + slot, fr10); break;
	    case 7: fldd(stack + slot, fr11); break;
	    }
	  break;

#ifdef PA64_HPUX
	case SFFI_TYPE_LONGDOUBLE:
	  
	  slot += (slot & 1);
	  *(UINT64 *)(stack + slot) = *(UINT64 *)(*p_argv);
	  *(UINT64 *)(stack + slot + 1) = *(UINT64 *)(*p_argv + 8);
	  break;
#endif

	case SFFI_TYPE_STRUCT:
	  
	  if (len > 8)
	    slot += (slot & 1);
	  memcpy((char *)(stack + slot), (char *)*p_argv, len);
	  break;

	default:
	  SFFI_ASSERT(0);
	}

      slot += ROUND_UP (len, 8) >> 3;
      p_arg++;
      p_argv++;
    }

  SFFI_ASSERT(slot * 8 <= bytes);

  return;
}

static void sffi_size_stack_pa64(sffi_cif *cif)
{
  sffi_type **ptr;
  int i;
  int z = 0; 

  for (ptr = cif->arg_types, i = 0; i < cif->nargs; ptr++, i++)
    {
      int type = (*ptr)->type;
      int size = (*ptr)->size;

      switch (type)
	{
#ifdef PA64_HPUX
	case SFFI_TYPE_LONGDOUBLE:
	  z += 2 + (z & 1);
	  break;
#endif

	case SFFI_TYPE_STRUCT:
	  if (size > 8)
	    z += (z & 1);
	  z += ROUND_UP (size, 8) >> 3;
	  break;

	default: 
	  z++;
	}
    }

  
  if (z <= 8)
    z = 8;
  else
    z += (z & 1);

  
  cif->bytes = z * 8 + 64;
  debug(3, "Calculated stack size is %u bytes\n", cif->bytes);
}


sffi_status sffi_prep_cif_machdep(sffi_cif *cif)
{
  
  switch (cif->rtype->type)
    {
    case SFFI_TYPE_COMPLEX:
    case SFFI_TYPE_STRUCT:
      
      cif->flags = sffi_struct_type(cif->rtype);
      break;

    default:
      cif->flags = (unsigned) cif->rtype->type;
      break;
    }

  
  switch (cif->abi)
    {
    case SFFI_PA64:
      sffi_size_stack_pa64(cif);
      break;

    default:
      SFFI_ASSERT(0);
      break;
    }

  return SFFI_OK;
}

extern void sffi_call_pa64(void (*)(UINT64 *, extended_cif *, unsigned),
			  extended_cif *, unsigned, unsigned, unsigned *,
			  void (*fn)(void));

void sffi_call(sffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;

  

  if (rvalue == NULL
      && (cif->rtype->type == SFFI_TYPE_STRUCT
	  || cif->rtype->type == SFFI_TYPE_COMPLEX)
      && cif->rtype->size > 16)
    ecif.rvalue = alloca(ROUND_UP (cif->rtype->size, 16));
  else
    ecif.rvalue = rvalue;


  switch (cif->abi)
    {
    case SFFI_PA64:
      debug(3, "Calling sffi_call_pa64: ecif=%p, bytes=%u, flags=%u, rvalue=%p, fn=%p\n", &ecif, cif->bytes, cif->flags, ecif.rvalue, (void *)fn);
      sffi_call_pa64(sffi_prep_args_pa64, &ecif, cif->bytes,
		     cif->flags, ecif.rvalue, fn);
      break;

    default:
      SFFI_ASSERT(0);
      break;
    }
}

#if SFFI_CLOSURES

sffi_status sffi_closure_inner_pa64(sffi_closure *closure, UINT64 *stack)
{
  sffi_cif *cif;
  void **avalue;
  void *rvalue;
  
  union { long double rld; UINT64 ret[2]; } u;
  sffi_type **p_arg;
  char *tmp;
  int i, avn;
  unsigned int slot = FIRST_ARG_SLOT;
  register UINT64 r28 __asm__("r28");

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
	  avalue[i] = (void *)(stack + slot) + 7;
	  break;

	case SFFI_TYPE_SINT16:
	case SFFI_TYPE_UINT16:
	  avalue[i] = (void *)(stack + slot) + 6;
	  break;

	case SFFI_TYPE_SINT32:
	case SFFI_TYPE_UINT32:
	  avalue[i] = (void *)(stack + slot) + 4;
	  break;

	case SFFI_TYPE_POINTER:
	case SFFI_TYPE_SINT64:
	case SFFI_TYPE_UINT64:
	  avalue[i] = (void *)(stack + slot);
	  break;

	case SFFI_TYPE_FLOAT:
	  
	  switch (slot + FIRST_ARG_SLOT)
	    {
	    case 0: fstw(fr4, (void *)(stack + slot)); break;
	    case 1: fstw(fr5, (void *)(stack + slot)); break;
	    case 2: fstw(fr6, (void *)(stack + slot)); break;
	    case 3: fstw(fr7, (void *)(stack + slot)); break;
	    case 4: fstw(fr8, (void *)(stack + slot)); break;
	    case 5: fstw(fr9, (void *)(stack + slot)); break;
	    case 6: fstw(fr10, (void *)(stack + slot)); break;
	    case 7: fstw(fr11, (void *)(stack + slot)); break;
	    }
	  avalue[i] = (void *)(stack + slot) + 4;
	  break;

	case SFFI_TYPE_DOUBLE:
	  
	  switch (slot + FIRST_ARG_SLOT)
	    {
	    case 0: fstd(fr4, (void *)(stack + slot)); break;
	    case 1: fstd(fr5, (void *)(stack + slot)); break;
	    case 2: fstd(fr6, (void *)(stack + slot)); break;
	    case 3: fstd(fr7, (void *)(stack + slot)); break;
	    case 4: fstd(fr8, (void *)(stack + slot)); break;
	    case 5: fstd(fr9, (void *)(stack + slot)); break;
	    case 6: fstd(fr10, (void *)(stack + slot)); break;
	    case 7: fstd(fr11, (void *)(stack + slot)); break;
	    }
	  avalue[i] = (void *)(stack + slot);
	  break;

#ifdef PA64_HPUX
	case SFFI_TYPE_LONGDOUBLE:
	  
	  slot += (slot & 1);
	  avalue[i] = (void *)(stack + slot);
	  break;
#endif

	case SFFI_TYPE_STRUCT:
	  
	  if((*p_arg)->size > 8)
	    slot += (slot & 1);
	  avalue[i] = (void *) (stack + slot);
	  break;

	default:
	  SFFI_ASSERT(0);
	}

      slot += (ROUND_UP ((*p_arg)->size, 8) >> 3);
      p_arg++;
    }

  
  (closure->fun) (cif, rvalue, avalue, closure->user_data);

  debug(3, "after calling function, ret[0] = %16lx, ret[1] = %16lx\n", u.ret[0],
	u.ret[1]);

  
  switch (cif->flags)
    {
    case SFFI_TYPE_UINT8:
      *(stack + FIRST_ARG_SLOT) = (UINT8)u.ret[0];
      break;
    case SFFI_TYPE_SINT8:
      *(stack + FIRST_ARG_SLOT) = (SINT8)u.ret[0];
      break;
    case SFFI_TYPE_UINT16:
      *(stack + FIRST_ARG_SLOT) = (UINT16)u.ret[0];
      break;
    case SFFI_TYPE_SINT16:
      *(stack + FIRST_ARG_SLOT) = (SINT16)u.ret[0];
      break;
    case SFFI_TYPE_INT:
    case SFFI_TYPE_SINT32:
      *(stack + FIRST_ARG_SLOT) = (SINT32)u.ret[0];
      break;
    case SFFI_TYPE_UINT32:
      *(stack - FIRST_ARG_SLOT) = (UINT32)u.ret[0];
      break;
    case SFFI_TYPE_SINT64:
    case SFFI_TYPE_UINT64:
    case SFFI_TYPE_POINTER:
      *(stack - FIRST_ARG_SLOT) = u.ret[0];
      break;

    case SFFI_TYPE_LONGDOUBLE:
      *(stack + FIRST_ARG_SLOT) = u.ret[0];
      *(stack + FIRST_ARG_SLOT + 1) = u.ret[1];
      break;

    case SFFI_TYPE_DOUBLE:
      fldd(rvalue, fr4);
      break;

    case SFFI_TYPE_FLOAT:
      
      fldw(rvalue - 4, fr4);
      break;

    case SFFI_TYPE_STRUCT:
      
      break;

    case -1:
    case -2:
    case -3:
    case -4:
    case -5:
    case -6:
    case -7:
    case -8:
    case -9:
    case -10:
    case -11:
    case -12:
    case -13:
    case -14:
    case -15:
    case -16:
      tmp = (void*)(stack + FIRST_ARG_SLOT);
      memcpy((void*)tmp, &u, cif->rtype->size);
      break;

    case SFFI_TYPE_VOID:
      break;

    default:
      debug(0, "assert with cif->flags: %d\n",cif->flags);
      SFFI_ASSERT(0);
      break;
    }
  return SFFI_OK;
}



extern void sffi_closure_pa64(void);

sffi_status
sffi_prep_closure_loc (sffi_closure* closure,
		      sffi_cif* cif,
		      void (*fun)(sffi_cif*,void*,void**,void*),
		      void *user_data,
		      void *codeloc)
{
  
  struct pa64_fd
  {
    UINT64 tmp1;
    UINT64 tmp2;
    UINT64 code_pointer;
    UINT64 gp;
  };

  struct sffi_pa64_trampoline_struct
  {
    UINT64 real_gp;             
    UINT64 tmp2;
    UINT64 code_pointer;        
    UINT64 fake_gp;             
  };

  struct sffi_pa64_trampoline_struct *tramp;
  struct pa64_fd *fd;

  if (cif->abi != SFFI_PA64)
    return SFFI_BAD_ABI;

  
  fd = (struct pa64_fd *)((UINT64)sffi_closure_pa64);

  
  tramp = (struct sffi_pa64_trampoline_struct *)closure->tramp;
  tramp->code_pointer = fd->code_pointer;
  tramp->fake_gp = (UINT64)codeloc;
  tramp->real_gp = fd->gp;

  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;

  return SFFI_OK;
}
#endif
