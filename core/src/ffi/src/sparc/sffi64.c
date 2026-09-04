#include <sffi.h>
#include <sffi_common.h>
#include <stdlib.h>
#include "internal.h"

#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
# if SFFI_TYPE_LONGDOUBLE != 4
#  error SFFI_TYPE_LONGDOUBLE out of date
# endif
#else
# undef SFFI_TYPE_LONGDOUBLE
# define SFFI_TYPE_LONGDOUBLE 4
#endif

#ifdef SPARC64

static int
sffi_struct_float_mask (sffi_type *outer_type, int size_mask)
{
  sffi_type **elts;
  sffi_type *t;

  if (outer_type->type == SFFI_TYPE_COMPLEX)
    {
      int m = 0, tt = outer_type->elements[0]->type;
      size_t z = outer_type->size;

      if (tt == SFFI_TYPE_FLOAT
	  || tt == SFFI_TYPE_DOUBLE
	  || tt == SFFI_TYPE_LONGDOUBLE)
        m = (1 << (z / 4)) - 1;
      return (m << 8) | z;
    }
  SFFI_ASSERT (outer_type->type == SFFI_TYPE_STRUCT);

  for (elts = outer_type->elements; (t = *elts) != NULL; elts++)
    {
      size_t z = t->size;
      int o, m, tt;

      size_mask = SFFI_ALIGN(size_mask, t->alignment);
      switch (t->type)
	{
	case SFFI_TYPE_STRUCT:
	  size_mask = sffi_struct_float_mask (t, size_mask);
	  continue;
	case SFFI_TYPE_COMPLEX:
	  tt = t->elements[0]->type;
	  if (tt != SFFI_TYPE_FLOAT
	      && tt != SFFI_TYPE_DOUBLE
	      && tt != SFFI_TYPE_LONGDOUBLE)
	    break;

	case SFFI_TYPE_FLOAT:
	case SFFI_TYPE_DOUBLE:
	case SFFI_TYPE_LONGDOUBLE:
	  m = (1 << (z / 4)) - 1;	
	  o = (size_mask >> 2) & 0x3f;	
	  size_mask |= m << (o + 8);	
	  break;
	}
      size_mask += z;
    }

  size_mask = SFFI_ALIGN(size_mask, outer_type->alignment);
  SFFI_ASSERT ((size_mask & 0xff) == outer_type->size);

  return size_mask;
}

static void *
sffi_struct_float_merge (int size_mask, void *vi, void *vf)
{
  int size = size_mask & 0xff;
  int mask = size_mask >> 8;
  int n = size >> 2;

  if (mask == 0)
    return vi;
  else if (mask == (1 << n) - 1)
    return vf;
  else
    {
      unsigned int *wi = vi, *wf = vf;
      int i;

      for (i = 0; i < n; ++i)
	if ((mask >> i) & 1)
	  wi[i] = wf[i];

      return vi;
    }
}

void SFFI_HIDDEN
sffi_struct_float_copy (int size_mask, void *vd, void *vi, void *vf)
{
  int size = size_mask & 0xff;
  int mask = size_mask >> 8;
  int n = size >> 2;

  if (mask == 0)
    ;
  else if (mask == (1 << n) - 1)
    vi = vf;
  else
    {
      unsigned int *wd = vd, *wi = vi, *wf = vf;
      int i;

      for (i = 0; i < n; ++i)
	wd[i] = ((mask >> i) & 1 ? wf : wi)[i];
      return;
    }
  memcpy (vd, vi, size);
}

