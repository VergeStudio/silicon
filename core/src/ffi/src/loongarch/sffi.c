#include <sffi.h>
#include <sffi_common.h>
#include <tramp.h>

#include <stdlib.h>
#include <stdint.h>

#if defined(__loongarch_soft_float)
# define ABI_FRLEN 0
#elif defined(__loongarch_single_float)
# define ABI_FRLEN 32
# define ABI_FLOAT float
#elif defined(__loongarch_double_float)
# define ABI_FRLEN 64
# define ABI_FLOAT double
#else
#error unsupported LoongArch floating-point ABI
#endif

#define NARGREG 8
#define STKALIGN 16
#define MAXCOPYARG (2 * sizeof (double))

typedef struct call_context
{
#if !defined(__loongarch_soft_float)
  ABI_FLOAT fa[8];
#endif
  size_t a[10];
} call_context;

typedef struct call_builder
{
  call_context *aregs;
  int used_integer;
  int used_float;
  size_t *used_stack;	    
  size_t *stack;	    
  size_t next_struct_area;  
} call_builder;

#if __SIZEOF_POINTER__ == 8
# define IS_INT(type) ((type) >= SFFI_TYPE_UINT8 && (type) <= SFFI_TYPE_SINT64)
#else
# define IS_INT(type) ((type) >= SFFI_TYPE_UINT8 && (type) <= SFFI_TYPE_SINT32)
#endif

#if ABI_FRLEN
typedef struct float_struct_info
{
  char as_elements;
  char type1;
  char offset2;
  char type2;
} float_struct_info;

#if ABI_FRLEN >= 64
# define IS_FLOAT(type) ((type) >= SFFI_TYPE_FLOAT && (type) <= SFFI_TYPE_DOUBLE)
#else
# define IS_FLOAT(type) ((type) == SFFI_TYPE_FLOAT)
#endif

static sffi_type **
flatten_struct (sffi_type *in, sffi_type **out, sffi_type **out_end)
{
  int i;

  if (out == out_end)
    return out;
  if (in->type != SFFI_TYPE_STRUCT)
    *(out++) = in;
  else
    for (i = 0; in->elements[i]; i++)
      out = flatten_struct (in->elements[i], out, out_end);
  return out;
}

static float_struct_info
struct_passed_as_elements (call_builder *cb, sffi_type *top)
{
  float_struct_info ret = {0, 0, 0, 0};
  sffi_type *fields[3];
  int num_floats, num_ints;
  int num_fields = flatten_struct (top, fields, fields + 3) - fields;

  if (num_fields == 1)
    {
      if (IS_FLOAT (fields[0]->type))
	{
	  ret.as_elements = 1;
	  ret.type1 = fields[0]->type;
	}
    }
  else if (num_fields == 2)
    {
      num_floats = IS_FLOAT (fields[0]->type) + IS_FLOAT (fields[1]->type);
      num_ints = IS_INT (fields[0]->type) + IS_INT (fields[1]->type);
      if (num_floats == 0 || num_floats + num_ints != 2)
	return ret;
      if (cb->used_float + num_floats > NARGREG
	  || cb->used_integer + (2 - num_floats) > NARGREG)
	return ret;
      if (!IS_FLOAT (fields[0]->type) && !IS_FLOAT (fields[1]->type))
	return ret;

      ret.type1 = fields[0]->type;
      ret.type2 = fields[1]->type;
      ret.offset2 = SFFI_ALIGN (fields[0]->size, fields[1]->alignment);
      ret.as_elements = 1;
    }
  return ret;
}
#endif

static void
marshal_atom (call_builder *cb, int type, void *data)
{
  size_t value = 0;
  switch (type)
    {
    case SFFI_TYPE_UINT8:
      value = *(uint8_t *) data;
      break;
    case SFFI_TYPE_SINT8:
      value = *(int8_t *) data;
      break;
    case SFFI_TYPE_UINT16:
      value = *(uint16_t *) data;
      break;
    case SFFI_TYPE_SINT16:
      value = *(int16_t *) data;
      break;

    case SFFI_TYPE_UINT32:
      value = *(int32_t *) data;
      break;
    case SFFI_TYPE_SINT32:
      value = *(int32_t *) data;
      break;
#if __SIZEOF_POINTER__ == 8
    case SFFI_TYPE_UINT64:
      value = *(uint64_t *) data;
      break;
    case SFFI_TYPE_SINT64:
      value = *(int64_t *) data;
      break;
#endif
    case SFFI_TYPE_POINTER:
      value = *(size_t *) data;
      break;

#if ABI_FRLEN >= 32
    case SFFI_TYPE_FLOAT:
      *(float *)(cb->aregs->fa + cb->used_float++) = *(float *) data;
      return;
#endif
#if ABI_FRLEN >= 64
    case SFFI_TYPE_DOUBLE:
      (cb->aregs->fa[cb->used_float++]) = *(double *) data;
      return;
#endif
    default:
      SFFI_ASSERT (0);
      break;
    }

  if (cb->used_integer == NARGREG)
    *cb->used_stack++ = value;
  else
    cb->aregs->a[cb->used_integer++] = value;
}

