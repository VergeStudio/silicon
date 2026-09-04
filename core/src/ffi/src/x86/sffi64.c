

#include <sffi.h>
#include <sffi_common.h>

#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <tramp.h>
#include "internal64.h"

#ifdef __x86_64__

#define MAX_GPR_REGS 6
#define MAX_SSE_REGS 8

#if defined(__INTEL_COMPILER)
#include "xmmintrin.h"
#define UINT128 __m128
#else
#if defined(__SUNPRO_C)
#include <sunmedia_types.h>
#define UINT128 __m128i
#else
#define UINT128 __int128_t
#endif
#endif

union big_int_union
{
  UINT32 i32;
  UINT64 i64;
  UINT128 i128;
};

struct register_args
{
  
  UINT64 gpr[MAX_GPR_REGS];
  union big_int_union sse[MAX_SSE_REGS];
  UINT64 rax;	
  UINT64 r10;	
};

extern void sffi_call_unix64 (void *args, unsigned long bytes, unsigned flags,
			     void *raddr, void (*fnaddr)(void)) SFFI_HIDDEN;




enum x86_64_reg_class
  {
    X86_64_NO_CLASS,
    X86_64_INTEGER_CLASS,
    X86_64_INTEGERSI_CLASS,
    X86_64_SSE_CLASS,
    X86_64_SSESF_CLASS,
    X86_64_SSEDF_CLASS,
    X86_64_SSEUP_CLASS,
    X86_64_X87_CLASS,
    X86_64_X87UP_CLASS,
    X86_64_COMPLEX_X87_CLASS,
    X86_64_MEMORY_CLASS
  };

#define MAX_CLASSES 4

#define SSE_CLASS_P(X)	((X) >= X86_64_SSE_CLASS && X <= X86_64_SSEUP_CLASS)


#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE \
    && defined(__LDBL_MANT_DIG__) && __LDBL_MANT_DIG__ == 113
# define SFFI_LONGDOUBLE_BINARY128 1
#else
# define SFFI_LONGDOUBLE_BINARY128 0
#endif





static enum x86_64_reg_class
merge_classes (enum x86_64_reg_class class1, enum x86_64_reg_class class2)
{
  
  if (class1 == class2)
    return class1;

  
  if (class1 == X86_64_NO_CLASS)
    return class2;
  if (class2 == X86_64_NO_CLASS)
    return class1;

  
  if (class1 == X86_64_MEMORY_CLASS || class2 == X86_64_MEMORY_CLASS)
    return X86_64_MEMORY_CLASS;

  
  if ((class1 == X86_64_INTEGERSI_CLASS && class2 == X86_64_SSESF_CLASS)
      || (class2 == X86_64_INTEGERSI_CLASS && class1 == X86_64_SSESF_CLASS))
    return X86_64_INTEGERSI_CLASS;
  if (class1 == X86_64_INTEGER_CLASS || class1 == X86_64_INTEGERSI_CLASS
      || class2 == X86_64_INTEGER_CLASS || class2 == X86_64_INTEGERSI_CLASS)
    return X86_64_INTEGER_CLASS;

  
  if (class1 == X86_64_X87_CLASS
      || class1 == X86_64_X87UP_CLASS
      || class1 == X86_64_COMPLEX_X87_CLASS
      || class2 == X86_64_X87_CLASS
      || class2 == X86_64_X87UP_CLASS
      || class2 == X86_64_COMPLEX_X87_CLASS)
    return X86_64_MEMORY_CLASS;

  
  return X86_64_SSE_CLASS;
}


