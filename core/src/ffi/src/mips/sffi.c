

#include <sffi.h>
#include <sffi_common.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __GNUC__
#  if (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 3))
#    define USE__BUILTIN___CLEAR_CACHE 1
#  endif
#endif

#ifndef USE__BUILTIN___CLEAR_CACHE
#  if defined(__FreeBSD__)
#    include <machine/sysarch.h>
#  elif defined(__OpenBSD__)
#    include <mips64/sysarch.h>
#  else
#    include <sys/cachectl.h>
#  endif
#endif

#ifdef SFFI_DEBUG
# define SFFI_MIPS_STOP_HERE() sffi_stop_here()
#else
# define SFFI_MIPS_STOP_HERE() do {} while(0)
#endif

#ifdef SFFI_MIPS_N32
#define FIX_ARGP \
SFFI_ASSERT(argp <= &stack[bytes]); \
if (argp == &stack[bytes]) \
{ \
  argp = stack; \
  SFFI_MIPS_STOP_HERE(); \
}
#else
#define FIX_ARGP 
#endif




static void sffi_prep_args(char *stack, 
			  extended_cif *ecif,
			  int bytes,
			  int flags)
{
  int i;
  void **p_argv;
  char *argp, *argp_f;
  sffi_type **p_arg;

  memset(stack, 0, bytes);

#ifdef SFFI_MIPS_N32
  int soft_float = (ecif->cif->abi == SFFI_N32_SOFT_FLOAT
		    || ecif->cif->abi == SFFI_N64_SOFT_FLOAT);
  
  
  if (ecif->cif->rtype->type == SFFI_TYPE_COMPLEX && ecif->cif->rtype->elements[0]->type == SFFI_TYPE_LONGDOUBLE)
    {
      if (bytes + 16 > 8 * sizeof(sffi_arg))
        argp = &stack[bytes - (8 * sizeof(sffi_arg))];
      else
        argp = stack;
      * (unsigned long *) argp = (unsigned long) ecif->rvalue;
      argp += 16;
    }
  else
    {
      if (bytes > 8 * sizeof(sffi_arg))
        argp = &stack[bytes - (8 * sizeof(sffi_arg))];
      else
        argp = stack;
    }
#else
  argp = stack;
#endif

  argp_f = argp;

#ifdef SFFI_MIPS_N32
  if ( ecif->cif->rstruct_flag != 0 )
#else
  if ( ecif->cif->rtype->type == SFFI_TYPE_STRUCT )
#endif  
    {
      *(sffi_arg *) argp = (sffi_arg) ecif->rvalue;
      argp += sizeof(sffi_arg);
      FIX_ARGP;
    }

  p_argv = ecif->avalue;

  for (i = 0, p_arg = ecif->cif->arg_types; i < ecif->cif->nargs; i++, p_arg++)
    {
      size_t z;
      unsigned int a;

      
      a = (*p_arg)->alignment;
      if (a < sizeof(sffi_arg))
        a = sizeof(sffi_arg);
      
      if ((a - 1) & (unsigned long) argp)
	{
	  argp = (char *) SFFI_ALIGN(argp, a);
	  FIX_ARGP;
	}

      z = (*p_arg)->size;
      if (z <= sizeof(sffi_arg))
	{
          int type = (*p_arg)->type;
	  z = sizeof(sffi_arg);

          
          if (type == SFFI_TYPE_POINTER)
            type = (ecif->cif->abi == SFFI_N64
		    || ecif->cif->abi == SFFI_N64_SOFT_FLOAT)
	      ? SFFI_TYPE_SINT64 : SFFI_TYPE_UINT32;

	if (i < 8 && (ecif->cif->abi == SFFI_N32_SOFT_FLOAT
		      || ecif->cif->abi == SFFI_N64_SOFT_FLOAT))
	  {
	    switch (type)
	      {
	      case SFFI_TYPE_FLOAT:
		type = SFFI_TYPE_UINT32;
		break;
	      case SFFI_TYPE_DOUBLE:
		type = SFFI_TYPE_UINT64;
		break;
	      default:
		break;
	      }
	  }
	  switch (type)
	    {
	      case SFFI_TYPE_SINT8:
		*(sffi_arg *)argp = *(SINT8 *)(* p_argv);
		break;

	      case SFFI_TYPE_UINT8:
		*(sffi_arg *)argp = *(UINT8 *)(* p_argv);
		break;
		  
	      case SFFI_TYPE_SINT16:
		*(sffi_arg *)argp = *(SINT16 *)(* p_argv);
		break;
		  
	      case SFFI_TYPE_UINT16:
		*(sffi_arg *)argp = *(UINT16 *)(* p_argv);
		break;
		  
	      case SFFI_TYPE_SINT32:
		*(sffi_arg *)argp = *(SINT32 *)(* p_argv);
		break;
		  
	      case SFFI_TYPE_UINT32:
#ifdef SFFI_MIPS_N32
		
		*(sffi_arg *)argp = *(SINT32 *)(* p_argv);
#else
		*(sffi_arg *)argp = *(UINT32 *)(* p_argv);
#endif
		break;

#ifdef SFFI_MIPS_N32
	      case SFFI_TYPE_COMPLEX:
		
		
		
	        if(!soft_float
		    && (*p_arg)->elements[0]->type == SFFI_TYPE_FLOAT
		    && argp>=argp_f
		    && i < ecif->cif->mips_nfixedargs)
		  {
		    *(float *) argp = *(float *)(* p_argv);
		    argp += z;
		    char *tmp = (void *) (*p_argv);
		    *(float *) argp = *(float *)(tmp+4);
		  }
		else
		  memcpy(argp, *p_argv, (*p_arg)->size);
		break;
#endif
	      
	      case SFFI_TYPE_FLOAT:
		*(float *) argp = *(float *)(* p_argv);
		break;

	      
	      default:
		memcpy(argp, *p_argv, (*p_arg)->size);
		break;
	    }
	}
      else
	{
#ifdef SFFI_MIPS_O32
	  memcpy(argp, *p_argv, z);
#else
	  {
	    unsigned long end = (unsigned long) argp + z;
	    unsigned long cap = (unsigned long) stack + bytes;

	    

	    if (end <= cap)
	      memcpy(argp, *p_argv, z);
	    else
	      {
		unsigned long portion = cap - (unsigned long)argp;

		memcpy(argp, *p_argv, portion);
		argp = stack;
                z -= portion;
		memcpy(argp, (void*)((unsigned long)(*p_argv) + portion),
                       z);
	      }
	  }
#endif
      }
      p_argv++;
      argp += z;
      FIX_ARGP;
    }
}

