#include <sffi.h>
#include <sffi_common.h>
#include <stdint.h>
#include "internal.h"
#include <tramp.h>

#define MAX_GPRARGS 5

#ifdef __s390x__
#define MAX_FPRARGS 4
#else
#define MAX_FPRARGS 2
#endif

#define ROUND_SIZE(size) (((size) + 15) & ~15)

struct call_frame
{
  void *back_chain;
  void *eos;
  unsigned long gpr_args[5];
  unsigned long gpr_save[9];
  unsigned long long fpr_args[4];
};

extern void SFFI_HIDDEN sffi_call_SYSV(struct call_frame *, unsigned, void *,
			             void (*fn)(void), void *);

extern void sffi_closure_SYSV(void);
extern void sffi_go_closure_SYSV(void);

static int
sffi_check_struct_type (sffi_type *arg)
{
  size_t size = arg->size;

  while (arg->type == SFFI_TYPE_STRUCT
         && arg->elements[0] && !arg->elements[1])
    arg = arg->elements[0];

  switch (size)
    {
      case 1:
        return SFFI_TYPE_UINT8;

      case 2:
        return SFFI_TYPE_UINT16;

      case 4:
	if (arg->type == SFFI_TYPE_FLOAT)
          return SFFI_TYPE_FLOAT;
	else
	  return SFFI_TYPE_UINT32;

      case 8:
	if (arg->type == SFFI_TYPE_DOUBLE)
          return SFFI_TYPE_DOUBLE;
	else
	  return SFFI_TYPE_UINT64;

      default:
	break;
    }

  return SFFI_TYPE_POINTER;
}

sffi_status SFFI_HIDDEN
sffi_prep_cif_machdep(sffi_cif *cif)
{
  size_t struct_size = 0;
  int n_gpr = 0;
  int n_fpr = 0;
  int n_ov = 0;

  sffi_type **ptr;
  int i;

  switch (cif->rtype->type)
    {

      case SFFI_TYPE_VOID:
	cif->flags = FFI390_RET_VOID;
	break;

      case SFFI_TYPE_STRUCT:
      case SFFI_TYPE_COMPLEX:
      case SFFI_TYPE_SINT128:
      case SFFI_TYPE_UINT128:
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
      case SFFI_TYPE_LONGDOUBLE:
#endif
	cif->flags = FFI390_RET_STRUCT;
	n_gpr++;  
	break;

      case SFFI_TYPE_FLOAT:
	cif->flags = FFI390_RET_FLOAT;
	break;

      case SFFI_TYPE_DOUBLE:
	cif->flags = FFI390_RET_DOUBLE;
	break;

      case SFFI_TYPE_UINT64:
      case SFFI_TYPE_SINT64:
	cif->flags = FFI390_RET_INT64;
	break;

      case SFFI_TYPE_POINTER:
      case SFFI_TYPE_INT:
      case SFFI_TYPE_UINT32:
      case SFFI_TYPE_SINT32:
      case SFFI_TYPE_UINT16:
      case SFFI_TYPE_SINT16:
      case SFFI_TYPE_UINT8:
      case SFFI_TYPE_SINT8:

#ifdef __s390x__
	cif->flags = FFI390_RET_INT64;
#else
	cif->flags = FFI390_RET_INT32;
#endif
	break;

      default:
        SFFI_ASSERT (0);
        break;
    }

  for (ptr = cif->arg_types, i = cif->nargs;
       i > 0;
       i--, ptr++)
    {
      int type = (*ptr)->type;

      switch (type)
	{
	case SFFI_TYPE_STRUCT:
	  type = sffi_check_struct_type (*ptr);
	  if (type != SFFI_TYPE_POINTER)
	    break;

	case SFFI_TYPE_COMPLEX:
	case SFFI_TYPE_SINT128:
	case SFFI_TYPE_UINT128:
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
	case SFFI_TYPE_LONGDOUBLE:
#endif
	  type = SFFI_TYPE_POINTER;

	  struct_size += ROUND_SIZE ((*ptr)->size);
	  break;
	}

      switch (type)
	{

	  case SFFI_TYPE_DOUBLE:
	    if (n_fpr < MAX_FPRARGS)
	      n_fpr++;
	    else
	      n_ov += sizeof (double) / sizeof (long);
	    break;

	  case SFFI_TYPE_FLOAT:
	    if (n_fpr < MAX_FPRARGS)
	      n_fpr++;
	    else
	      n_ov++;
	    break;

#ifndef __s390x__
	  case SFFI_TYPE_UINT64:
	  case SFFI_TYPE_SINT64:
	    if (n_gpr == MAX_GPRARGS-1)
	      n_gpr = MAX_GPRARGS;
	    if (n_gpr < MAX_GPRARGS)
	      n_gpr += 2;
	    else
	      n_ov += 2;
	    break;
#endif

	  default:
	    if (n_gpr < MAX_GPRARGS)
	      n_gpr++;
	    else
	      n_ov++;
	    break;
        }
    }

  cif->bytes = ROUND_SIZE (n_ov * sizeof (long)) + struct_size;

  return SFFI_OK;
}