static size_t
classify_argument (sffi_type *type, enum x86_64_reg_class classes[],
		   size_t byte_offset)
{
  switch (type->type)
    {
    case SFFI_TYPE_UINT8:
    case SFFI_TYPE_SINT8:
    case SFFI_TYPE_UINT16:
    case SFFI_TYPE_SINT16:
    case SFFI_TYPE_UINT32:
    case SFFI_TYPE_SINT32:
    case SFFI_TYPE_UINT64:
    case SFFI_TYPE_SINT64:
    case SFFI_TYPE_UINT128:
    case SFFI_TYPE_SINT128:
    case SFFI_TYPE_POINTER:
    do_integer:
      {
	size_t size = byte_offset + type->size;

	if (size <= 4)
	  {
	    classes[0] = X86_64_INTEGERSI_CLASS;
	    return 1;
	  }
	else if (size <= 8)
	  {
	    classes[0] = X86_64_INTEGER_CLASS;
	    return 1;
	  }
	else if (size <= 12)
	  {
	    classes[0] = X86_64_INTEGER_CLASS;
	    classes[1] = X86_64_INTEGERSI_CLASS;
	    return 2;
	  }
	else if (size <= 16)
	  {
	    classes[0] = classes[1] = X86_64_INTEGER_CLASS;
	    return 2;
	  }
	else
	  SFFI_ASSERT (0);
      }
    case SFFI_TYPE_FLOAT:
      if (!(byte_offset % 8))
	classes[0] = X86_64_SSESF_CLASS;
      else
	classes[0] = X86_64_SSE_CLASS;
      return 1;
    case SFFI_TYPE_DOUBLE:
      classes[0] = X86_64_SSEDF_CLASS;
      return 1;
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
    case SFFI_TYPE_LONGDOUBLE:
#if SFFI_LONGDOUBLE_BINARY128
      
      classes[0] = X86_64_SSE_CLASS;
      classes[1] = X86_64_SSEUP_CLASS;
#else
      classes[0] = X86_64_X87_CLASS;
      classes[1] = X86_64_X87UP_CLASS;
#endif
      return 2;
#endif
    case SFFI_TYPE_STRUCT:
      {
	const size_t UNITS_PER_WORD = 8;
        size_t words = (type->size + byte_offset + UNITS_PER_WORD - 1)
                       / UNITS_PER_WORD;
	sffi_type **ptr;
	unsigned int i;
	enum x86_64_reg_class subclasses[MAX_CLASSES];

	
	if (type->size > 32)
	  return 0;

	for (i = 0; i < words; i++)
	  classes[i] = X86_64_NO_CLASS;

	
	if (!words)
	  {
    case SFFI_TYPE_VOID:
	    classes[0] = X86_64_NO_CLASS;
	    return 1;
	  }

	
	for (ptr = type->elements; *ptr != NULL; ptr++)
	  {
	    size_t num, pos;

	    byte_offset = SFFI_ALIGN (byte_offset, (*ptr)->alignment);

	    num = classify_argument (*ptr, subclasses, byte_offset % 8);
	    if (num == 0)
	      return 0;
            pos = byte_offset / 8;
            for (i = 0; i < num && (i + pos) < words; i++)
	      {
		size_t pos = byte_offset / 8;
		classes[i + pos] =
		  merge_classes (subclasses[i], classes[i + pos]);
	      }

	    byte_offset += (*ptr)->size;
	  }

	if (words > 2)
	  {
	    
	    if (classes[0] != X86_64_SSE_CLASS)
	      return 0;

	    for (i = 1; i < words; i++)
	      if (classes[i] != X86_64_SSEUP_CLASS)
		return 0;
	  }

	
	for (i = 0; i < words; i++)
	  {
	    
	    if (classes[i] == X86_64_MEMORY_CLASS)
	      return 0;

	    
	    if (i > 1 && classes[i] == X86_64_SSEUP_CLASS
		&& classes[i - 1] != X86_64_SSE_CLASS
		&& classes[i - 1] != X86_64_SSEUP_CLASS)
	      {
		
		SFFI_ASSERT (i != 0);
		classes[i] = X86_64_SSE_CLASS;
	      }

	    
	    if (i > 1 && classes[i] == X86_64_X87UP_CLASS
		&& (classes[i - 1] != X86_64_X87_CLASS))
	      {
		
		SFFI_ASSERT (i != 0);
		return 0;
	      }
	  }
	return words;
      }
    case SFFI_TYPE_COMPLEX:
      {
	sffi_type *inner = type->elements[0];
	switch (inner->type)
	  {
	  case SFFI_TYPE_INT:
	  case SFFI_TYPE_UINT8:
	  case SFFI_TYPE_SINT8:
	  case SFFI_TYPE_UINT16:
	  case SFFI_TYPE_SINT16:
	  case SFFI_TYPE_UINT32:
	  case SFFI_TYPE_SINT32:
	  case SFFI_TYPE_UINT64:
	  case SFFI_TYPE_SINT64:
	    goto do_integer;

	  case SFFI_TYPE_SINT128:
	  case SFFI_TYPE_UINT128:
	    return 0;

	  case SFFI_TYPE_FLOAT:
	    classes[0] = X86_64_SSE_CLASS;
	    if (byte_offset % 8)
	      {
		classes[1] = X86_64_SSESF_CLASS;
		return 2;
	      }
	    return 1;
	  case SFFI_TYPE_DOUBLE:
	    classes[0] = classes[1] = X86_64_SSEDF_CLASS;
	    return 2;
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
	  case SFFI_TYPE_LONGDOUBLE:
#if SFFI_LONGDOUBLE_BINARY128
	    
	    return 0;
#else
	    classes[0] = X86_64_COMPLEX_X87_CLASS;
	    return 1;
#endif
#endif
	  }
      }
    }
  abort();
}



static size_t
examine_argument (sffi_type *type, enum x86_64_reg_class classes[MAX_CLASSES],
		  _Bool in_return, int *pngpr, int *pnsse)
{
  size_t n;
  unsigned int i;
  int ngpr, nsse;

  n = classify_argument (type, classes, 0);
  if (n == 0)
    return 0;

  ngpr = nsse = 0;
  for (i = 0; i < n; ++i)
    switch (classes[i])
      {
      case X86_64_INTEGER_CLASS:
      case X86_64_INTEGERSI_CLASS:
	ngpr++;
	break;
      case X86_64_SSE_CLASS:
      case X86_64_SSESF_CLASS:
      case X86_64_SSEDF_CLASS:
	nsse++;
	break;
      case X86_64_NO_CLASS:
      case X86_64_SSEUP_CLASS:
	break;
      case X86_64_X87_CLASS:
      case X86_64_X87UP_CLASS:
      case X86_64_COMPLEX_X87_CLASS:
	return in_return != 0;
      default:
	abort ();
      }

  *pngpr = ngpr;
  *pnsse = nsse;

  return n;
}