#ifdef SFFI_MIPS_N32



static int
calc_n32_struct_flags_element(unsigned *flags, sffi_type *e,
			      unsigned *loc, unsigned *arg_reg)
{
  
  *loc = SFFI_ALIGN(*loc, e->alignment);
  if (e->type == SFFI_TYPE_DOUBLE)
    {
      
      *arg_reg = *loc / SFFI_SIZEOF_ARG;
      if (*arg_reg > 7)
	return 1;
      *flags += (SFFI_TYPE_DOUBLE << (*arg_reg * SFFI_FLAG_BITS));
    }
  *loc += e->size;
  return 0;
}

static unsigned
calc_n32_struct_flags(int soft_float, sffi_type *arg,
		      unsigned *loc, unsigned *arg_reg)
{
  unsigned flags = 0;
  unsigned index = 0;

  sffi_type *e;

  if (soft_float)
    return 0;

  while ((e = arg->elements[index]))
    {
      if (e->type == SFFI_TYPE_COMPLEX)
	{
	  if (calc_n32_struct_flags_element(&flags, e->elements[0], loc, arg_reg))
	    break;
	  if (calc_n32_struct_flags_element(&flags, e->elements[0], loc, arg_reg))
	    break;
	}
      else
	if (calc_n32_struct_flags_element(&flags, e, loc, arg_reg))
	  break;
      index++;
    }
  
  *arg_reg = SFFI_ALIGN(*loc, SFFI_SIZEOF_ARG) / SFFI_SIZEOF_ARG;

  return flags;
}

static unsigned
calc_n32_return_struct_flags(int soft_float, sffi_type *arg)
{
  unsigned flags;
  unsigned small = SFFI_TYPE_SMALLSTRUCT;
  sffi_type *e;

  
  
  if (arg->size > 16)
    return 0;

  if (arg->size > 8)
    small = SFFI_TYPE_SMALLSTRUCT2;

  e = arg->elements[0];

  if (e->type == SFFI_TYPE_COMPLEX)
    {
      int type = e->elements[0]->type;

      if (type != SFFI_TYPE_DOUBLE && type != SFFI_TYPE_FLOAT)
	return small;

      if (arg->elements[1])
	{
	  
	  return small;
	}

      flags = (type << SFFI_FLAG_BITS) + type;
    }
  else
    {
      if (e->type != SFFI_TYPE_DOUBLE && e->type != SFFI_TYPE_FLOAT)
	return small;

      flags = e->type;

      if (arg->elements[1])
	{
	  e = arg->elements[1];
	  if (e->type != SFFI_TYPE_DOUBLE && e->type != SFFI_TYPE_FLOAT)
	    return small;

	  if (arg->elements[2])
	    {
	      
	      return small;
	    }

	  flags += e->type << SFFI_FLAG_BITS;
	}
    }

  if (soft_float)
    flags += SFFI_TYPE_STRUCT_SOFT;
  return flags;
}

#endif