static void
sffi_call_int(sffi_cif *cif,
	     void (*fn)(void),
	     void *rvalue,
	     void **avalue,
	     void *closure)
{
  int ret_type = cif->flags;
  size_t rsize = 0, bytes = cif->bytes;
  unsigned char *stack, *p_struct;
  struct call_frame *frame;
  unsigned long *p_ov, *p_gpr;
  unsigned long long *p_fpr;
  int n_fpr, n_gpr, n_ov, i, n;
  sffi_type **arg_types;

  SFFI_ASSERT (cif->abi == SFFI_SYSV);

  if (rvalue == NULL)
    {
      if (ret_type & FFI390_RET_IN_MEM)
	rsize = cif->rtype->size;
      else
	ret_type = FFI390_RET_VOID;
    }

  stack = alloca (bytes + sizeof(struct call_frame) + rsize);
  frame = (struct call_frame *)(stack + bytes);
  if (rsize)
    rvalue = frame + 1;

  frame->back_chain = __builtin_frame_address (0);

  p_ov = (unsigned long *)stack;
  p_struct = (unsigned char *)frame;
  p_gpr = frame->gpr_args;
  p_fpr = frame->fpr_args;
  n_fpr = n_gpr = n_ov = 0;

  if (cif->flags & FFI390_RET_IN_MEM)
    p_gpr[n_gpr++] = (uintptr_t) rvalue;

  arg_types = cif->arg_types;
  for (i = 0, n = cif->nargs; i < n; ++i)
    {
      sffi_type *ty = arg_types[i];
      void *arg = avalue[i];
      int type = ty->type;
      sffi_arg val;

    restart:
      switch (type)
	{
	case SFFI_TYPE_SINT8:
	  val = *(SINT8 *)arg;
	  goto do_int;
	case SFFI_TYPE_UINT8:
	  val = *(UINT8 *)arg;
	  goto do_int;
	case SFFI_TYPE_SINT16:
	  val = *(SINT16 *)arg;
	  goto do_int;
	case SFFI_TYPE_UINT16:
	  val = *(UINT16 *)arg;
	  goto do_int;
	case SFFI_TYPE_INT:
	case SFFI_TYPE_SINT32:
	  val = *(SINT32 *)arg;
	  goto do_int;
	case SFFI_TYPE_UINT32:
	  val = *(UINT32 *)arg;
	  goto do_int;
	case SFFI_TYPE_POINTER:
	  val = *(uintptr_t *)arg;
	do_int:
	  *(n_gpr < MAX_GPRARGS ? p_gpr + n_gpr++ : p_ov + n_ov++) = val;
	  break;

	case SFFI_TYPE_UINT64:
	case SFFI_TYPE_SINT64:
#ifdef __s390x__
	  val = *(UINT64 *)arg;
	  goto do_int;
#else
	  if (n_gpr == MAX_GPRARGS-1)
	    n_gpr = MAX_GPRARGS;
	  if (n_gpr < MAX_GPRARGS)
	    p_gpr[n_gpr++] = ((UINT32 *) arg)[0],
	    p_gpr[n_gpr++] = ((UINT32 *) arg)[1];
	  else
	    p_ov[n_ov++] = ((UINT32 *) arg)[0],
	    p_ov[n_ov++] = ((UINT32 *) arg)[1];
#endif
	  break;

	case SFFI_TYPE_DOUBLE:
	  if (n_fpr < MAX_FPRARGS)
	    p_fpr[n_fpr++] = *(UINT64 *) arg;
	  else
	    {
#ifdef __s390x__
	      p_ov[n_ov++] = *(UINT64 *) arg;
#else
	      p_ov[n_ov++] = ((UINT32 *) arg)[0],
	      p_ov[n_ov++] = ((UINT32 *) arg)[1];
#endif
	    }
	  break;

	case SFFI_TYPE_FLOAT:
	  val = *(UINT32 *)arg;
	  if (n_fpr < MAX_FPRARGS)
	    p_fpr[n_fpr++] = (UINT64)val << 32;
	  else
	    p_ov[n_ov++] = val;
	  break;

	case SFFI_TYPE_STRUCT:

	  type = sffi_check_struct_type (ty);

	  if (type != SFFI_TYPE_POINTER)
	    goto restart;

#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
	case SFFI_TYPE_LONGDOUBLE:
#endif
	case SFFI_TYPE_COMPLEX:
	case SFFI_TYPE_SINT128:
	case SFFI_TYPE_UINT128:

	  p_struct -= ROUND_SIZE (ty->size);
	  memcpy (p_struct, arg, ty->size);
	  val = (uintptr_t)p_struct;
	  goto do_int;

	default:
	  SFFI_ASSERT (0);
	  break;
        }
    }

  sffi_call_SYSV (frame, ret_type & FFI360_RET_MASK, rvalue, fn, closure);
}

