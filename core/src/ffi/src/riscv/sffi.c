#include <sffi.h>
#include <sffi_common.h>
#include "internal.h"
#include <tramp.h>

#include <stdlib.h>
#include <stdint.h>

#if __riscv_float_abi_double
#define ABI_FLEN 64
typedef union {
    uint64_t i;
    double d;
} float_reg;
#elif __riscv_float_abi_single
#define ABI_FLEN 32
typedef union {
    float f;
} float_reg;
#endif

#define NARGREG 8
#define STKALIGN 16
#define MAXCOPYARG (2 * sizeof(double))

typedef struct call_context
{
#if ABI_FLEN
    float_reg fa[8];
#endif
    size_t a[8];

    char frame[16];
} call_context;

typedef struct call_builder
{
    call_context *aregs;
    int used_integer;
    int used_float;
    size_t *used_stack;
    void *struct_stack;
} call_builder;

#if __SIZEOF_POINTER__ == 8
#define IS_INT(type) ((type) >= SFFI_TYPE_UINT8 && (type) <= SFFI_TYPE_SINT64)
#else
#define IS_INT(type) ((type) >= SFFI_TYPE_UINT8 && (type) <= SFFI_TYPE_SINT32)
#endif

#if ABI_FLEN
typedef struct {
    char as_elements, type1, offset2, type2;
} float_struct_info;

#if ABI_FLEN >= 64
#define IS_FLOAT(type) ((type) >= SFFI_TYPE_FLOAT && (type) <= SFFI_TYPE_DOUBLE)
#else
#define IS_FLOAT(type) ((type) == SFFI_TYPE_FLOAT)
#endif

static sffi_type **flatten_struct(sffi_type *in, sffi_type **out, sffi_type **out_end) {
    int i;
    if (out == out_end) return out;
    if (in->type != SFFI_TYPE_STRUCT) {
        *(out++) = in;
    } else {
        for (i = 0; in->elements[i]; i++)
            out = flatten_struct(in->elements[i], out, out_end);
    }
    return out;
}

static float_struct_info struct_passed_as_elements(call_builder *cb, sffi_type *top) {
    float_struct_info ret = {0, 0, 0, 0};
    sffi_type *fields[3];
    int num_floats, num_ints;
    int num_fields = flatten_struct(top, fields, fields + 3) - fields;

    if (num_fields == 1) {
        if (IS_FLOAT(fields[0]->type)) {
            ret.as_elements = 1;
            ret.type1 = fields[0]->type;
        }
    } else if (num_fields == 2) {
        num_floats = IS_FLOAT(fields[0]->type) + IS_FLOAT(fields[1]->type);
        num_ints = IS_INT(fields[0]->type) + IS_INT(fields[1]->type);
        if (num_floats == 0 || num_floats + num_ints != 2)
            return ret;
        if (cb->used_float + num_floats > NARGREG || cb->used_integer + (2 - num_floats) > NARGREG)
            return ret;
        if (!IS_FLOAT(fields[0]->type) && !IS_FLOAT(fields[1]->type))
            return ret;

        ret.type1 = fields[0]->type;
        ret.type2 = fields[1]->type;
        ret.offset2 = SFFI_ALIGN(fields[0]->size, fields[1]->alignment);
        ret.as_elements = 1;
    }

    return ret;
}

#if ABI_FLEN >= 64

static void marshal_float(call_builder *cb, void *data) {
    union {
        uint32_t i;
        float f;
    } value;

    value.f = *(float *)data;
    cb->aregs->fa[cb->used_float++].i =
        UINT64_C(0xffffffff00000000) | value.i;
}

static void unmarshal_float(call_builder *cb, void *data) {
    union {
        uint32_t i;
        float f;
    } value;

    value.i = (uint32_t)cb->aregs->fa[cb->used_float++].i;
    *(float *)data = value.f;
}
#endif
#endif