static sffi_status sffi_prep_cif_machdep_int(sffi_cif *cif, unsigned nfixedargs)
{
  cif->flags = 0;
  cif->mips_nfixedargs = nfixedargs;

#ifdef SFFI_MIPS_O32
  

  if (cif->rtype->type != SFFI_TYPE_STRUCT && cif->rtype->type != SFFI_TYPE_COMPLEX && cif->abi == SFFI_O32)
    {
      if (cif->nargs > 0 && cif->nargs == nfixedargs)
	{
	  switch ((cif->arg_types)[0]->type)
	    {
	    case SFFI_TYPE_FLOAT:
	    case SFFI_TYPE_DOUBLE:
	      cif->flags += (cif->arg_types)[0]->type;
	      break;
	      
	    default:
	      break;
	    }

	  if (cif->nargs > 1)
	    {
	      
	      if (cif->flags)
		{
		  switch ((cif->arg_types)[1]->type)
		    {
		    case SFFI_TYPE_FLOAT:
		    case SFFI_TYPE_DOUBLE:
		      cif->flags += (cif->arg_types)[1]->type << SFFI_FLAG_BITS;
		      break;
		      
		    default:
		      break;
		    }
		}
	    }
	}
    }
      
  

  if (cif->abi == SFFI_O32_SOFT_FLOAT)
    {
      switch (cif->rtype->type)
        {
        case SFFI_TYPE_VOID:
        case SFFI_TYPE_STRUCT:
          cif->flags += cif->rtype->type << (SFFI_FLAG_BITS * 2);
          break;

        case SFFI_TYPE_SINT64:
        case SFFI_TYPE_UINT64:
        case SFFI_TYPE_DOUBLE:
          cif->flags += SFFI_TYPE_UINT64 << (SFFI_FLAG_BITS * 2);
          break;
      
        case SFFI_TYPE_FLOAT:
        default:
          cif->flags += SFFI_TYPE_INT << (SFFI_FLAG_BITS * 2);
          break;
        }
    }
  else
    {
            
      switch (cif->rtype->type)
        {
        case SFFI_TYPE_VOID:
        case SFFI_TYPE_STRUCT:
        case SFFI_TYPE_FLOAT:
        case SFFI_TYPE_DOUBLE:
        case SFFI_TYPE_COMPLEX:
          cif->flags += cif->rtype->type << (SFFI_FLAG_BITS * 2);
	  if (cif->rtype->type == SFFI_TYPE_COMPLEX)
            cif->flags +=  ((*cif->rtype->elements[0]).type) << (SFFI_FLAG_BITS * 4);
          break;

        case SFFI_TYPE_SINT64:
        case SFFI_TYPE_UINT64:
          cif->flags += SFFI_TYPE_UINT64 << (SFFI_FLAG_BITS * 2);
          break;
      
        default:
          cif->flags += SFFI_TYPE_INT << (SFFI_FLAG_BITS * 2);
          break;
        }
    }
#endif

#ifdef SFFI_MIPS_N32
  
  {
    unsigned arg_reg = 0;
    unsigned loc = 0;
    unsigned count = (cif->nargs < 8) ? cif->nargs : 8;
    unsigned index = 0;

    unsigned struct_flags = 0;
    int soft_float = (cif->abi == SFFI_N32_SOFT_FLOAT
		      || cif->abi == SFFI_N64_SOFT_FLOAT);

    if (cif->rtype->type == SFFI_TYPE_STRUCT)
      {
	struct_flags = calc_n32_return_struct_flags(soft_float, cif->rtype);

	if (struct_flags == 0)
	  {
	    

	    arg_reg = 1;
	    count = (cif->nargs < 7) ? cif->nargs : 7;

	    cif->rstruct_flag = !0;
	  }
	else
	    cif->rstruct_flag = 0;
      }
    else
      cif->rstruct_flag = 0;

    while (count-- > 0 && arg_reg < 8)
      {
	sffi_type *t = cif->arg_types[index];

	switch (t->type)
	  {
	  case SFFI_TYPE_FLOAT:
	  case SFFI_TYPE_DOUBLE:
	    if (!soft_float && index < nfixedargs)
              cif->flags += t->type << (arg_reg * SFFI_FLAG_BITS);
	    arg_reg++;
	    break;
          case SFFI_TYPE_LONGDOUBLE:
            
            arg_reg = SFFI_ALIGN(arg_reg, 2);
            
	    if (soft_float || index >= nfixedargs)
	      {
		arg_reg += 2;
	      }
	    else
	      {
		cif->flags +=
		  (SFFI_TYPE_DOUBLE << (arg_reg * SFFI_FLAG_BITS));
		arg_reg++;
		if (arg_reg >= 8)
		  continue;
		cif->flags +=
		  (SFFI_TYPE_DOUBLE << (arg_reg * SFFI_FLAG_BITS));
		arg_reg++;
	      }
            break;

	  case SFFI_TYPE_COMPLEX:
	    switch (t->elements[0]->type)
	      {
	      case SFFI_TYPE_LONGDOUBLE:
		arg_reg = SFFI_ALIGN(arg_reg, 2);
		if (soft_float || index >= nfixedargs)
		  {
		    arg_reg += 2;
		  }
		else
		  {
		    cif->flags +=
		      (SFFI_TYPE_DOUBLE << (arg_reg * SFFI_FLAG_BITS));
		    arg_reg++;
		    if (arg_reg >= 8)
		        continue;
		    cif->flags +=
		      (SFFI_TYPE_DOUBLE << (arg_reg * SFFI_FLAG_BITS));
		    arg_reg++;
		    if (arg_reg >= 8)
		        continue;
		  }
		
	      case SFFI_TYPE_FLOAT:

		cif->bytes += 16;
		
	      case SFFI_TYPE_SINT32:
	      case SFFI_TYPE_UINT32:
	      case SFFI_TYPE_DOUBLE:
		if (soft_float || index >= nfixedargs)
		  {
		    arg_reg += 2;
		  }
		else
		  {
		    uint32_t type = t->elements[0]->type != SFFI_TYPE_LONGDOUBLE? t->elements[0]->type: SFFI_TYPE_DOUBLE;
		    cif->flags +=
		      (type << (arg_reg * SFFI_FLAG_BITS));
		    arg_reg++;
		    if (arg_reg >= 8)
		        continue;
		    cif->flags +=
		      (type << (arg_reg * SFFI_FLAG_BITS));
		    arg_reg++;
		  }
		break;
	      default:
		arg_reg += 2;
		break;
	      }
	    break;

	  case SFFI_TYPE_STRUCT:
            loc = arg_reg * SFFI_SIZEOF_ARG;
	    cif->flags += calc_n32_struct_flags(soft_float || index >= nfixedargs,
						t, &loc, &arg_reg);
	    break;

	  default:
	    arg_reg++;
            break;
	  }

	index++;
      }

  
    switch (cif->rtype->type)
      {
      case SFFI_TYPE_STRUCT:
	{
	  if (struct_flags == 0)
	    {
	      
	    }
	  else
	    {
	      
	      cif->flags += SFFI_TYPE_STRUCT << (SFFI_FLAG_BITS * 8);
	      cif->flags += struct_flags << (4 + (SFFI_FLAG_BITS * 8));
	    }
	  break;
	}
      
      case SFFI_TYPE_VOID:
	
	break;

      case SFFI_TYPE_POINTER:
	if (cif->abi == SFFI_N32_SOFT_FLOAT || cif->abi == SFFI_N32)
	  cif->flags += SFFI_TYPE_SINT32 << (SFFI_FLAG_BITS * 8);
	else
	  cif->flags += SFFI_TYPE_UINT64 << (SFFI_FLAG_BITS * 8);
	break;

      case SFFI_TYPE_FLOAT:
	if (soft_float)
	  {
	    cif->flags += SFFI_TYPE_SINT32 << (SFFI_FLAG_BITS * 8);
	    break;
	  }
	
      case SFFI_TYPE_DOUBLE:
	if (soft_float)
	  cif->flags += SFFI_TYPE_UINT64 << (SFFI_FLAG_BITS * 8);
	else
	  cif->flags += cif->rtype->type << (SFFI_FLAG_BITS * 8);
	break;

      case SFFI_TYPE_LONGDOUBLE:
	
	if (soft_float)
	  {
	    
	    cif->flags += SFFI_TYPE_LONGDOUBLE << (SFFI_FLAG_BITS * 8);
 	  }
	else
	  {
	    cif->flags += SFFI_TYPE_STRUCT << (SFFI_FLAG_BITS * 8);
	    cif->flags += (SFFI_TYPE_DOUBLE
			   + (SFFI_TYPE_DOUBLE << SFFI_FLAG_BITS))
					      << (4 + (SFFI_FLAG_BITS * 8));
	  }
	break;
      case SFFI_TYPE_COMPLEX:
	{
	  int type = cif->rtype->elements[0]->type;

	  cif->flags += (SFFI_TYPE_COMPLEX << (SFFI_FLAG_BITS * 8));
	  if (soft_float || (type != SFFI_TYPE_FLOAT && type != SFFI_TYPE_DOUBLE && type != SFFI_TYPE_LONGDOUBLE))
	    {
	      switch (type)
		{
		case SFFI_TYPE_DOUBLE:
		case SFFI_TYPE_SINT64:
		case SFFI_TYPE_UINT64:
		case SFFI_TYPE_INT:
		  type = SFFI_TYPE_SMALLSTRUCT2;
		  break;
		case SFFI_TYPE_LONGDOUBLE:
		  type = SFFI_TYPE_LONGDOUBLE;
		  break;
		case SFFI_TYPE_FLOAT:
		default:
		  type = SFFI_TYPE_SMALLSTRUCT;
		}
	      cif->flags += type << (4 + (SFFI_FLAG_BITS * 8));
	    }
	  else
	    {


	      cif->flags += type << (4 + (SFFI_FLAG_BITS * 8));
	    }
	  break;
	}
      case SFFI_TYPE_UINT32:
	
	cif->flags += SFFI_TYPE_SINT32 << (SFFI_FLAG_BITS * 8);
	break;
      case SFFI_TYPE_SINT64:
	cif->flags += SFFI_TYPE_UINT64 << (SFFI_FLAG_BITS * 8);
	break;
      default:
	cif->flags += cif->rtype->type << (SFFI_FLAG_BITS * 8);
	break;
      }
  }
#endif
  return SFFI_OK;
}