static sffi_status
sffi_prep_cif_machdep_core(sffi_cif *cif)
{
  sffi_type *rtype = cif->rtype;
  int rtt = rtype->type;
  size_t bytes = 0;
  int i, n, flags;

  switch (rtt)
    {
    case SFFI_TYPE_VOID:
      flags = SPARC_RET_VOID;
      break;
    case SFFI_TYPE_FLOAT:
      flags = SPARC_RET_F_1;
      break;
    case SFFI_TYPE_DOUBLE:
      flags = SPARC_RET_F_2;
      break;
    case SFFI_TYPE_LONGDOUBLE:
      flags = SPARC_RET_F_4;
      break;

    case SFFI_TYPE_COMPLEX:
    case SFFI_TYPE_STRUCT:
      if (rtype->size > 32)
	{
	  flags = SPARC_RET_VOID | SPARC_FLAG_RET_IN_MEM;
	  bytes = 8;
	}
      else
	{
	  int size_mask = sffi_struct_float_mask (rtype, 0);
	  int word_size = (size_mask >> 2) & 0x3f;
	  int all_mask = (1 << word_size) - 1;
	  int fp_mask = size_mask >> 8;

	  flags = (size_mask << SPARC_SIZEMASK_SHIFT) | SPARC_RET_STRUCT;

	  if (fp_mask == 0)
	    {
	      if (rtype->alignment >= 8)
		{
		  if (rtype->size == 8)
		    flags = SPARC_RET_INT64;
		  else if (rtype->size == 16)
		    flags = SPARC_RET_INT128;
		}
	    }
	  else if (fp_mask == all_mask)
	    switch (word_size)
	      {
	      case 1: flags = SPARC_RET_F_1; break;
	      case 2: flags = SPARC_RET_F_2; break;
	      case 3: flags = SP_V9_RET_F_3; break;
	      case 4: flags = SPARC_RET_F_4; break;

	      case 6: flags = SPARC_RET_F_6; break;

	      case 8: flags = SPARC_RET_F_8; break;
	      }
	}
      break;

    case SFFI_TYPE_SINT8:
      flags = SPARC_RET_SINT8;
      break;
    case SFFI_TYPE_UINT8:
      flags = SPARC_RET_UINT8;
      break;
    case SFFI_TYPE_SINT16:
      flags = SPARC_RET_SINT16;
      break;
    case SFFI_TYPE_UINT16:
      flags = SPARC_RET_UINT16;
      break;
    case SFFI_TYPE_INT:
    case SFFI_TYPE_SINT32:
      flags = SP_V9_RET_SINT32;
      break;
    case SFFI_TYPE_UINT32:
      flags = SPARC_RET_UINT32;
      break;
    case SFFI_TYPE_SINT64:
    case SFFI_TYPE_UINT64:
    case SFFI_TYPE_POINTER:
      flags = SPARC_RET_INT64;
      break;

    default:
      abort();
    }

  bytes = 0;
  for (i = 0, n = cif->nargs; i < n; ++i)
    {
      sffi_type *ty = cif->arg_types[i];
      size_t z = ty->size;
      size_t a = ty->alignment;

      switch (ty->type)
	{
	case SFFI_TYPE_COMPLEX:
	case SFFI_TYPE_STRUCT:

	  if (z > 16)
	    {
	      a = z = 8;
	      break;
	    }

	  if (bytes >= 16*8)
	    break;
	  if ((sffi_struct_float_mask (ty, 0) & 0xff00) == 0)
	    break;

	case SFFI_TYPE_FLOAT:
	case SFFI_TYPE_DOUBLE:
	case SFFI_TYPE_LONGDOUBLE:
	  flags |= SPARC_FLAG_FP_ARGS;
	  break;
	}
      bytes = SFFI_ALIGN(bytes, a);
      bytes += SFFI_ALIGN(z, 8);
    }

  if (bytes < 6 * 8)
    bytes = 6 * 8;

  bytes = SFFI_ALIGN(bytes, 16);

  bytes += 8*16 + 8*8;

  cif->bytes = bytes;
  cif->flags = flags;
  return SFFI_OK;
}

sffi_status SFFI_HIDDEN
sffi_prep_cif_machdep(sffi_cif *cif)
{
  cif->nfixedargs = cif->nargs;
  return sffi_prep_cif_machdep_core(cif);
}

sffi_status SFFI_HIDDEN
sffi_prep_cif_machdep_var(sffi_cif *cif, unsigned nfixedargs, unsigned ntotalargs)
{
  cif->nfixedargs = nfixedargs;
  return sffi_prep_cif_machdep_core(cif);
}

extern void sffi_call_v9(sffi_cif *cif, void (*fn)(void), void *rvalue,
			void **avalue, size_t bytes, void *closure) SFFI_HIDDEN;

