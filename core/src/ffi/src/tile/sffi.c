

#include <sffi.h>
#include <sffi_common.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <arch/abi.h>
#include <arch/icache.h>
#include <arch/opcode.h>



#define NUM_ARG_REGS 10


extern void sffi_call_tile(sffi_sarg reg_args[NUM_ARG_REGS],
                          const sffi_sarg *stack_args,
                          size_t stack_args_bytes,
                          void (*fnaddr)(void))
  SFFI_HIDDEN;


extern void sffi_closure_tile(void) SFFI_HIDDEN;


sffi_status
sffi_prep_cif_machdep(sffi_cif *cif)
{
  
  if (cif->bytes < NUM_ARG_REGS * SFFI_SIZEOF_ARG)
    cif->bytes = NUM_ARG_REGS * SFFI_SIZEOF_ARG;

  if (cif->rtype->size > NUM_ARG_REGS * SFFI_SIZEOF_ARG)
    cif->flags = SFFI_TYPE_STRUCT;
  else
    cif->flags = SFFI_TYPE_INT;

  
  return SFFI_OK;
}


static long
assign_to_ffi_arg(sffi_sarg *out, void *in, const sffi_type *type,
                  int write_to_reg)
{
  switch (type->type)
    {
    case SFFI_TYPE_SINT8:
      *out = *(SINT8 *)in;
      return 1;

    case SFFI_TYPE_UINT8:
      *out = *(UINT8 *)in;
      return 1;

    case SFFI_TYPE_SINT16:
      *out = *(SINT16 *)in;
      return 1;

    case SFFI_TYPE_UINT16:
      *out = *(UINT16 *)in;
      return 1;

    case SFFI_TYPE_SINT32:
    case SFFI_TYPE_UINT32:
#ifndef __LP64__
    case SFFI_TYPE_POINTER:
#endif
      
      *out = *(SINT32 *)in;
      return 1;

    case SFFI_TYPE_FLOAT:
#ifdef __tilegx__
      if (write_to_reg)
        {
          
          union { float f; SINT32 s32; } val;
          val.f = *(float *)in;
          *out = val.s32;
        }
      else
#endif
        {
          *(float *)out = *(float *)in;
        }
      return 1;

    case SFFI_TYPE_SINT64:
    case SFFI_TYPE_UINT64:
    case SFFI_TYPE_DOUBLE:
#ifdef __LP64__
    case SFFI_TYPE_POINTER:
#endif
      *(UINT64 *)out = *(UINT64 *)in;
      return sizeof(UINT64) / SFFI_SIZEOF_ARG;

    case SFFI_TYPE_STRUCT:
      memcpy(out, in, type->size);
      return (type->size + SFFI_SIZEOF_ARG - 1) / SFFI_SIZEOF_ARG;

    case SFFI_TYPE_VOID:
      
      return 0;

    default:
      SFFI_ASSERT(0);
      return -1;
    }
}


void
sffi_call(sffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  sffi_sarg * const arg_mem = alloca(cif->bytes);
  sffi_sarg * const reg_args = arg_mem;
  sffi_sarg * const stack_args = &reg_args[NUM_ARG_REGS];
  sffi_sarg *argp = arg_mem;
  sffi_type ** const arg_types = cif->arg_types;
  const long num_args = cif->nargs;
  long i;

  if (cif->flags == SFFI_TYPE_STRUCT)
    {
      
      *argp++ = (intptr_t)(rvalue ? rvalue : alloca(cif->rtype->size));

      
      rvalue = NULL;
    }

  for (i = 0; i < num_args; i++)
    {
      sffi_type *type = arg_types[i];
      void * const arg_in = avalue[i];
      ptrdiff_t arg_word = argp - arg_mem;

#ifndef __tilegx__
      
      long align = arg_word & (type->alignment > SFFI_SIZEOF_ARG);
      argp += align;
      arg_word += align;
#endif

      if (type->type == SFFI_TYPE_STRUCT)
        {
          const size_t arg_size_in_words =
            (type->size + SFFI_SIZEOF_ARG - 1) / SFFI_SIZEOF_ARG;

          if (arg_word < NUM_ARG_REGS &&
              arg_word + arg_size_in_words > NUM_ARG_REGS)
            {
              
              argp = stack_args;
            }

          memcpy(argp, arg_in, type->size);
          argp += arg_size_in_words;
        }
      else
        {
          argp += assign_to_ffi_arg(argp, arg_in, arg_types[i], 1);
        }
    }

  
  sffi_call_tile(reg_args, stack_args,
                cif->bytes - (NUM_ARG_REGS * SFFI_SIZEOF_ARG), fn);

  if (rvalue != NULL)
    assign_to_ffi_arg(rvalue, reg_args, cif->rtype, 0);
}