sffi_status sffi_prep_cif_machdep(sffi_cif *cif)
{
    return sffi_prep_cif_machdep_int(cif, cif->nargs);
}

sffi_status sffi_prep_cif_machdep_var(sffi_cif *cif,
                                    unsigned nfixedargs,
                                    unsigned ntotalargs MAYBE_UNUSED)
{
    return sffi_prep_cif_machdep_int(cif, nfixedargs);
}


extern int sffi_call_O32(void (*)(char *, extended_cif *, int, int), 
			extended_cif *, unsigned, 
			unsigned, unsigned *, void (*)(void), void *closure);


extern int sffi_call_N32(void (*)(char *, extended_cif *, int, int), 
			extended_cif *, unsigned, 
			unsigned, void *, void (*)(void), void *closure);

void sffi_call_int(sffi_cif *cif, void (*fn)(void), void *rvalue, 
	      void **avalue, void *closure)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;
  
  
  
  
  if ((rvalue == NULL) && 
      (cif->rtype->type == SFFI_TYPE_STRUCT || cif->rtype->type == SFFI_TYPE_COMPLEX))
    ecif.rvalue = alloca(cif->rtype->size);
  else
    ecif.rvalue = rvalue;
    
  switch (cif->abi) 
    {
#ifdef SFFI_MIPS_O32
    case SFFI_O32:
    case SFFI_O32_SOFT_FLOAT:
      sffi_call_O32(sffi_prep_args, &ecif, cif->bytes, 
		   cif->flags, ecif.rvalue, fn, closure);
      break;
#endif

#ifdef SFFI_MIPS_N32
    case SFFI_N32:
    case SFFI_N32_SOFT_FLOAT:
    case SFFI_N64:
    case SFFI_N64_SOFT_FLOAT:
      {
        int copy_rvalue = 0;
	int copy_offset = 0;
        char *rvalue_copy = ecif.rvalue;
        if (cif->rtype->type == SFFI_TYPE_STRUCT && cif->rtype->size < 16)
          {
            
            rvalue_copy = alloca(16);
            copy_rvalue = 1;
          }
	else if (cif->rtype->type == SFFI_TYPE_FLOAT
		 && (cif->abi == SFFI_N64_SOFT_FLOAT
		     || cif->abi == SFFI_N32_SOFT_FLOAT))
	  {
	    rvalue_copy = alloca (8);
	    copy_rvalue = 1;
#if defined(__MIPSEB__) || defined(_MIPSEB)
	    copy_offset = 4;
#endif
	  }
        sffi_call_N32(sffi_prep_args, &ecif, cif->bytes,
                     cif->flags, rvalue_copy, fn, closure);
        if (copy_rvalue)
          memcpy(ecif.rvalue, rvalue_copy + copy_offset, cif->rtype->size);
      }
      break;
#endif

    default:
      SFFI_ASSERT(0);
      break;
    }
}

