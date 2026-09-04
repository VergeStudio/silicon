#if defined(__kvx__)
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sfficonfig.h>
#include <sffi.h>
#include "sffi_common.h"
#include "asm.h"

#define ALIGN(x, a) ALIGN_MASK(x, (typeof(x))(a) - 1)
#define ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define KVX_ABI_STACK_ALIGNMENT (32)
#define KVX_ABI_STACK_ARG_ALIGNMENT (8)
#define max(a,b) ((a) > (b) ? (a) : (b))

#ifdef SFFI_DEBUG
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while(0)
#else
#define DEBUG_PRINT(...)
#endif

struct ret_value {
	unsigned long int r0;
	unsigned long int r1;
	unsigned long int r2;
	unsigned long int r3;
};

extern struct ret_value sffi_call_SYSV(unsigned total_size,
                                      unsigned size,
                                      extended_cif *ecif,
                                      unsigned *rvalue_addr,
                                      void *fn,
                                      unsigned int_ext_method);

sffi_status sffi_prep_cif_machdep(sffi_cif *cif)
{
  cif->flags = cif->rtype->size;
  return SFFI_OK;
}

void *sffi_prep_args(char *stack, unsigned int arg_slots_size, extended_cif *ecif)
{
  char *stacktemp = stack;
  char *current_arg_passed_by_value = stack + arg_slots_size;
  int i, s;
  sffi_type **arg;
  int count = 0;
  sffi_cif *cif = ecif->cif;
  void **argv = ecif->avalue;

  arg = cif->arg_types;

  DEBUG_PRINT("stack: %p\n", stack);
  DEBUG_PRINT("arg_slots_size: %u\n", arg_slots_size);
  DEBUG_PRINT("current_arg_passed_by_value: %p\n", current_arg_passed_by_value);
  DEBUG_PRINT("ecif: %p\n", ecif);
  DEBUG_PRINT("ecif->avalue: %p\n", ecif->avalue);

  for (i = 0; i < cif->nargs; i++) {

    s = KVX_ABI_SLOT_SIZE;
    switch((*arg)->type) {
      case SFFI_TYPE_SINT8:
      case SFFI_TYPE_UINT8:
      case SFFI_TYPE_SINT16:
      case SFFI_TYPE_UINT16:
      case SFFI_TYPE_SINT32:
      case SFFI_TYPE_UINT32:
      case SFFI_TYPE_FLOAT:
      case SFFI_TYPE_DOUBLE:
      case SFFI_TYPE_UINT64:
      case SFFI_TYPE_SINT64:
      case SFFI_TYPE_POINTER:
        DEBUG_PRINT("INT64/32/16/8/FLOAT/DOUBLE or POINTER @%p\n", stack);
        *(uint64_t *) stack = *(uint64_t *)(* argv);
        break;

      case SFFI_TYPE_COMPLEX:
        if ((*arg)->size == 8)
          *(_Complex float *) stack = *(_Complex float *)(* argv);
        else if ((*arg)->size == 16) {
          *(_Complex double *) stack = *(_Complex double *)(* argv);
          s = 16;
        } else
          abort();
        break;
      case SFFI_TYPE_STRUCT: {
        char *value;
        unsigned int written_size = 0;
        DEBUG_PRINT("struct by value @%p\n", stack);
        if ((*arg)->size > KVX_ABI_MAX_AGGREGATE_IN_REG_SIZE) {
          DEBUG_PRINT("big struct\n");
          *(uint64_t *) stack = (uintptr_t)current_arg_passed_by_value;
          value = current_arg_passed_by_value;
          current_arg_passed_by_value += (*arg)->size;
          written_size = KVX_ABI_SLOT_SIZE;
        } else {
          value = stack;
          written_size = (*arg)->size;
        }
        memcpy(value, *argv, (*arg)->size);
        s = ALIGN(written_size, KVX_ABI_STACK_ARG_ALIGNMENT);
        break;
      }
      default:
        printf("Error: unsupported arg type %d\n", (*arg)->type);
        abort();
        break;

    }
    stack += s;
    count += s;
    argv++;
    arg++;
  }
#ifdef SFFI_DEBUG
  SFFI_ASSERT(((intptr_t)(stacktemp + REG_ARGS_SIZE) & (KVX_ABI_STACK_ALIGNMENT-1)) == 0);
#endif
  return stacktemp + REG_ARGS_SIZE;
}