#ifndef __ILP32__
extern sffi_status
sffi_prep_cif_machdep_efi64(sffi_cif *cif);
#endif

sffi_status SFFI_HIDDEN
sffi_prep_cif_machdep (sffi_cif *cif)
{
  int gprcount, ssecount, i, avn, ngpr, nsse;
  unsigned flags;
  enum x86_64_reg_class classes[MAX_CLASSES];
  size_t bytes, n, rtype_size;
  sffi_type *rtype;

#ifndef __ILP32__
  if (cif->abi == SFFI_EFI64 || cif->abi == SFFI_GNUW64)
    return sffi_prep_cif_machdep_efi64(cif);
#endif
  if (cif->abi != SFFI_UNIX64)
    return SFFI_BAD_ABI;

  gprcount = ssecount = 0;

  rtype = cif->rtype;
  rtype_size = rtype->size;
  switch (rtype->type)
    {
    case SFFI_TYPE_VOID:
      flags = UNIX64_RET_VOID;
      break;
    case SFFI_TYPE_UINT8:
      flags = UNIX64_RET_UINT8;
      break;
    case SFFI_TYPE_SINT8:
      flags = UNIX64_RET_SINT8;
      break;
    case SFFI_TYPE_UINT16:
      flags = UNIX64_RET_UINT16;
      break;
    case SFFI_TYPE_SINT16:
      flags = UNIX64_RET_SINT16;
      break;
    case SFFI_TYPE_UINT32:
      flags = UNIX64_RET_UINT32;
      break;
    case SFFI_TYPE_INT:
    case SFFI_TYPE_SINT32:
      flags = UNIX64_RET_SINT32;
      break;
    case SFFI_TYPE_UINT64:
    case SFFI_TYPE_SINT64:
      flags = UNIX64_RET_INT64;
      break;
    case SFFI_TYPE_UINT128:
    case SFFI_TYPE_SINT128:
      flags = UNIX64_RET_ST_RAX_RDX | (16 << UNIX64_SIZE_SHIFT);
      break;
    case SFFI_TYPE_POINTER:
      flags = (sizeof(void *) == 4 ? UNIX64_RET_UINT32 : UNIX64_RET_INT64);
      break;
    case SFFI_TYPE_FLOAT:
      flags = UNIX64_RET_XMM32;
      break;
    case SFFI_TYPE_DOUBLE:
      flags = UNIX64_RET_XMM64;
      break;
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
    case SFFI_TYPE_LONGDOUBLE:
#if SFFI_LONGDOUBLE_BINARY128
      flags = UNIX64_RET_XMM128;	
#else
      flags = UNIX64_RET_X87;
#endif
      break;
#endif
    case SFFI_TYPE_STRUCT:
      n = examine_argument (cif->rtype, classes, 1, &ngpr, &nsse);
      if (n == 0)
	{
	  
	  gprcount++;
	  
	  flags = UNIX64_RET_VOID | UNIX64_FLAG_RET_IN_MEM;
	}
      else
	{
	  _Bool sse0 = SSE_CLASS_P (classes[0]);

	  if (rtype_size == 4 && sse0)
	    flags = UNIX64_RET_XMM32;
	  else if (rtype_size == 8)
	    flags = sse0 ? UNIX64_RET_XMM64 : UNIX64_RET_INT64;
	  else
	    {
	      _Bool sse1 = n == 2 && SSE_CLASS_P (classes[1]);
	      if (sse0 && sse1)
		flags = UNIX64_RET_ST_XMM0_XMM1;
	      else if (sse0)
		flags = UNIX64_RET_ST_XMM0_RAX;
	      else if (sse1)
		flags = UNIX64_RET_ST_RAX_XMM0;
	      else
		flags = UNIX64_RET_ST_RAX_RDX;
	      flags |= rtype_size << UNIX64_SIZE_SHIFT;
	    }
	}
      break;
    case SFFI_TYPE_COMPLEX:
      switch (rtype->elements[0]->type)
	{
	case SFFI_TYPE_UINT8:
	case SFFI_TYPE_SINT8:
	case SFFI_TYPE_UINT16:
	case SFFI_TYPE_SINT16:
	case SFFI_TYPE_INT:
	case SFFI_TYPE_UINT32:
	case SFFI_TYPE_SINT32:
	case SFFI_TYPE_UINT64:
	case SFFI_TYPE_SINT64:
	  flags = UNIX64_RET_ST_RAX_RDX | ((unsigned) rtype_size << UNIX64_SIZE_SHIFT);
	  break;
	case SFFI_TYPE_FLOAT:
	  flags = UNIX64_RET_XMM64;
	  break;
	case SFFI_TYPE_DOUBLE:
	  flags = UNIX64_RET_ST_XMM0_XMM1 | (16 << UNIX64_SIZE_SHIFT);
	  break;
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
	case SFFI_TYPE_LONGDOUBLE:
#if SFFI_LONGDOUBLE_BINARY128
	  
	  gprcount++;
	  flags = UNIX64_RET_VOID | UNIX64_FLAG_RET_IN_MEM;
#else
	  flags = UNIX64_RET_X87_2;
#endif
	  break;
#endif
	case SFFI_TYPE_SINT128:
	case SFFI_TYPE_UINT128:
	  gprcount++;
	  flags = UNIX64_RET_VOID | UNIX64_FLAG_RET_IN_MEM;
	  break;
	default:
	  return SFFI_BAD_TYPEDEF;
	}
      break;
    default:
      return SFFI_BAD_TYPEDEF;
    }

  
  for (bytes = 0, i = 0, avn = cif->nargs; i < avn; i++)
    {
      if (examine_argument (cif->arg_types[i], classes, 0, &ngpr, &nsse) == 0
	  || gprcount + ngpr > MAX_GPR_REGS
	  || ssecount + nsse > MAX_SSE_REGS)
	{
	  long align = cif->arg_types[i]->alignment;

	  if (align < 8)
	    align = 8;

	  bytes = SFFI_ALIGN (bytes, align);
	  bytes += cif->arg_types[i]->size;
	}
      else
	{
	  gprcount += ngpr;
	  ssecount += nsse;
	}
    }
  if (ssecount)
    flags |= UNIX64_FLAG_XMM_ARGS;

  cif->flags = flags;
  cif->bytes = (unsigned) SFFI_ALIGN (bytes, 8);

  return SFFI_OK;
}