static void
unmarshal_atom (call_builder *cb, int type, void *data)
{
  size_t value;
  switch (type)
    {
#if ABI_FRLEN >= 32
    case SFFI_TYPE_FLOAT:
      *(float *) data = *(float *)(cb->aregs->fa + cb->used_float++);
      return;
#endif
#if ABI_FRLEN >= 64
    case SFFI_TYPE_DOUBLE:
      *(double *) data = cb->aregs->fa[cb->used_float++];
      return;
#endif
    }

  if (cb->used_integer == NARGREG)
    value = *cb->used_stack++;
  else
    value = cb->aregs->a[cb->used_integer++];

  switch (type)
    {
    case SFFI_TYPE_UINT8:
    case SFFI_TYPE_SINT8:
    case SFFI_TYPE_UINT16:
    case SFFI_TYPE_SINT16:
    case SFFI_TYPE_UINT32:
    case SFFI_TYPE_SINT32:
#if __SIZEOF_POINTER__ == 8
    case SFFI_TYPE_UINT64:
    case SFFI_TYPE_SINT64:
#endif
    case SFFI_TYPE_POINTER:
      *(sffi_arg *)data = value;
      break;
    default:
      SFFI_ASSERT (0);
      break;
    }
}

static void *
allocate_and_copy_struct_to_stack (call_builder *cb, void *data,
				   sffi_type *type)
{
  size_t dest = cb->next_struct_area - type->size;

  dest = SFFI_ALIGN_DOWN (dest, type->alignment);
  cb->next_struct_area = dest;

  return memcpy ((char *)cb->stack + dest, data, type->size);
}

static void
marshal (call_builder *cb, sffi_type *type, int var, void *data)
{
  size_t realign[2];

#if ABI_FRLEN
  if (!var && type->type == SFFI_TYPE_STRUCT)
    {
      float_struct_info fsi = struct_passed_as_elements (cb, type);
      if (fsi.as_elements)
	{
	  marshal_atom (cb, fsi.type1, data);
	  if (fsi.offset2)
	    marshal_atom (cb, fsi.type2, ((char *) data) + fsi.offset2);
	  return;
	}
    }

  if (!var && cb->used_float < NARGREG
      && IS_FLOAT (type->type))
    {
      marshal_atom (cb, type->type, data);
      return;
    }

  double promoted;
  if (var && type->type == SFFI_TYPE_FLOAT)
    {

      promoted = *(float *) data;
      type = &sffi_type_double;
      data = &promoted;
    }
#endif

  if (type->size > 2 * __SIZEOF_POINTER__)

    {
      allocate_and_copy_struct_to_stack (cb, data, type);
      data = (char *)cb->stack + cb->next_struct_area;
      marshal_atom (cb, SFFI_TYPE_POINTER, &data);
    }
  else if (IS_INT (type->type) || type->type == SFFI_TYPE_POINTER)
    marshal_atom (cb, type->type, data);
  else
    {

      if (type->alignment > __SIZEOF_POINTER__)
	{
	  if (var)
	    cb->used_integer = SFFI_ALIGN (cb->used_integer, 2);
	  cb->used_stack
	    = (size_t *) SFFI_ALIGN (cb->used_stack, 2 * __SIZEOF_POINTER__);
	}

      memcpy (realign, data, type->size);
      if (type->size > 0)
	marshal_atom (cb, SFFI_TYPE_POINTER, realign);
      if (type->size > __SIZEOF_POINTER__)
	marshal_atom (cb, SFFI_TYPE_POINTER, realign + 1);
    }
}