extern const UINT64 sffi_template_tramp_tile[] SFFI_HIDDEN;


sffi_status
sffi_prep_closure_loc (sffi_closure *closure,
                      sffi_cif *cif,
                      void (*fun)(sffi_cif*, void*, void**, void*),
                      void *user_data,
                      void *codeloc)
{
#ifdef __tilegx__
  
  SINT64 c;
  SINT64 h;
  int s;
  UINT64 *out;

  if (cif->abi != SFFI_UNIX)
    return SFFI_BAD_ABI;

  out = (UINT64 *)closure->tramp;

  c = (intptr_t)closure;
  h = (intptr_t)sffi_closure_tile;
  s = 0;

  
  while ((c >> s) != (SINT16)(c >> s) || (h >> s) != (SINT16)(h >> s))
    s += 16;

#define OPS(a, b, shift) \
  (create_Imm16_X0((a) >> (shift)) | create_Imm16_X1((b) >> (shift)))

  
  *out++ = sffi_template_tramp_tile[0] | OPS(c, h, s);
  for (s -= 16; s >= 0; s -= 16)
    *out++ = sffi_template_tramp_tile[1] | OPS(c, h, s);

#undef OPS

  *out++ = sffi_template_tramp_tile[2];

#else
  
  UINT64 *out;
  intptr_t delta;

  if (cif->abi != SFFI_UNIX)
    return SFFI_BAD_ABI;

  out = (UINT64 *)closure->tramp;
  delta = (intptr_t)sffi_closure_tile - (intptr_t)codeloc;

  *out++ = sffi_template_tramp_tile[0] | create_JOffLong_X1(delta >> 3);
#endif

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  invalidate_icache(closure->tramp, (char *)out - closure->tramp,
                    getpagesize());

  return SFFI_OK;
}



void SFFI_HIDDEN
sffi_closure_tile_inner(sffi_closure *closure,
                       sffi_sarg reg_args[2][NUM_ARG_REGS],
                       sffi_sarg *stack_args)
{
  sffi_cif * const cif = closure->cif;
  void ** const avalue = alloca(cif->nargs * sizeof(void *));
  void *rvalue;
  sffi_type ** const arg_types = cif->arg_types;
  sffi_sarg * const reg_args_in = reg_args[0];
  sffi_sarg * const reg_args_out = reg_args[1];
  sffi_sarg * argp;
  long i, arg_word, nargs = cif->nargs;
  
  union { sffi_sarg arg[NUM_ARG_REGS]; double d; UINT64 u64; } closure_ret;

  
  argp = reg_args_in;

  
  if (cif->flags == SFFI_TYPE_STRUCT)
    {
      
      rvalue = (void *)(intptr_t)*argp++;
      arg_word = 1;
    }
  else
    {
      
      rvalue = &closure_ret;
      arg_word = 0;
    }

  
  for (i = 0; i < nargs; i++)
    {
      sffi_type * const type = arg_types[i];
      const size_t arg_size_in_words =
        (type->size + SFFI_SIZEOF_ARG - 1) / SFFI_SIZEOF_ARG;

#ifndef __tilegx__
      
      long align = arg_word & (type->alignment > SFFI_SIZEOF_ARG);
      argp += align;
      arg_word += align;
#endif

      if (arg_word == NUM_ARG_REGS ||
          (arg_word < NUM_ARG_REGS &&
           arg_word + arg_size_in_words > NUM_ARG_REGS))
        {
          
          argp = stack_args;
          arg_word = NUM_ARG_REGS;
        }

      avalue[i] = argp;
      argp += arg_size_in_words;
      arg_word += arg_size_in_words;
    }

  
  closure->fun(cif, rvalue, avalue, closure->user_data);

  if (cif->flags != SFFI_TYPE_STRUCT)
    {
      
      assign_to_ffi_arg(reg_args_out, &closure_ret, cif->rtype, 1);
    }
}