static void marshal_atom(call_builder *cb, int type, void *data) {
    size_t value = 0;
    switch (type) {
        case SFFI_TYPE_UINT8: value = *(uint8_t *)data; break;
        case SFFI_TYPE_SINT8: value = *(int8_t *)data; break;
        case SFFI_TYPE_UINT16: value = *(uint16_t *)data; break;
        case SFFI_TYPE_SINT16: value = *(int16_t *)data; break;

        case SFFI_TYPE_UINT32: value = *(int32_t *)data; break;
        case SFFI_TYPE_SINT32: value = *(int32_t *)data; break;
#if __SIZEOF_POINTER__ == 8
        case SFFI_TYPE_UINT64: value = *(uint64_t *)data; break;
        case SFFI_TYPE_SINT64: value = *(int64_t *)data; break;
#endif
        case SFFI_TYPE_POINTER: value = *(size_t *)data; break;

#if ABI_FLEN >= 64
        case SFFI_TYPE_FLOAT:
            marshal_float(cb, data);
            return;
#elif ABI_FLEN >= 32
        case SFFI_TYPE_FLOAT:
            __asm__("" : "=f"(cb->aregs->fa[cb->used_float++].f) : "0"(*(float *)data));
            return;
#endif
#if ABI_FLEN >= 64
        case SFFI_TYPE_DOUBLE:
            __asm__("" : "=f"(cb->aregs->fa[cb->used_float++].d) : "0"(*(double *)data));
            return;
#endif
        default: SFFI_ASSERT(0); break;
    }

    if (cb->used_integer == NARGREG) {
        *cb->used_stack++ = value;
    } else {
        cb->aregs->a[cb->used_integer++] = value;
    }
}

static void unmarshal_atom(call_builder *cb, int type, void *data) {
    size_t value;
    switch (type) {
#if ABI_FLEN >= 64
        case SFFI_TYPE_FLOAT:
            unmarshal_float(cb, data);
            return;
#elif ABI_FLEN >= 32
        case SFFI_TYPE_FLOAT:
            __asm__("" : "=f"(*(float *)data) : "0"(cb->aregs->fa[cb->used_float++].f));
            return;
#endif
#if ABI_FLEN >= 64
        case SFFI_TYPE_DOUBLE:
            __asm__("" : "=f"(*(double *)data) : "0"(cb->aregs->fa[cb->used_float++].d));
            return;
#endif
    }

    if (cb->used_integer == NARGREG) {
        value = *cb->used_stack++;
    } else {
        value = cb->aregs->a[cb->used_integer++];
    }

    switch (type) {
        case SFFI_TYPE_UINT8: *(uint8_t *)data = value; break;
        case SFFI_TYPE_SINT8: *(uint8_t *)data = value; break;
        case SFFI_TYPE_UINT16: *(uint16_t *)data = value; break;
        case SFFI_TYPE_SINT16: *(uint16_t *)data = value; break;
        case SFFI_TYPE_UINT32: *(uint32_t *)data = value; break;
        case SFFI_TYPE_SINT32: *(uint32_t *)data = value; break;
#if __SIZEOF_POINTER__ == 8
        case SFFI_TYPE_UINT64: *(uint64_t *)data = value; break;
        case SFFI_TYPE_SINT64: *(uint64_t *)data = value; break;
#endif
        case SFFI_TYPE_POINTER: *(size_t *)data = value; break;
        default: SFFI_ASSERT(0); break;
    }
}

static void marshal(call_builder *cb, sffi_type *type, int var, void *data) {
    size_t realign[2];

#if ABI_FLEN
    if (!var && type->type == SFFI_TYPE_STRUCT) {
        float_struct_info fsi = struct_passed_as_elements(cb, type);
        if (fsi.as_elements) {
            marshal_atom(cb, fsi.type1, data);
            if (fsi.offset2)
                marshal_atom(cb, fsi.type2, ((char*)data) + fsi.offset2);
            return;
        }
    }

    if (!var && cb->used_float < NARGREG && IS_FLOAT(type->type)) {
        marshal_atom(cb, type->type, data);
        return;
    }
#endif

    if (type->size > 2 * __SIZEOF_POINTER__) {

        data = memcpy (cb->struct_stack, data, type->size);
        cb->struct_stack = (size_t *) SFFI_ALIGN ((char *) cb->struct_stack + type->size, __SIZEOF_POINTER__);
        marshal_atom(cb, SFFI_TYPE_POINTER, &data);
    } else if (IS_INT(type->type) || type->type == SFFI_TYPE_POINTER) {
        marshal_atom(cb, type->type, data);
    } else {

        if (type->alignment > __SIZEOF_POINTER__) {
            if (var)
                cb->used_integer = SFFI_ALIGN(cb->used_integer, 2);
            cb->used_stack = (size_t *)SFFI_ALIGN(cb->used_stack, 2*__SIZEOF_POINTER__);
        }

        memcpy(realign, data, type->size);
        if (type->size > 0)
            marshal_atom(cb, SFFI_TYPE_POINTER, realign);
        if (type->size > __SIZEOF_POINTER__)
            marshal_atom(cb, SFFI_TYPE_POINTER, realign + 1);
    }
}