sffi_status sffi_prep_cif_machdep_var(sffi_cif *cif, unsigned int nfixedargs,
                                    unsigned int ntotalargs)
{
  cif->flags = cif->rtype->size;
  return SFFI_OK;
}

static unsigned long handle_small_int_ext(kvx_intext_method *int_ext_method,
                                          const sffi_type *rtype)
{
  switch (rtype->type) {
    case SFFI_TYPE_SINT8:
      *int_ext_method = KVX_RET_SXBD;
      return KVX_REGISTER_SIZE;

    case SFFI_TYPE_SINT16:
      *int_ext_method = KVX_RET_SXHD;
      return KVX_REGISTER_SIZE;

    case SFFI_TYPE_SINT32:
      *int_ext_method = KVX_RET_SXWD;
      return KVX_REGISTER_SIZE;

    case SFFI_TYPE_UINT8:
      *int_ext_method = KVX_RET_ZXBD;
      return KVX_REGISTER_SIZE;

    case SFFI_TYPE_UINT16:
      *int_ext_method = KVX_RET_ZXHD;
      return KVX_REGISTER_SIZE;

    case SFFI_TYPE_UINT32:
      *int_ext_method = KVX_RET_ZXWD;
      return KVX_REGISTER_SIZE;

    default:
      *int_ext_method = KVX_RET_NONE;
      return rtype->size;
  }
}

void sffi_call(sffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  int i;
  unsigned long int slot_fitting_args_size = 0;
  unsigned long int total_size = 0;
  unsigned long int big_struct_size = 0;
  kvx_intext_method int_extension_method;
  sffi_type **arg;
  struct ret_value local_rvalue = {0};
  size_t wb_size;

  for (i = 0, arg = cif->arg_types; i < cif->nargs; i++, arg++) {
    DEBUG_PRINT("argument %d, type %d, size %lu\n", i, (*arg)->type, (*arg)->size);
    if (((*arg)->type == SFFI_TYPE_STRUCT) || ((*arg)->type == SFFI_TYPE_COMPLEX)) {
      if ((*arg)->size <= KVX_ABI_MAX_AGGREGATE_IN_REG_SIZE) {
        slot_fitting_args_size += ALIGN((*arg)->size, KVX_ABI_SLOT_SIZE);
      } else {
        slot_fitting_args_size += KVX_ABI_SLOT_SIZE; 
        big_struct_size += ALIGN((*arg)->size, KVX_ABI_SLOT_SIZE);
      }
    } else if ((*arg)->size <= KVX_ABI_SLOT_SIZE) {
      slot_fitting_args_size += KVX_ABI_SLOT_SIZE;
    } else {
      printf("Error: unsupported arg size %ld arg type %d\n", (*arg)->size, (*arg)->type);
      abort(); 
    }
  }

  extended_cif ecif;
  ecif.cif = cif;
  ecif.avalue = avalue;
  ecif.rvalue = rvalue;

  slot_fitting_args_size = max(slot_fitting_args_size, REG_ARGS_SIZE);
  total_size = slot_fitting_args_size + big_struct_size;
  total_size = ALIGN(total_size, KVX_ABI_STACK_ALIGNMENT);

  wb_size = handle_small_int_ext(&int_extension_method, cif->rtype);

  switch (cif->abi) {
    case SFFI_SYSV:
      DEBUG_PRINT("total_size: %lu\n", total_size);
      DEBUG_PRINT("slot fitting args size: %lu\n", slot_fitting_args_size);
      DEBUG_PRINT("rvalue: %p\n", rvalue);
      DEBUG_PRINT("fn: %p\n", fn);
      DEBUG_PRINT("rsize: %u\n", cif->flags);
      DEBUG_PRINT("wb_size: %u\n", wb_size);
      DEBUG_PRINT("int_extension_method: %u\n", int_extension_method);
      local_rvalue = sffi_call_SYSV(total_size, slot_fitting_args_size,
                                   &ecif, rvalue, fn, int_extension_method);
      if ((cif->flags <= KVX_ABI_MAX_AGGREGATE_IN_REG_SIZE)
          && (cif->rtype->type != SFFI_TYPE_VOID))
        memcpy(rvalue, &local_rvalue, wb_size);
      break;
    default:
      abort();
      break;
  }
}

sffi_status
sffi_prep_closure_loc (sffi_closure* closure,
                      sffi_cif* cif,
                      void (*fun)(sffi_cif*,void*,void**,void*),
                      void *user_data,
                      void *codeloc)
{
  return SFFI_BAD_ABI;
}

#endif 