void
sffi_call(sffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  sffi_call_int (cif, fn, rvalue, avalue, NULL);
}

void
sffi_call_go (sffi_cif *cif, void (*fn)(void), void *rvalue,
	     void **avalue, void *closure)
{
  sffi_call_int (cif, fn, rvalue, avalue, closure);
}


#if SFFI_CLOSURES
#if defined(SFFI_MIPS_O32)
extern void sffi_closure_O32(void);
extern void sffi_go_closure_O32(void);
#else
extern void sffi_closure_N32(void);
extern void sffi_go_closure_N32(void);
#endif 

sffi_status
sffi_prep_closure_loc (sffi_closure *closure,
		      sffi_cif *cif,
		      void (*fun)(sffi_cif*,void*,void**,void*),
		      void *user_data,
		      void *codeloc)
{
  unsigned int *tramp = (unsigned int *) &closure->tramp[0];
  void * fn;
  char *clear_location = (char *) codeloc;

#if defined(SFFI_MIPS_O32)
  if (cif->abi != SFFI_O32 && cif->abi != SFFI_O32_SOFT_FLOAT)
    return SFFI_BAD_ABI;
  fn = sffi_closure_O32;
#else
#if _MIPS_SIM ==_ABIN32
  if (cif->abi != SFFI_N32
      && cif->abi != SFFI_N32_SOFT_FLOAT)
    return SFFI_BAD_ABI;
#else
  if (cif->abi != SFFI_N64
      && cif->abi != SFFI_N64_SOFT_FLOAT)
    return SFFI_BAD_ABI;
#endif
  fn = sffi_closure_N32;
#endif 

#if defined(SFFI_MIPS_O32) || (_MIPS_SIM ==_ABIN32)
  
  tramp[0] = 0x3c190000 | ((unsigned)fn >> 16);
  
  tramp[1] = 0x37390000 | ((unsigned)fn & 0xffff);
  
  tramp[2] = 0x3c0c0000 | ((unsigned)codeloc >> 16);
  
#if !defined(__mips_isa_rev) || (__mips_isa_rev<6)
  tramp[3] = 0x03200008;
#else
  tramp[3] = 0x03200009;
#endif
  
  tramp[4] = 0x358c0000 | ((unsigned)codeloc & 0xffff);
#else
  
  
  tramp[0] = 0x3c190000 | ((unsigned long)fn >> 48);
  
  tramp[1] = 0x3c0c0000 | ((unsigned long)codeloc >> 48);
  
  tramp[2] = 0x37390000 | (((unsigned long)fn >> 32 ) & 0xffff);
  
  tramp[3] = 0x358c0000 | (((unsigned long)codeloc >> 32) & 0xffff);
  
  tramp[4] = 0x0019cc38;
  
  tramp[5] = 0x000c6438;
  
  tramp[6] = 0x37390000 | (((unsigned long)fn >> 16 ) & 0xffff);
  
  tramp[7] = 0x358c0000 | (((unsigned long)codeloc >> 16) & 0xffff);
  
  tramp[8] = 0x0019cc38;
  
  tramp[9] = 0x000c6438;
  
  tramp[10] = 0x37390000 | ((unsigned long)fn  & 0xffff);
  
#if !defined(__mips_isa_rev) || (__mips_isa_rev<6)
  tramp[11] = 0x03200008;
#else
  tramp[11] = 0x03200009;
#endif
  
  tramp[12] = 0x358c0000 | ((unsigned long)codeloc & 0xffff);

#endif

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

#if !defined(__FreeBSD__)
#ifdef USE__BUILTIN___CLEAR_CACHE
  __builtin___clear_cache(clear_location, clear_location + SFFI_TRAMPOLINE_SIZE);
#else
  cacheflush (clear_location, SFFI_TRAMPOLINE_SIZE, ICACHE);
#endif
#endif 
  return SFFI_OK;
}