static void *unmarshal(call_builder *cb, sffi_type *type, int var, void *data) {
    size_t realign[2];
    void *pointer;

#if ABI_FLEN
    if (!var && type->type == SFFI_TYPE_STRUCT) {
        float_struct_info fsi = struct_passed_as_elements(cb, type);
        if (fsi.as_elements) {
            unmarshal_atom(cb, fsi.type1, data);
            if (fsi.offset2)
                unmarshal_atom(cb, fsi.type2, ((char*)data) + fsi.offset2);
            return data;
        }
    }

    if (!var && cb->used_float < NARGREG && IS_FLOAT(type->type)) {
        unmarshal_atom(cb, type->type, data);
        return data;
    }
#endif

    if (type->size > 2 * __SIZEOF_POINTER__) {

        unmarshal_atom(cb, SFFI_TYPE_POINTER, (char*)&pointer);
        return pointer;
    } else if (IS_INT(type->type) || type->type == SFFI_TYPE_POINTER) {
        unmarshal_atom(cb, type->type, data);
        return data;
    } else {

        if (type->alignment > __SIZEOF_POINTER__) {
            if (var)
                cb->used_integer = SFFI_ALIGN(cb->used_integer, 2);
            cb->used_stack = (size_t *)SFFI_ALIGN(cb->used_stack, 2*__SIZEOF_POINTER__);
        }

        if (type->size > 0)
            unmarshal_atom(cb, SFFI_TYPE_POINTER, realign);
        if (type->size > __SIZEOF_POINTER__)
            unmarshal_atom(cb, SFFI_TYPE_POINTER, realign + 1);
        memcpy(data, realign, type->size);
        return data;
    }
}

static int passed_by_ref(call_builder *cb, sffi_type *type, int var) {
#if ABI_FLEN
    if (!var && type->type == SFFI_TYPE_STRUCT) {
        float_struct_info fsi = struct_passed_as_elements(cb, type);
        if (fsi.as_elements) return 0;
    }
#endif

    return type->size > 2 * __SIZEOF_POINTER__;
}

sffi_status sffi_prep_cif_machdep(sffi_cif *cif) {
    cif->riscv_nfixedargs = cif->nargs;
    return SFFI_OK;
}

sffi_status sffi_prep_cif_machdep_var(sffi_cif *cif, unsigned int nfixedargs, unsigned int ntotalargs) {
    cif->riscv_nfixedargs = nfixedargs;
    return SFFI_OK;
}

extern void sffi_call_asm (void *stack, struct call_context *regs,
			  void (*fn) (void), void *closure) SFFI_HIDDEN;

static void
sffi_call_int (sffi_cif *cif, void (*fn) (void), void *rvalue, void **avalue,
	      void *closure)
{

    size_t arg_bytes = cif->nargs <= 3 ? 0 :
        SFFI_ALIGN(2 * sizeof(size_t) * (cif->nargs - 3), STKALIGN);

    size_t struct_bytes = SFFI_ALIGN (cif->bytes, STKALIGN);
    size_t rval_bytes = 0;
    if (rvalue == NULL && cif->rtype->size > 2*__SIZEOF_POINTER__)
        rval_bytes = SFFI_ALIGN(cif->rtype->size, STKALIGN);
    size_t alloc_size = arg_bytes + rval_bytes + struct_bytes + sizeof(call_context);

    size_t alloc_base;

    if (_Alignof(max_align_t) >= STKALIGN) {

        alloc_base = (size_t)alloca(alloc_size);
    } else {
        alloc_base = SFFI_ALIGN(alloca(alloc_size + STKALIGN - 1), STKALIGN);
    }

    if (rval_bytes)
        rvalue = (void*)(alloc_base + arg_bytes);

    call_builder cb;
    cb.used_float = cb.used_integer = 0;
    cb.aregs = (call_context*)(alloc_base + arg_bytes + rval_bytes + struct_bytes);
    cb.used_stack = (void*)alloc_base;
    cb.struct_stack = (void *) (alloc_base + arg_bytes + rval_bytes);

    int return_by_ref = passed_by_ref(&cb, cif->rtype, 0);
    if (return_by_ref)
        marshal(&cb, &sffi_type_pointer, 0, &rvalue);

    int i;
    for (i = 0; i < cif->nargs; i++)
        marshal(&cb, cif->arg_types[i], i >= cif->riscv_nfixedargs, avalue[i]);

    sffi_call_asm ((void *) alloc_base, cb.aregs, fn, closure);

    cb.used_float = cb.used_integer = 0;
    if (!return_by_ref && rvalue)
      {
	if (IS_INT(cif->rtype->type)
	    && cif->rtype->size < sizeof (sffi_arg))
	  {

	    switch (cif->rtype->type)
	      {
	      case SFFI_TYPE_SINT8:
	      case SFFI_TYPE_SINT16:
	      case SFFI_TYPE_SINT32:
		unmarshal_atom (&cb, (sizeof (sffi_arg) > 4
				      ? SFFI_TYPE_SINT64 : SFFI_TYPE_SINT32),
				rvalue);
		break;
	      case SFFI_TYPE_UINT8:
	      case SFFI_TYPE_UINT16:
	      case SFFI_TYPE_UINT32:
		unmarshal_atom (&cb, (sizeof (sffi_arg) > 4
				      ? SFFI_TYPE_UINT64 : SFFI_TYPE_UINT32),
				rvalue);
		break;
	      }
	  }
	else
	  unmarshal(&cb, cif->rtype, 0, rvalue);
      }
}