SFFI_ASAN_NO_SANITIZE
static void
sffi_call_int (sffi_cif *cif, void (*fn)(void), void *rvalue,
	      void **avalue, void *closure)
{
  enum x86_64_reg_class classes[MAX_CLASSES];
  char *stack, *argp;
  sffi_type **arg_types;
  int gprcount, ssecount, ngpr, nsse, i, avn, flags;
  struct register_args *reg_args;

  
  SFFI_ASSERT (cif->abi == SFFI_UNIX64);

  
  flags = cif->flags;
  if (rvalue == NULL)
    {
      if (flags & UNIX64_FLAG_RET_IN_MEM)
	rvalue = alloca (cif->rtype->size);
      else
	flags = UNIX64_RET_VOID;
    }

  arg_types = cif->arg_types;
  avn = cif->nargs;

  
  stack = alloca (sizeof (struct register_args) + cif->bytes + 4*8);
  reg_args = (struct register_args *) stack;
  argp = stack + sizeof (struct register_args);

  reg_args->r10 = (uintptr_t) closure;

  gprcount = ssecount = 0;

  
  if (flags & UNIX64_FLAG_RET_IN_MEM)
    reg_args->gpr[gprcount++] = (unsigned long) rvalue;

  for (i = 0; i < avn; ++i)
    {
      size_t n, size = arg_types[i]->size;

      n = examine_argument (arg_types[i], classes, 0, &ngpr, &nsse);
      if (n == 0
	  || gprcount + ngpr > MAX_GPR_REGS
	  || ssecount + nsse > MAX_SSE_REGS)
	{
	  long align = arg_types[i]->alignment;

	  
	  if (align < 8)
	    align = 8;

          
          argp = (void *) SFFI_ALIGN (argp, align);
          memcpy (argp, avalue[i], size);

          argp += size;
        }
      else
	{
	  
	  char *a = (char *) avalue[i];
	  unsigned int j;

	  for (j = 0; j < n; j++, a += 8, size -= 8)
	    {
	      switch (classes[j])
		{
		case X86_64_NO_CLASS:
		  break;
		case X86_64_SSEUP_CLASS:
		  
		  memcpy ((char *) &reg_args->sse[ssecount - 1] + 8, a,
			  size < 8 ? size : 8);
		  break;
		case X86_64_INTEGER_CLASS:
		case X86_64_INTEGERSI_CLASS:
		  
		  switch (arg_types[i]->type)
		    {
		    case SFFI_TYPE_SINT8:
		      reg_args->gpr[gprcount] = (SINT64) *((SINT8 *) a);
		      break;
		    case SFFI_TYPE_SINT16:
		      reg_args->gpr[gprcount] = (SINT64) *((SINT16 *) a);
		      break;
		    case SFFI_TYPE_SINT32:
		      reg_args->gpr[gprcount] = (SINT64) *((SINT32 *) a);
		      break;
		    default:
		      reg_args->gpr[gprcount] = 0;
		      memcpy (&reg_args->gpr[gprcount], a, size <= 8 ? size : 8);
		    }
		  gprcount++;
		  break;
		case X86_64_SSE_CLASS:
		case X86_64_SSEDF_CLASS:
		  memcpy (&reg_args->sse[ssecount++].i64, a, sizeof(UINT64));
		  break;
		case X86_64_SSESF_CLASS:
		  memcpy (&reg_args->sse[ssecount++].i32, a, sizeof(UINT32));
		  break;
		default:
		  abort();
		}
	    }
	}
    }
  reg_args->rax = ssecount;

  sffi_call_unix64 (stack, cif->bytes + sizeof (struct register_args),
		   flags, rvalue, fn);
}