int
sffi_closure_mips_inner_O32 (sffi_cif *cif,
                            void (*fun)(sffi_cif*, void*, void**, void*),
			    void *user_data,
			    void *rvalue, sffi_arg *ar,
			    double *fpr)
{
  void **avaluep;
  sffi_arg *avalue;
  sffi_type **arg_types;
  int i, avn, argn, seen_int;

  avalue = alloca (cif->nargs * sizeof (sffi_arg));
  avaluep = alloca (cif->nargs * sizeof (sffi_arg));

  seen_int = (cif->abi == SFFI_O32_SOFT_FLOAT) || (cif->mips_nfixedargs != cif->nargs);
  argn = 0;

  if ((cif->flags >> (SFFI_FLAG_BITS * 2)) == SFFI_TYPE_STRUCT)
    {
      rvalue = (void *)(uintptr_t)ar[0];
      argn = 1;
      seen_int = 1;
    }
  if ((cif->flags >> (SFFI_FLAG_BITS * 2)) == SFFI_TYPE_COMPLEX)
    {
      rvalue = fpr;
      argn = 1;
    }

  i = 0;
  avn = cif->nargs;
  arg_types = cif->arg_types;

  while (i < avn)
    {
      if (arg_types[i]->alignment == 8 && (argn & 0x1))
        argn++;
      if (i < 2 && !seen_int &&
	  (arg_types[i]->type == SFFI_TYPE_FLOAT ||
	   arg_types[i]->type == SFFI_TYPE_DOUBLE ||
	   arg_types[i]->type == SFFI_TYPE_LONGDOUBLE))
	{
#if defined(__MIPSEB__) || defined(_MIPSEB)
	  if (arg_types[i]->type == SFFI_TYPE_FLOAT)
	    avaluep[i] = ((char *) &fpr[i]) + sizeof (float);
	  else
#endif
	    avaluep[i] = (char *) &fpr[i];
	}
      else
	{
	  switch (arg_types[i]->type)
	    {
	      case SFFI_TYPE_SINT8:
		avaluep[i] = &avalue[i];
		*(SINT8 *) &avalue[i] = (SINT8) ar[argn];
		break;

	      case SFFI_TYPE_UINT8:
		avaluep[i] = &avalue[i];
		*(UINT8 *) &avalue[i] = (UINT8) ar[argn];
		break;
		  
	      case SFFI_TYPE_SINT16:
		avaluep[i] = &avalue[i];
		*(SINT16 *) &avalue[i] = (SINT16) ar[argn];
		break;
		  
	      case SFFI_TYPE_UINT16:
		avaluep[i] = &avalue[i];
		*(UINT16 *) &avalue[i] = (UINT16) ar[argn];
		break;

	      default:
		avaluep[i] = (char *) &ar[argn];
		break;
	    }
	  seen_int = 1;
	}
      argn += SFFI_ALIGN(arg_types[i]->size, SFFI_SIZEOF_ARG) / SFFI_SIZEOF_ARG;
      i++;
    }

  
  fun(cif, rvalue, avaluep, user_data);

  if (cif->abi == SFFI_O32_SOFT_FLOAT)
    {
      switch (cif->rtype->type)
        {
        case SFFI_TYPE_FLOAT:
          return SFFI_TYPE_INT;
        case SFFI_TYPE_DOUBLE:
          return SFFI_TYPE_UINT64;
        default:
          return cif->rtype->type;
        }
    }
  else
    {
      if (cif->rtype->type == SFFI_TYPE_COMPLEX) {
          __asm__ volatile ("move $v1, %0" : : "r"(cif->rtype->size));
      }
      return cif->rtype->type;
    }
}