void
sffi_call (sffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  sffi_call_int(cif, fn, rvalue, avalue, NULL);
}

void
sffi_call_go (sffi_cif *cif, void (*fn)(void), void *rvalue,
	     void **avalue, void *closure)
{
  sffi_call_int(cif, fn, rvalue, avalue, closure);
}

void SFFI_HIDDEN
sffi_closure_helper_SYSV (sffi_cif *cif,
			 void (*fun)(sffi_cif*,void*,void**,void*),
			 void *user_data,
			 unsigned long *p_gpr,
			 unsigned long long *p_fpr,
			 unsigned long *p_ov)
{
  unsigned long long ret_buffer;

  void *rvalue = &ret_buffer;
  void **avalue;
  void **p_arg;

  int n_gpr = 0;
  int n_fpr = 0;
  int n_ov = 0;

  sffi_type **ptr;
  int i;

  p_arg = avalue = alloca (cif->nargs * sizeof (void *));

  if (cif->flags & FFI390_RET_IN_MEM)
    rvalue = (void *) p_gpr[n_gpr++];

  for (ptr = cif->arg_types, i = cif->nargs; i > 0; i--, p_arg++, ptr++)
    {
      int deref_struct_pointer = 0;
      int type = (*ptr)->type;

#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE

      if (type == SFFI_TYPE_LONGDOUBLE)
	type = SFFI_TYPE_STRUCT;
#endif

      if (type == SFFI_TYPE_STRUCT || type == SFFI_TYPE_COMPLEX)
	{
	  if (type == SFFI_TYPE_COMPLEX)
	    type = SFFI_TYPE_POINTER;
	  else
	    type = sffi_check_struct_type (*ptr);

	  if (type == SFFI_TYPE_POINTER)
	    deref_struct_pointer = 1;
	}

      if (type == SFFI_TYPE_POINTER)
	{
#ifdef __s390x__
	  type = SFFI_TYPE_UINT64;
#else
	  type = SFFI_TYPE_UINT32;
#endif
	}

      switch (type)
	{
	  case SFFI_TYPE_DOUBLE:
	    if (n_fpr < MAX_FPRARGS)
	      *p_arg = &p_fpr[n_fpr++];
	    else
	      *p_arg = &p_ov[n_ov],
	      n_ov += sizeof (double) / sizeof (long);
	    break;

	  case SFFI_TYPE_FLOAT:
	    if (n_fpr < MAX_FPRARGS)
	      *p_arg = &p_fpr[n_fpr++];
	    else
	      *p_arg = (char *)&p_ov[n_ov++] + sizeof (long) - 4;
	    break;

	  case SFFI_TYPE_UINT64:
	  case SFFI_TYPE_SINT64:
#ifdef __s390x__
	    if (n_gpr < MAX_GPRARGS)
	      *p_arg = &p_gpr[n_gpr++];
	    else
	      *p_arg = &p_ov[n_ov++];
#else
	    if (n_gpr == MAX_GPRARGS-1)
	      n_gpr = MAX_GPRARGS;
	    if (n_gpr < MAX_GPRARGS)
	      *p_arg = &p_gpr[n_gpr], n_gpr += 2;
	    else
	      *p_arg = &p_ov[n_ov], n_ov += 2;
#endif
	    break;

	  case SFFI_TYPE_INT:
	  case SFFI_TYPE_UINT32:
	  case SFFI_TYPE_SINT32:
	    if (n_gpr < MAX_GPRARGS)
	      *p_arg = (char *)&p_gpr[n_gpr++] + sizeof (long) - 4;
	    else
	      *p_arg = (char *)&p_ov[n_ov++] + sizeof (long) - 4;
	    break;

	  case SFFI_TYPE_UINT16:
	  case SFFI_TYPE_SINT16:
	    if (n_gpr < MAX_GPRARGS)
	      *p_arg = (char *)&p_gpr[n_gpr++] + sizeof (long) - 2;
	    else
	      *p_arg = (char *)&p_ov[n_ov++] + sizeof (long) - 2;
	    break;

	  case SFFI_TYPE_UINT8:
	  case SFFI_TYPE_SINT8:
	    if (n_gpr < MAX_GPRARGS)
	      *p_arg = (char *)&p_gpr[n_gpr++] + sizeof (long) - 1;
	    else
	      *p_arg = (char *)&p_ov[n_ov++] + sizeof (long) - 1;
	    break;

	  default:
	    SFFI_ASSERT (0);
	    break;
        }

      if (deref_struct_pointer)
	*p_arg = *(void **)*p_arg;
    }

  (fun) (cif, rvalue, avalue, user_data);

  switch (cif->rtype->type)
    {

      case SFFI_TYPE_VOID:
      case SFFI_TYPE_STRUCT:
      case SFFI_TYPE_COMPLEX:
#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
      case SFFI_TYPE_LONGDOUBLE:
#endif
	break;

      case SFFI_TYPE_FLOAT:
	p_fpr[0] = (long long) *(unsigned int *) rvalue << 32;
	break;

      case SFFI_TYPE_DOUBLE:
	p_fpr[0] = *(unsigned long long *) rvalue;
	break;

      case SFFI_TYPE_UINT64:
      case SFFI_TYPE_SINT64:
#ifdef __s390x__
	p_gpr[0] = *(unsigned long *) rvalue;
#else
	p_gpr[0] = ((unsigned long *) rvalue)[0],
	p_gpr[1] = ((unsigned long *) rvalue)[1];
#endif
	break;

      case SFFI_TYPE_POINTER:
      case SFFI_TYPE_UINT32:
      case SFFI_TYPE_UINT16:
      case SFFI_TYPE_UINT8:
	p_gpr[0] = *(unsigned long *) rvalue;
	break;

      case SFFI_TYPE_INT:
      case SFFI_TYPE_SINT32:
      case SFFI_TYPE_SINT16:
      case SFFI_TYPE_SINT8:
	p_gpr[0] = *(signed long *) rvalue;
	break;

      default:
        SFFI_ASSERT (0);
        break;
    }
}