#ifndef __ILP32__


enum sffi_move_op
{
  SFFI_MOVE_SE8, SFFI_MOVE_SE16, SFFI_MOVE_SE32,  
  SFFI_MOVE_GP64,                               
  SFFI_MOVE_GP,                                 
  SFFI_MOVE_SSE64, SFFI_MOVE_SSE32,              
  SFFI_MOVE_STACK                               
};

typedef struct
{
  unsigned src_idx;     
  unsigned src_off;     
  unsigned dst_off;     
  unsigned len;         
  unsigned char op;
} sffi_move;

typedef struct
{
  unsigned nmoves;
  unsigned ssecount;    
  unsigned bytes;       
  unsigned flags;       
  unsigned ret_in_mem;  
  unsigned fast;        
  unsigned retcode;     
  int      thunk_n;     
  sffi_move moves[];
} sffi_plan;


struct sffi_ret2 { UINT64 i; double d; };
extern struct sffi_ret2 sffi_plan_fast_call (struct register_args *img,
					   void (*fn) (void)) SFFI_HIDDEN;


extern struct sffi_ret2 sffi_plan_gp0 (void **, void (*)(void)) SFFI_HIDDEN;
extern struct sffi_ret2 sffi_plan_gp1 (void **, void (*)(void)) SFFI_HIDDEN;
extern struct sffi_ret2 sffi_plan_gp2 (void **, void (*)(void)) SFFI_HIDDEN;
extern struct sffi_ret2 sffi_plan_gp3 (void **, void (*)(void)) SFFI_HIDDEN;
extern struct sffi_ret2 sffi_plan_gp4 (void **, void (*)(void)) SFFI_HIDDEN;
extern struct sffi_ret2 sffi_plan_gp5 (void **, void (*)(void)) SFFI_HIDDEN;
extern struct sffi_ret2 sffi_plan_gp6 (void **, void (*)(void)) SFFI_HIDDEN;
static struct sffi_ret2 (*const sffi_gp_thunks[7]) (void **, void (*)(void)) =
  { sffi_plan_gp0, sffi_plan_gp1, sffi_plan_gp2, sffi_plan_gp3,
    sffi_plan_gp4, sffi_plan_gp5, sffi_plan_gp6 };


static inline void
store_ret (void *rvalue, unsigned retcode, struct sffi_ret2 r)
{
  switch (retcode)
    {
    case UNIX64_RET_VOID:   break;
    case UNIX64_RET_UINT8:  *(UINT64 *) rvalue = (UINT8)  r.i; break;
    case UNIX64_RET_UINT16: *(UINT64 *) rvalue = (UINT16) r.i; break;
    case UNIX64_RET_UINT32: *(UINT64 *) rvalue = (UINT32) r.i; break;
    case UNIX64_RET_SINT8:  *(UINT64 *) rvalue = (UINT64)(SINT64)(SINT8)  r.i; break;
    case UNIX64_RET_SINT16: *(UINT64 *) rvalue = (UINT64)(SINT64)(SINT16) r.i; break;
    case UNIX64_RET_SINT32: *(UINT64 *) rvalue = (UINT64)(SINT64)(SINT32) r.i; break;
    case UNIX64_RET_INT64:  *(UINT64 *) rvalue = r.i; break;
    case UNIX64_RET_XMM32:  memcpy (rvalue, &r.d, 4); break;
    case UNIX64_RET_XMM64:  memcpy (rvalue, &r.d, 8); break;
    }
}