#if defined(SFFI_MIPS_N32)

static void
copy_struct_N32(char *target, unsigned offset, sffi_abi abi, sffi_type *type,
                int argn, unsigned arg_offset, sffi_arg *ar,
                sffi_arg *fpr, int soft_float)
{
  sffi_type **elt_typep = type->elements;
  while(*elt_typep)
    {
      sffi_type *elt_type = *elt_typep;
      unsigned o;
      char *tp;
      char *argp;
      char *fpp;

      o = SFFI_ALIGN(offset, elt_type->alignment);
      arg_offset += o - offset;
      offset = o;
      argn += arg_offset / sizeof(sffi_arg);
      arg_offset = arg_offset % sizeof(sffi_arg);

      argp = (char *)(ar + argn);
      fpp = (char *)(argn >= 8 ? ar + argn : fpr + argn);

      tp = target + offset;

      if (elt_type->type == SFFI_TYPE_DOUBLE && !soft_float)
        *(double *)tp = *(double *)fpp;
      else
        memcpy(tp, argp + arg_offset, elt_type->size);

      offset += elt_type->size;
      arg_offset += elt_type->size;
      elt_typep++;
      argn += arg_offset / sizeof(sffi_arg);
      arg_offset = arg_offset % sizeof(sffi_arg);
    }
}