int SFFI_HIDDEN
sffi_prep_args_v9(sffi_cif *cif, unsigned long *argp, void *rvalue, void **avalue)
{
  sffi_type **p_arg;
  int flags = cif->flags;
  int i, nargs;

  if (rvalue == NULL)
    {
      if (flags & SPARC_FLAG_RET_IN_MEM)
	{

	  rvalue = (char *)argp + cif->bytes;
	}
      else
	{

	  flags = SPARC_RET_VOID;
	}
    }

#ifdef USING_PURIFY

  memset(argp, 0, 6*8);
#endif

  if (flags & SPARC_FLAG_RET_IN_MEM)
    *argp++ = (unsigned long)rvalue;

  p_arg = cif->arg_types;
  for (i = 0, nargs = cif->nargs; i < nargs; i++)
    {
      sffi_type *ty = p_arg[i];
      void *a = avalue[i];
      size_t z;

      switch (ty->type)
	{
	case SFFI_TYPE_SINT8:
	  *argp++ = *(SINT8 *)a;
	  break;
	case SFFI_TYPE_UINT8:
	  *argp++ = *(UINT8 *)a;
	  break;
	case SFFI_TYPE_SINT16:
	  *argp++ = *(SINT16 *)a;
	  break;
	case SFFI_TYPE_UINT16:
	  *argp++ = *(UINT16 *)a;
	  break;
	case SFFI_TYPE_INT:
	case SFFI_TYPE_SINT32:
	  *argp++ = *(SINT32 *)a;
	  break;
	case SFFI_TYPE_UINT32:
	  *argp++ = *(UINT32 *)a;
	  break;
	case SFFI_TYPE_SINT64:
	case SFFI_TYPE_UINT64:
	case SFFI_TYPE_POINTER:
	  *argp++ = *(UINT64 *)a;
	  break;
	case SFFI_TYPE_FLOAT:
	  flags |= SPARC_FLAG_FP_ARGS;
	  *argp++ = *(UINT32 *)a;
	  break;
	case SFFI_TYPE_DOUBLE:
	  flags |= SPARC_FLAG_FP_ARGS;
	  *argp++ = *(UINT64 *)a;
	  break;

	case SFFI_TYPE_LONGDOUBLE:
	case SFFI_TYPE_COMPLEX:
	case SFFI_TYPE_STRUCT:
	  z = ty->size;
	  if (z > 16)
	    {

	      *argp++ = (unsigned long)a;
	      break;
	    }
	  if (((unsigned long)argp & 15) && ty->alignment > 8)
	    argp++;
	  memcpy(argp, a, z);
	  argp += SFFI_ALIGN(z, 8) / 8;
	  break;

	default:
	  abort();
	}
    }

  return flags;
}