static sffi_plan *
build_plan (sffi_cif *cif)
{
  unsigned i, avn = cif->nargs;
  enum x86_64_reg_class classes[MAX_CLASSES];
  unsigned nm, gprcount, ssecount;
  size_t argp_off;
  sffi_plan *plan;
  int all_gp64 = 1;	

  if (cif->abi != SFFI_UNIX64)
    return NULL;

  
  for (i = 0; i < avn; i++)
    {
      int t = cif->arg_types[i]->type;
      if (t == SFFI_TYPE_STRUCT || t == SFFI_TYPE_COMPLEX)
	return NULL;
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
      if (t == SFFI_TYPE_LONGDOUBLE)
	return NULL;
#endif
    }

  
  plan = malloc (sizeof (sffi_plan) + sizeof (sffi_move) * (2 * avn + 1));
  if (plan == NULL)
    return NULL;

  nm = gprcount = ssecount = 0;
  argp_off = 0;
  plan->ret_in_mem = (cif->flags & UNIX64_FLAG_RET_IN_MEM) ? 1 : 0;
  if (plan->ret_in_mem)
    gprcount++;				

  for (i = 0; i < avn; i++)
    {
      sffi_type *at = cif->arg_types[i];
      size_t size = at->size, n, rem;
      int ngpr, nsse;
      unsigned j;

      n = examine_argument (at, classes, 0, &ngpr, &nsse);
      if (n == 0
	  || gprcount + ngpr > MAX_GPR_REGS
	  || ssecount + nsse > MAX_SSE_REGS)
	{
	  long align = at->alignment;
	  sffi_move *m = &plan->moves[nm++];
	  all_gp64 = 0;
	  if (align < 8)
	    align = 8;
	  argp_off = SFFI_ALIGN (argp_off, align);
	  m->op = SFFI_MOVE_STACK;
	  m->src_idx = i;
	  m->src_off = 0;
	  m->dst_off = (unsigned) (sizeof (struct register_args) + argp_off);
	  m->len = (unsigned) size;
	  argp_off += size;
	  continue;
	}

      for (j = 0, rem = size; j < n; j++, rem -= 8)
	{
	  sffi_move m;
	  m.src_idx = i;
	  m.src_off = j * 8;
	  switch (classes[j])
	    {
	    case X86_64_NO_CLASS:
	    case X86_64_SSEUP_CLASS:
	      continue;			
	    case X86_64_INTEGER_CLASS:
	    case X86_64_INTEGERSI_CLASS:
	      m.dst_off = gprcount * 8;	
	      switch (at->type)
		{
		case SFFI_TYPE_SINT8:  m.op = SFFI_MOVE_SE8;  all_gp64 = 0; break;
		case SFFI_TYPE_SINT16: m.op = SFFI_MOVE_SE16; all_gp64 = 0; break;
		case SFFI_TYPE_SINT32: m.op = SFFI_MOVE_SE32; all_gp64 = 0; break;
		default:
		  if (rem >= 8)
		    m.op = SFFI_MOVE_GP64;
		  else
		    { m.op = SFFI_MOVE_GP; m.len = (unsigned) rem; all_gp64 = 0; }
		  break;
		}
	      gprcount++;
	      break;
	    case X86_64_SSE_CLASS:
	    case X86_64_SSEDF_CLASS:
	      m.dst_off = (unsigned) (offsetof (struct register_args, sse)
				      + ssecount * sizeof (union big_int_union));
	      m.op = SFFI_MOVE_SSE64;
	      ssecount++;
	      all_gp64 = 0;
	      break;
	    case X86_64_SSESF_CLASS:
	      m.dst_off = (unsigned) (offsetof (struct register_args, sse)
				      + ssecount * sizeof (union big_int_union));
	      m.op = SFFI_MOVE_SSE32;
	      ssecount++;
	      all_gp64 = 0;
	      break;
	    default:
	      free (plan);		
	      return NULL;
	    }
	  plan->moves[nm++] = m;
	}
    }

  plan->nmoves = nm;
  plan->ssecount = ssecount;
  plan->bytes = cif->bytes;
  plan->flags = cif->flags;
  plan->retcode = cif->flags & 0xff;	
  
  plan->fast = (cif->bytes == 0 && plan->retcode <= UNIX64_RET_XMM64) ? 1 : 0;
  
  plan->thunk_n =
    (all_gp64 && !plan->ret_in_mem && nm == avn && avn <= MAX_GPR_REGS
     && plan->fast)
    ? (int) avn : -1;
  return plan;
}


SFFI_ASAN_NO_SANITIZE
static inline __attribute__ ((always_inline)) void
plan_exec (sffi_cif *cif, sffi_plan *plan, void (*fn) (void),
	   void *rvalue, void **avalue)
{
  unsigned flags = plan->flags;
  struct register_args local __attribute__ ((aligned (16)));
  char *stack = NULL;
  struct register_args *reg_args;
  unsigned k;

  if (rvalue == NULL)
    {
      if (flags & UNIX64_FLAG_RET_IN_MEM)
	rvalue = alloca (cif->rtype->size);
      else
	flags = UNIX64_RET_VOID;
    }

  if (plan->thunk_n >= 0)
    {
      
      struct sffi_ret2 r = sffi_gp_thunks[plan->thunk_n] (avalue, fn);
      if (rvalue != NULL)
	store_ret (rvalue, plan->retcode, r);
      return;
    }

  if (plan->fast)
    reg_args = &local;			
  else
    {
      stack = alloca (sizeof (struct register_args) + plan->bytes + 4 * 8);
      reg_args = (struct register_args *) stack;
    }
  reg_args->r10 = 0;			
  if (plan->ret_in_mem)
    reg_args->gpr[0] = (UINT64) (uintptr_t) rvalue;

  for (k = 0; k < plan->nmoves; k++)
    {
      sffi_move *m = &plan->moves[k];
      char *src = (char *) avalue[m->src_idx] + m->src_off;
      char *dst = (char *) reg_args + m->dst_off;
      switch (m->op)
	{
	
	case SFFI_MOVE_SE8:   *(UINT64 *) dst = (UINT64) (SINT64) *(SINT8 *)  src; break;
	case SFFI_MOVE_SE16:  *(UINT64 *) dst = (UINT64) (SINT64) *(SINT16 *) src; break;
	case SFFI_MOVE_SE32:  *(UINT64 *) dst = (UINT64) (SINT64) *(SINT32 *) src; break;
	case SFFI_MOVE_GP64:  *(UINT64 *) dst = *(UINT64 *) src;                   break;
	case SFFI_MOVE_GP:    *(UINT64 *) dst = 0; memcpy (dst, src, m->len);     break;
	case SFFI_MOVE_SSE64: *(UINT64 *) dst = *(UINT64 *) src;                   break;
	case SFFI_MOVE_SSE32: *(UINT32 *) dst = *(UINT32 *) src;                   break;
	case SFFI_MOVE_STACK: memcpy (dst, src, m->len);                          break;
	}
    }
  reg_args->rax = plan->ssecount;

  if (plan->fast)
    {
      
      struct sffi_ret2 r = sffi_plan_fast_call (reg_args, fn);
      if (rvalue != NULL)
	store_ret (rvalue, plan->retcode, r);
      return;
    }

  sffi_call_unix64 (stack, plan->bytes + sizeof (struct register_args),
		   flags, rvalue, fn);
}