void
sffi_call (sffi_cif *cif, void (*fn) (void), void *rvalue, void **avalue)
{
  sffi_call_int(cif, fn, rvalue, avalue, NULL);
}

void
sffi_call_go (sffi_cif *cif, void (*fn) (void), void *rvalue,
	     void **avalue, void *closure)
{
  sffi_call_int(cif, fn, rvalue, avalue, closure);
}

extern void sffi_closure_asm(void) SFFI_HIDDEN;

sffi_status sffi_prep_closure_loc(sffi_closure *closure, sffi_cif *cif, void (*fun)(sffi_cif*,void*,void**,void*), void *user_data, void *codeloc)
{
    if (cif->abi <= SFFI_FIRST_ABI || cif->abi >= SFFI_LAST_ABI)
        return SFFI_BAD_ABI;

#ifdef SFFI_EXEC_STATIC_TRAMP
  if (sffi_tramp_is_present (closure))
    {

      void (*dest)(void) = sffi_closure_asm;
      sffi_tramp_set_parms (closure->ftramp, dest, closure);
    }
  else
#endif
    {
      uint32_t *tramp = (uint32_t *) &closure->tramp[0];
      uint64_t fn = (uint64_t) (uintptr_t) sffi_closure_asm;

      tramp[0] = 0x00000317; 
#if __SIZEOF_POINTER__ == 8
      tramp[1] = 0x01033383; 
#else
      tramp[1] = 0x01032383; 
#endif
      tramp[2] = 0x00038067; 
      tramp[3] = 0x00000013; 
      tramp[4] = fn;
      tramp[5] = fn >> 32;
#if !defined(__FreeBSD__)
    __builtin___clear_cache(codeloc, codeloc + SFFI_TRAMPOLINE_SIZE);
#endif
    }

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
		   void *user_data,
		   size_t *stack, call_context *aregs)
{
    void **avalue = alloca(cif->nargs * sizeof(void*));

    char *astorage = alloca(cif->nargs * MAXCOPYARG);
    void *rvalue;
    call_builder cb;
    int return_by_ref;
    int i;

    cb.aregs = aregs;
    cb.used_integer = cb.used_float = 0;
    cb.used_stack = stack;

    return_by_ref = passed_by_ref(&cb, cif->rtype, 0);
    if (return_by_ref)
        unmarshal(&cb, &sffi_type_pointer, 0, &rvalue);
    else
        rvalue = alloca(cif->rtype->size);

    for (i = 0; i < cif->nargs; i++)
        avalue[i] = unmarshal(&cb, cif->arg_types[i],
            i >= cif->riscv_nfixedargs, astorage + i*MAXCOPYARG);

    fun (cif, rvalue, avalue, user_data);

    if (!return_by_ref && cif->rtype->type != SFFI_TYPE_VOID) {
        cb.used_integer = cb.used_float = 0;
        marshal(&cb, cif->rtype, 0, rvalue);
    }
}

#ifdef SFFI_EXEC_STATIC_TRAMP
void *
sffi_tramp_arch (size_t *tramp_size, size_t *map_size)
{
  extern void *trampoline_code_table;
  *tramp_size = RISCV_TRAMP_SIZE;
  *map_size = RISCV_TRAMP_MAP_SIZE;
  return &trampoline_code_table;
}
#endif