sffi_status
sffi_prep_closure_loc (sffi_closure *closure,
		      sffi_cif *cif,
		      void (*fun) (sffi_cif *, void *, void **, void *),
		      void *user_data,
		      void *codeloc)
{
  static unsigned short const template[] = {
    0x0d10,			
#ifndef __s390x__
    0x9801, 0x1006,		
#else
    0xeb01, 0x100e, 0x0004,	
#endif
    0x07f1			
  };
  void (*dest)(void);
  unsigned long *tramp = (unsigned long *)&closure->tramp;

  if (cif->abi != SFFI_SYSV)
    return SFFI_BAD_ABI;

#if defined(SFFI_EXEC_STATIC_TRAMP)
  if (sffi_tramp_is_present(closure))
    {

      dest = sffi_closure_SYSV;
      sffi_tramp_set_parms (closure->ftramp, dest, closure);
      goto out;
    }
#endif

  memcpy (tramp, template, sizeof(template));
  tramp[2] = (unsigned long)codeloc;
  tramp[3] = (unsigned long)&sffi_closure_SYSV;

#if defined(SFFI_EXEC_STATIC_TRAMP)
out:
#endif
  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  return SFFI_OK;
}

sffi_status
sffi_prep_go_closure (sffi_go_closure *closure, sffi_cif *cif,
		     void (*fun)(sffi_cif*,void*,void**,void*))
{
  if (cif->abi != SFFI_SYSV)
    return SFFI_BAD_ABI;

  closure->tramp = sffi_go_closure_SYSV;
  closure->cif = cif;
  closure->fun = fun;

  return SFFI_OK;
}

#if defined(SFFI_EXEC_STATIC_TRAMP)
void *
sffi_tramp_arch (size_t *tramp_size, size_t *map_size)
{
  extern void *trampoline_code_table;

  *tramp_size = FFI390_TRAMP_SIZE;
  *map_size = FFI390_TRAMP_MAP_SIZE;
  return &trampoline_code_table;
}
#endif