struct sffi_call_plan
{
  sffi_cif  *cif;
  sffi_plan *fast;		
};

sffi_call_plan *
sffi_call_plan_alloc (sffi_cif *cif)
{
  sffi_call_plan *plan = malloc (sizeof (struct sffi_call_plan));
  if (plan == NULL)
    return NULL;
  plan->cif  = cif;
  plan->fast = build_plan (cif);	
  return plan;
}

void
sffi_call_plan_invoke (sffi_call_plan *plan, void (*fn) (void),
		      void *rvalue, void **avalue)
{
  if (plan->fast != NULL)
    plan_exec (plan->cif, plan->fast, fn, rvalue, avalue);
  else
    sffi_call (plan->cif, fn, rvalue, avalue);
}

void
sffi_call_plan_free (sffi_call_plan *plan)
{
  if (plan != NULL)
    {
      free (plan->fast);
      free (plan);
    }
}

extern void
sffi_call_efi64(sffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue);
#endif

void
sffi_call (sffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  sffi_type **arg_types = cif->arg_types;
  void **avalue_copy = NULL;
  int i, nargs = cif->nargs;
  const int max_reg_struct_size = cif->abi == SFFI_GNUW64 ? 8 : 16;

  
  for (i = 0; i < nargs; i++)
    {
      sffi_type *at = arg_types[i];
      int size = at->size;
      if (at->type == SFFI_TYPE_STRUCT && size > max_reg_struct_size)
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

#ifndef __ILP32__
  if (cif->abi == SFFI_EFI64 || cif->abi == SFFI_GNUW64)
    {
      sffi_call_efi64(cif, fn, rvalue, avalue);
      return;
    }
#endif
  sffi_call_int (cif, fn, rvalue, avalue, NULL);
}

#ifdef SFFI_GO_CLOSURES

#ifndef __ILP32__
extern void
sffi_call_go_efi64(sffi_cif *cif, void (*fn)(void), void *rvalue,
		  void **avalue, void *closure);
#endif

void
sffi_call_go (sffi_cif *cif, void (*fn)(void), void *rvalue,
	     void **avalue, void *closure)
{
#ifndef __ILP32__
  if (cif->abi == SFFI_EFI64 || cif->abi == SFFI_GNUW64)
    {
      sffi_call_go_efi64(cif, fn, rvalue, avalue, closure);
      return;
    }
#endif
  sffi_call_int (cif, fn, rvalue, avalue, closure);
}

#endif 

extern void sffi_closure_unix64(void) SFFI_HIDDEN;
extern void sffi_closure_unix64_sse(void) SFFI_HIDDEN;
#if defined(SFFI_EXEC_STATIC_TRAMP)
extern void sffi_closure_unix64_alt(void) SFFI_HIDDEN;
extern void sffi_closure_unix64_sse_alt(void) SFFI_HIDDEN;
#endif

#ifndef __ILP32__
extern sffi_status
sffi_prep_closure_loc_efi64(sffi_closure* closure,
			   sffi_cif* cif,
			   void (*fun)(sffi_cif*, void*, void**, void*),
			   void *user_data,
			   void *codeloc);
#endif

sffi_status
sffi_prep_closure_loc (sffi_closure* closure,
		      sffi_cif* cif,
		      void (*fun)(sffi_cif*, void*, void**, void*),
		      void *user_data,
		      void *codeloc)
{
  static const unsigned char trampoline[24] = {
    
    0xf3, 0x0f, 0x1e, 0xfa,
    
    0x4c, 0x8d, 0x15, 0xf5, 0xff, 0xff, 0xff,
    
    0xff, 0x25, 0x07, 0x00, 0x00, 0x00,
    
    0x0f, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00
  };
  void (*dest)(void);
  char *tramp = closure->tramp;

#ifndef __ILP32__
  if (cif->abi == SFFI_EFI64 || cif->abi == SFFI_GNUW64)
    return sffi_prep_closure_loc_efi64(closure, cif, fun, user_data, codeloc);
#endif
  if (cif->abi != SFFI_UNIX64)
    return SFFI_BAD_ABI;

  if (cif->flags & UNIX64_FLAG_XMM_ARGS)
    dest = sffi_closure_unix64_sse;
  else
    dest = sffi_closure_unix64;

#if defined(SFFI_EXEC_STATIC_TRAMP)
  if (sffi_tramp_is_present(closure))
    {
      
      if (dest == sffi_closure_unix64_sse)
        dest = sffi_closure_unix64_sse_alt;
      else
        dest = sffi_closure_unix64_alt;
      sffi_tramp_set_parms (closure->ftramp, dest, closure);
      goto out;
    }
#endif

  
  memcpy (tramp, trampoline, sizeof(trampoline));
  *(UINT64 *)(tramp + sizeof (trampoline)) = (uintptr_t)dest;

#if defined(SFFI_EXEC_STATIC_TRAMP)
out:
#endif
  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  return SFFI_OK;
}

int SFFI_HIDDEN
sffi_closure_unix64_inner(sffi_cif *cif,
			 void (*fun)(sffi_cif*, void*, void**, void*),
			 void *user_data,
			 void *rvalue,
			 struct register_args *reg_args,
			 char *argp)
{
  void **avalue;
  sffi_type **arg_types;
  long i, avn;
  int gprcount, ssecount, ngpr, nsse;
  int flags;

  avn = cif->nargs;
  flags = cif->flags;

  avalue = alloca(avn * sizeof(void *));
  gprcount = ssecount = 0;

  if (flags & UNIX64_FLAG_RET_IN_MEM)
    {
      
      void *r = (void *)(uintptr_t)reg_args->gpr[gprcount++];
      *(void **)rvalue = r;
      rvalue = r;
      flags = (sizeof(void *) == 4 ? UNIX64_RET_UINT32 : UNIX64_RET_INT64);
    }

  arg_types = cif->arg_types;
  for (i = 0; i < avn; ++i)
    {
      enum x86_64_reg_class classes[MAX_CLASSES];
      size_t n;

      n = examine_argument (arg_types[i], classes, 0, &ngpr, &nsse);
      if (n == 0
	  || gprcount + ngpr > MAX_GPR_REGS
	  || ssecount + nsse > MAX_SSE_REGS)
	{
	  long align = arg_types[i]->alignment;

	  
	  if (align < 8)
	    align = 8;

	  
	  argp = (void *) SFFI_ALIGN (argp, align);
	  avalue[i] = argp;
	  argp += arg_types[i]->size;
	}
      
      else if (n == 1
	       || (n == 2 && !(SSE_CLASS_P (classes[0])
			       || SSE_CLASS_P (classes[1]))))
	{
	  
	  if (SSE_CLASS_P (classes[0]))
	    {
	      avalue[i] = &reg_args->sse[ssecount];
	      ssecount += n;
	    }
	  else
	    {
	      avalue[i] = &reg_args->gpr[gprcount];
	      gprcount += n;
	    }
	}
      
      else
	{
	  char *a = alloca (n * 8);
	  unsigned int j;

	  avalue[i] = a;
	  for (j = 0; j < n; j++, a += 8)
	    {
	      if (classes[j] == X86_64_SSEUP_CLASS)
		
		memcpy (a, (char *) &reg_args->sse[ssecount - 1] + 8, 8);
	      else if (SSE_CLASS_P (classes[j]))
		memcpy (a, &reg_args->sse[ssecount++], 8);
	      else
		memcpy (a, &reg_args->gpr[gprcount++], 8);
	    }
	}
    }

  
  fun (cif, rvalue, avalue, user_data);

  
  return flags;
}

#ifdef SFFI_GO_CLOSURES

extern void sffi_go_closure_unix64(void) SFFI_HIDDEN;
extern void sffi_go_closure_unix64_sse(void) SFFI_HIDDEN;

#ifndef __ILP32__
extern sffi_status
sffi_prep_go_closure_efi64(sffi_go_closure* closure, sffi_cif* cif,
			  void (*fun)(sffi_cif*, void*, void**, void*));
#endif

sffi_status
sffi_prep_go_closure (sffi_go_closure* closure, sffi_cif* cif,
		     void (*fun)(sffi_cif*, void*, void**, void*))
{
#ifndef __ILP32__
  if (cif->abi == SFFI_EFI64 || cif->abi == SFFI_GNUW64)
    return sffi_prep_go_closure_efi64(closure, cif, fun);
#endif
  if (cif->abi != SFFI_UNIX64)
    return SFFI_BAD_ABI;

  closure->tramp = (cif->flags & UNIX64_FLAG_XMM_ARGS
		    ? sffi_go_closure_unix64_sse
		    : sffi_go_closure_unix64);
  closure->cif = cif;
  closure->fun = fun;

  return SFFI_OK;
}

#endif 

#if defined(SFFI_EXEC_STATIC_TRAMP)
void *
sffi_tramp_arch (size_t *tramp_size, size_t *map_size)
{
  extern void *trampoline_code_table;

  *map_size = UNIX64_TRAMP_MAP_SIZE;
  *tramp_size = UNIX64_TRAMP_SIZE;
  return &trampoline_code_table;
}
#endif

#endif 