int
sffi_closure_mips_inner_N32 (sffi_cif *cif, 
			    void (*fun)(sffi_cif*, void*, void**, void*),
                            void *user_data,
			    void *rvalue, sffi_arg *ar,
			    sffi_arg *fpr)
{
  void **avaluep;
  sffi_arg *avalue;
  sffi_type **arg_types;
  int i, avn, argn;
  int soft_float;
  sffi_arg *argp;

  soft_float = cif->abi == SFFI_N64_SOFT_FLOAT
    || cif->abi == SFFI_N32_SOFT_FLOAT;
  avalue = alloca (cif->nargs * sizeof (sffi_arg));
  avaluep = alloca (cif->nargs * sizeof (sffi_arg));

  argn = 0;

  if (cif->rstruct_flag)
    {
#if _MIPS_SIM==_ABIN32
      rvalue = (void *)(UINT32)ar[0];
#else 
      rvalue = (void *)ar[0];
#endif
      argn = 1;
    }
  if (cif->rtype->type == SFFI_TYPE_COMPLEX && cif->rtype->elements[0]->type == SFFI_TYPE_LONGDOUBLE)
    argn = 2;

  i = 0;
  avn = cif->nargs;
  arg_types = cif->arg_types;

  while (i < avn)
    {
      if (arg_types[i]->type == SFFI_TYPE_FLOAT
	  || arg_types[i]->type == SFFI_TYPE_DOUBLE
	  || arg_types[i]->type == SFFI_TYPE_LONGDOUBLE)
        {
          argp = (argn >= 8 || i >= cif->mips_nfixedargs || soft_float) ? ar + argn : fpr + argn;
          if ((arg_types[i]->type == SFFI_TYPE_LONGDOUBLE) && ((uintptr_t)argp & (arg_types[i]->alignment-1)))
            {
              argp=(sffi_arg*)SFFI_ALIGN(argp,arg_types[i]->alignment);
              argn++;
            }
#if defined(__MIPSEB__) || defined(_MIPSEB)
          if (arg_types[i]->type == SFFI_TYPE_FLOAT && argn < 8)
            avaluep[i] = ((char *) argp) + sizeof (float);
          else
#endif
            avaluep[i] = (char *) argp;
        }
      else if (arg_types[i]->type == SFFI_TYPE_COMPLEX && arg_types[i]->elements[0]->type == SFFI_TYPE_DOUBLE)
        {
          argp = (argn >= 8 || i >= cif->mips_nfixedargs || soft_float) ? ar + argn : fpr + argn;
          avaluep[i] = (char *) argp;
        }
      else if (arg_types[i]->type == SFFI_TYPE_COMPLEX && arg_types[i]->elements[0]->type == SFFI_TYPE_LONGDOUBLE)
        {
	  
	  argn += ((argn & 0x1)? 1 : 0);
          argp = (argn >= 8 || i >= cif->mips_nfixedargs || soft_float) ? ar + argn : fpr + argn;
          avaluep[i] = (char *) argp;
        }
      else if (arg_types[i]->type == SFFI_TYPE_COMPLEX && arg_types[i]->elements[0]->type == SFFI_TYPE_FLOAT)
        {
          if (argn >= 8 || i >= cif->mips_nfixedargs || soft_float)
	     argp = ar + argn;
	  else
	    {
	      argp = fpr + argn;
	      
	      uint32_t *tmp = (uint32_t *)argp;
	      tmp[1] = tmp[2];
	    }
          avaluep[i] = (char *) argp;
        }
      else
        {
          unsigned type = arg_types[i]->type;

          if (arg_types[i]->alignment > sizeof(sffi_arg))
            argn = SFFI_ALIGN(argn, arg_types[i]->alignment / sizeof(sffi_arg));

          argp = ar + argn;

          
          if (type == SFFI_TYPE_POINTER)
            type = (cif->abi == SFFI_N64 || cif->abi == SFFI_N64_SOFT_FLOAT)
	      ? SFFI_TYPE_SINT64 : SFFI_TYPE_UINT32;

	  if (soft_float && type ==  SFFI_TYPE_FLOAT)
	    type = SFFI_TYPE_SINT32;

          switch (type)
            {
            case SFFI_TYPE_SINT8:
              avaluep[i] = &avalue[i];
              *(SINT8 *) &avalue[i] = (SINT8) *argp;
              break;

            case SFFI_TYPE_UINT8:
              avaluep[i] = &avalue[i];
              *(UINT8 *) &avalue[i] = (UINT8) *argp;
              break;

            case SFFI_TYPE_SINT16:
              avaluep[i] = &avalue[i];
              *(SINT16 *) &avalue[i] = (SINT16) *argp;
              break;

            case SFFI_TYPE_UINT16:
              avaluep[i] = &avalue[i];
              *(UINT16 *) &avalue[i] = (UINT16) *argp;
              break;

            case SFFI_TYPE_SINT32:
              avaluep[i] = &avalue[i];
              *(SINT32 *) &avalue[i] = (SINT32) *argp;
              break;

            case SFFI_TYPE_UINT32:
              avaluep[i] = &avalue[i];
              *(UINT32 *) &avalue[i] = (UINT32) *argp;
              break;

            case SFFI_TYPE_STRUCT:
              if (argn < 8)
                {
                  
                  avaluep[i] = alloca(arg_types[i]->size);
                  copy_struct_N32(avaluep[i], 0, cif->abi, arg_types[i],
                                  argn, 0, ar, fpr, i >= cif->mips_nfixedargs || soft_float);

                  break;
                }
              
            default:
              avaluep[i] = (char *) argp;
              break;
            }
        }
      argn += SFFI_ALIGN(arg_types[i]->size, sizeof(sffi_arg)) / sizeof(sffi_arg);
      i++;
    }

  
  fun (cif, rvalue, avaluep, user_data);

  return cif->flags >> (SFFI_FLAG_BITS * 8);
}

#endif 

#if defined(SFFI_MIPS_O32)
extern void sffi_closure_O32(void);
extern void sffi_go_closure_O32(void);
#else
extern void sffi_closure_N32(void);
extern void sffi_go_closure_N32(void);
#endif 

sffi_status
sffi_prep_go_closure (sffi_go_closure* closure, sffi_cif* cif,
		     void (*fun)(sffi_cif*,void*,void**,void*))
{
  void * fn;

#if defined(SFFI_MIPS_O32)
  if (cif->abi != SFFI_O32 && cif->abi != SFFI_O32_SOFT_FLOAT)
    return SFFI_BAD_ABI;
  fn = sffi_go_closure_O32;
#else
#if _MIPS_SIM ==_ABIN32
  if (cif->abi != SFFI_N32
      && cif->abi != SFFI_N32_SOFT_FLOAT)
    return SFFI_BAD_ABI;
#else
  if (cif->abi != SFFI_N64
      && cif->abi != SFFI_N64_SOFT_FLOAT)
    return SFFI_BAD_ABI;
#endif
  fn = sffi_go_closure_N32;
#endif 

  closure->tramp = (void *)fn;
  closure->cif = cif;
  closure->fun = fun;

  return SFFI_OK;
}

#endif 