static void
sffi_call_int(sffi_cif *cif, void (*fn)(void), void *rvalue,
	     void **avalue, void *closure)
{
  size_t bytes = cif->bytes;
  size_t i, nargs = cif->nargs;
  sffi_type **arg_types = cif->arg_types;
  void **avalue_copy = NULL;

  SFFI_ASSERT (cif->abi == SFFI_V9);

  if (rvalue == NULL && (cif->flags & SPARC_FLAG_RET_IN_MEM))
    bytes += SFFI_ALIGN (cif->rtype->size, 16);

  for (i = 0; i < nargs; i++)
    {
      sffi_type *at = arg_types[i];
      int size = at->size;
      if (at->type == SFFI_TYPE_STRUCT && size > 4)
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

  sffi_call_v9(cif, fn, rvalue, avalue, -bytes, closure);
}

void
sffi_call(sffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  sffi_call_int(cif, fn, rvalue, avalue, NULL);
}

void
sffi_call_go(sffi_cif *cif, void (*fn)(void), void *rvalue,
	    void **avalue, void *closure)
{
  sffi_call_int(cif, fn, rvalue, avalue, closure);
}

#ifdef __GNUC__
static inline void
sffi_flush_icache (void *p)
{
  __asm__ volatile ("flush	%0; flush %0+8" : : "r" (p) : "memory");
}
#else
extern void sffi_flush_icache (void *) SFFI_HIDDEN;
#endif

extern void sffi_closure_v9(void) SFFI_HIDDEN;
extern void sffi_go_closure_v9(void) SFFI_HIDDEN;

sffi_status
sffi_prep_closure_loc (sffi_closure* closure,
		      sffi_cif* cif,
		      void (*fun)(sffi_cif*, void*, void**, void*),
		      void *user_data,
		      void *codeloc)
{
  unsigned int *tramp = (unsigned int *) &closure->tramp[0];
  unsigned long fn;

  if (cif->abi != SFFI_V9)
    return SFFI_BAD_ABI;

  fn = (unsigned long) sffi_closure_v9;
  tramp[0] = 0x83414000;	
  tramp[1] = 0xca586010;	
  tramp[2] = 0x81c14000;	
  tramp[3] = 0x01000000;	
  *((unsigned long *) &tramp[4]) = fn;

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  sffi_flush_icache (closure);

  return SFFI_OK;
}

sffi_status
sffi_prep_go_closure (sffi_go_closure* closure, sffi_cif* cif,
		     void (*fun)(sffi_cif*, void*, void**, void*))
{
  if (cif->abi != SFFI_V9)
    return SFFI_BAD_ABI;

  closure->tramp = sffi_go_closure_v9;
  closure->cif = cif;
  closure->fun = fun;

  return SFFI_OK;
}

int SFFI_HIDDEN
sffi_closure_sparc_inner_v9(sffi_cif *cif,
			   void (*fun)(sffi_cif*, void*, void**, void*),
			   void *user_data, void *rvalue,
			   unsigned long *gpr, unsigned long *fpr)
{
  sffi_type **arg_types;
  void **avalue;
  int i, argn, argx, nargs, flags, nfixedargs;

  arg_types = cif->arg_types;
  nargs = cif->nargs;
  flags = cif->flags;
  nfixedargs = cif->nfixedargs;

  avalue = alloca(nargs * sizeof(void *));

  if (flags & SPARC_FLAG_RET_IN_MEM)
    {
      rvalue = (void *) gpr[0];

      argn = 1;
    }
  else
    argn = 0;

  for (i = 0; i < nargs; i++, argn = argx)
    {
      int named = i < nfixedargs;
      sffi_type *ty = arg_types[i];
      void *a = &gpr[argn];
      size_t z;

      argx = argn + 1;
      switch (ty->type)
	{
	case SFFI_TYPE_COMPLEX:
	case SFFI_TYPE_STRUCT:
	  z = ty->size;
	  if (z > 16)
	    a = *(void **)a;
	  else
	    {
	      argx = argn + SFFI_ALIGN (z, 8) / 8;
	      if (named && argn < 16)
		{
		  int size_mask = sffi_struct_float_mask (ty, 0);
		  int argn_mask = (0xffff00 >> argn) & 0xff00;

		  size_mask = (size_mask & 0xff) | (size_mask & argn_mask);
		  a = sffi_struct_float_merge (size_mask, gpr+argn, fpr+argn);
		}
	    }
	  break;

	case SFFI_TYPE_LONGDOUBLE:
	  argn = SFFI_ALIGN (argn, 2);
	  a = (named && argn < 16 ? fpr : gpr) + argn;
	  argx = argn + 2;
	  break;
	case SFFI_TYPE_DOUBLE:
	  if (named && argn < 16)
	    a = fpr + argn;
	  break;
	case SFFI_TYPE_FLOAT:
	  if (named && argn < 16)
	    a = fpr + argn;
	  a += 4;
	  break;

	case SFFI_TYPE_UINT64:
	case SFFI_TYPE_SINT64:
	case SFFI_TYPE_POINTER:
	  break;
	case SFFI_TYPE_INT:
	case SFFI_TYPE_UINT32:
	case SFFI_TYPE_SINT32:
	  a += 4;
	  break;
        case SFFI_TYPE_UINT16:
        case SFFI_TYPE_SINT16:
	  a += 6;
	  break;
        case SFFI_TYPE_UINT8:
        case SFFI_TYPE_SINT8:
	  a += 7;
	  break;

	default:
	  abort();
	}
      avalue[i] = a;
    }

  fun (cif, rvalue, avalue, user_data);

  return flags;
}
#endif 