static void *
unmarshal (call_builder *cb, sffi_type *type, int var, void *data)
{
  size_t realign[2];
  void *pointer;

#if ABI_FRLEN
  if (!var && type->type == SFFI_TYPE_STRUCT)
    {
      float_struct_info fsi = struct_passed_as_elements (cb, type);
      if (fsi.as_elements)
	{
	  unmarshal_atom (cb, fsi.type1, data);
	  if (fsi.offset2)
	    unmarshal_atom (cb, fsi.type2, ((char *) data) + fsi.offset2);
	  return data;
	}
    }

  if (!var && cb->used_float < NARGREG
      && IS_FLOAT (type->type))
    {
      unmarshal_atom (cb, type->type, data);
      return data;
    }

  if (var && type->type == SFFI_TYPE_FLOAT)
    {
      int m = cb->used_integer;
      void *promoted
	= m < NARGREG ? cb->aregs->a + m : cb->used_stack + m - NARGREG + 1;
      *(float *) promoted = *(double *) promoted;
    }
#endif

  if (type->size > 2 * __SIZEOF_POINTER__)
    {

      unmarshal_atom (cb, SFFI_TYPE_POINTER, (char *) &pointer);
      return pointer;
    }
  else if (IS_INT (type->type) || type->type == SFFI_TYPE_POINTER)
    {
      unmarshal_atom (cb, type->type, data);
      return data;
    }
  else
    {

      if (type->alignment > __SIZEOF_POINTER__)
	{
	  if (var)
	    cb->used_integer = SFFI_ALIGN (cb->used_integer, 2);
	  cb->used_stack
	    = (size_t *) SFFI_ALIGN (cb->used_stack, 2 * __SIZEOF_POINTER__);
	}

      if (type->size > 0)
	unmarshal_atom (cb, SFFI_TYPE_POINTER, realign);
      if (type->size > __SIZEOF_POINTER__)
	unmarshal_atom (cb, SFFI_TYPE_POINTER, realign + 1);
      memcpy (data, realign, type->size);
      return data;
    }
}

static int
passed_by_ref (call_builder *cb, sffi_type *type, int var)
{
#if ABI_FRLEN
  if (!var && type->type == SFFI_TYPE_STRUCT)
    {
      float_struct_info fsi = struct_passed_as_elements (cb, type);
      if (fsi.as_elements)
	return 0;
    }
#endif

  return type->size > 2 * __SIZEOF_POINTER__;
}

sffi_status
sffi_prep_cif_machdep (sffi_cif *cif)
{
  cif->loongarch_nfixedargs = cif->nargs;
  return SFFI_OK;
}

sffi_status
sffi_prep_cif_machdep_var (sffi_cif *cif, unsigned int nfixedargs,
			  unsigned int ntotalargs)
{
  cif->loongarch_nfixedargs = nfixedargs;
  return SFFI_OK;
}

extern void sffi_call_asm (void *stack, struct call_context *regs,
			  void (*fn) (void), void *closure) SFFI_HIDDEN;

static void
sffi_call_int (sffi_cif *cif, void (*fn) (void), void *rvalue, void **avalue,
	      void *closure)
{
  size_t arg_bytes = SFFI_ALIGN (cif->bytes, STKALIGN);

  size_t extra_bytes = cif->nargs <= 3 ? 0 :
      SFFI_ALIGN(2 * sizeof(size_t) * (cif->nargs - 3), STKALIGN);

  size_t rval_bytes = 0;
  if (rvalue == NULL && cif->rtype->size > 2 * __SIZEOF_POINTER__)
    rval_bytes = SFFI_ALIGN (cif->rtype->size, STKALIGN);
  size_t alloc_size = arg_bytes + extra_bytes + rval_bytes + sizeof (call_context);

  size_t alloc_base;

  if (_Alignof(max_align_t) >= STKALIGN)

    alloc_base = (size_t) alloca (alloc_size);
  else
    alloc_base = SFFI_ALIGN (alloca (alloc_size + STKALIGN - 1), STKALIGN);

  if (rval_bytes)
    rvalue = (void *) (alloc_base + arg_bytes);

  call_builder cb;
  cb.used_float = cb.used_integer = 0;
  cb.aregs = (call_context *) (alloc_base + arg_bytes + extra_bytes + rval_bytes);
  cb.used_stack = (size_t *)((char *) alloc_base);
  cb.stack = (size_t *)((char *) alloc_base + extra_bytes + rval_bytes);
  cb.next_struct_area = arg_bytes;

  int return_by_ref = passed_by_ref (&cb, cif->rtype, 0);
  if (return_by_ref)
    cb.aregs->a[cb.used_integer++] = (size_t)rvalue;

  int i;
  for (i = 0; i < cif->nargs; i++)
    marshal (&cb, cif->arg_types[i], i >= cif->loongarch_nfixedargs,
	     avalue[i]);

  sffi_call_asm ((void *) alloc_base, cb.aregs, fn, closure);

  cb.used_float = cb.used_integer = 0;
  if (!return_by_ref && rvalue)
    unmarshal (&cb, cif->rtype, 0, rvalue);
}

void
sffi_call (sffi_cif *cif, void (*fn) (void), void *rvalue, void **avalue)
{
  sffi_call_int (cif, fn, rvalue, avalue, NULL);
}

void
sffi_call_go (sffi_cif *cif, void (*fn) (void), void *rvalue, void **avalue,
	     void *closure)
{
  sffi_call_int (cif, fn, rvalue, avalue, closure);
}

extern void sffi_closure_asm (void) SFFI_HIDDEN;

sffi_status
sffi_prep_closure_loc (sffi_closure *closure, sffi_cif *cif,
		      void (*fun) (sffi_cif *, void *, void **, void *),
		      void *user_data, void *codeloc)
{
  uint32_t *tramp = (uint32_t *) &closure->tramp[0];
  uint64_t fn = (uint64_t) (uintptr_t) sffi_closure_asm;

  if (cif->abi <= SFFI_FIRST_ABI || cif->abi >= SFFI_LAST_ABI)
    return SFFI_BAD_ABI;

#if defined(SFFI_EXEC_STATIC_TRAMP)
  if (sffi_tramp_is_present(closure))
    {
      sffi_tramp_set_parms (closure->ftramp, sffi_closure_asm, closure);
      goto out;
    }
#endif

  tramp[0] = 0x1800000c; 
  tramp[1] = 0x28c0418d; 
  tramp[2] = 0x4c0001a0; 
  tramp[3] = 0x03400000; 
  tramp[4] = fn;
  tramp[5] = fn >> 32;

  __builtin___clear_cache (codeloc, codeloc + SFFI_TRAMPOLINE_SIZE);

#if defined(SFFI_EXEC_STATIC_TRAMP)
out:
#endif
  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  return SFFI_OK;
}

extern void sffi_go_closure_asm (void) SFFI_HIDDEN;

sffi_status
sffi_prep_go_closure (sffi_go_closure *closure, sffi_cif *cif,
		     void (*fun) (sffi_cif *, void *, void **, void *))
{
  if (cif->abi <= SFFI_FIRST_ABI || cif->abi >= SFFI_LAST_ABI)
    return SFFI_BAD_ABI;

  closure->tramp = (void *) sffi_go_closure_asm;
  closure->cif = cif;
  closure->fun = fun;
  return SFFI_OK;
}

void SFFI_HIDDEN
sffi_closure_inner (sffi_cif *cif,
		   void (*fun) (sffi_cif *, void *, void **, void *),
		   void *user_data, size_t *stack, call_context *aregs)
{
  void **avalue = alloca (cif->nargs * sizeof (void *));

  char *astorage = alloca (cif->nargs * MAXCOPYARG);
  void *rvalue;
  call_builder cb;
  int return_by_ref;
  int i;

  cb.aregs = aregs;
  cb.used_integer = cb.used_float = 0;
  cb.used_stack = stack;

  return_by_ref = passed_by_ref (&cb, cif->rtype, 0);
  if (return_by_ref)
    unmarshal (&cb, &sffi_type_pointer, 0, &rvalue);
  else
    rvalue = alloca (cif->rtype->size);

  for (i = 0; i < cif->nargs; i++)
    avalue[i]
      = unmarshal (&cb, cif->arg_types[i], i >= cif->loongarch_nfixedargs,
		   astorage + i * MAXCOPYARG);

  fun (cif, rvalue, avalue, user_data);

  if (!return_by_ref && cif->rtype->type != SFFI_TYPE_VOID)
    {
      cb.used_integer = cb.used_float = 0;
      marshal (&cb, cif->rtype, 0, rvalue);
    }
}

#if defined(SFFI_EXEC_STATIC_TRAMP)
void *
sffi_tramp_arch (size_t *tramp_size, size_t *map_size)
{
  extern void *trampoline_code_table;

  *tramp_size = 16;

  *map_size = 1 << 16;
  return &trampoline_code_table;
}
#endif
