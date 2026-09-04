#include <sffi.h>
#include <sffi_common.h>

#include <stdlib.h>

void sffi_prep_args(char *stack, extended_cif *ecif)
{
 register unsigned int i;
 register void **p_argv;
 register char *argp;
 register sffi_type **p_arg;

 argp = stack;

 if ( ecif->cif->flags == SFFI_TYPE_STRUCT ) {
  *(void **) argp = ecif->rvalue;
  argp += 4;
 }

 p_argv = ecif->avalue;

 for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types;
   (i != 0);
   i--, p_arg++)
 {
  size_t z;
  size_t alignment;

  alignment = (*p_arg)->alignment;
#ifdef __CSKYABIV1__

  if (((*p_arg)->type == SFFI_TYPE_STRUCT) && ((*p_arg)->size > 8) && (alignment == 8)) {
   alignment = 4;
  }
#endif

  if ((alignment - 1) & (unsigned) argp) {
   argp = (char *) SFFI_ALIGN(argp, alignment);
  }

  if ((*p_arg)->type == SFFI_TYPE_STRUCT)
   argp = (char *) SFFI_ALIGN(argp, 4);

  z = (*p_arg)->size;
  if (z < sizeof(int))
  {
   z = sizeof(int);
   switch ((*p_arg)->type)
   {
   case SFFI_TYPE_SINT8:
    *(signed int *) argp = (signed int)*(SINT8 *)(* p_argv);
    break;

   case SFFI_TYPE_UINT8:
    *(unsigned int *) argp = (unsigned int)*(UINT8 *)(* p_argv);
    break;

   case SFFI_TYPE_SINT16:
    *(signed int *) argp = (signed int)*(SINT16 *)(* p_argv);
    break;

   case SFFI_TYPE_UINT16:
    *(unsigned int *) argp = (unsigned int)*(UINT16 *)(* p_argv);
    break;

   case SFFI_TYPE_STRUCT:
#ifdef __CSKYBE__
    memcpy((argp + 4 - (*p_arg)->size), *p_argv, (*p_arg)->size);
#else
    memcpy(argp, *p_argv, (*p_arg)->size);
#endif
    break;

   default:
    SFFI_ASSERT(0);
   }
  }
  else if (z == sizeof(int))
  {
   *(unsigned int *) argp = (unsigned int)*(UINT32 *)(* p_argv);
  }
  else
  {
   memcpy(argp, *p_argv, z);
  }
  p_argv++;
  argp += z;
 }

 return;
}

sffi_status sffi_prep_cif_machdep(sffi_cif *cif)
{

  cif->bytes = (cif->bytes + 7) & ~7;

  switch (cif->rtype->type)
    {

    case SFFI_TYPE_DOUBLE:
    case SFFI_TYPE_SINT64:
    case SFFI_TYPE_UINT64:
      cif->flags = (unsigned) SFFI_TYPE_SINT64;
      break;

    case SFFI_TYPE_STRUCT:
      if (cif->rtype->size <= 4)

 cif->flags = (unsigned)SFFI_TYPE_INT;
      else if (cif->rtype->size <= 8)

 cif->flags = (unsigned)SFFI_TYPE_SINT64;
      else

 cif->flags = (unsigned)SFFI_TYPE_STRUCT;
      break;

    default:
      cif->flags = SFFI_TYPE_INT;
      break;
    }

  return SFFI_OK;
}

sffi_status sffi_prep_cif_machdep_var(sffi_cif *cif,
        unsigned int nfixedargs,
        unsigned int ntotalargs)
{
  return sffi_prep_cif_machdep(cif);
}

extern void sffi_call_SYSV (void (*fn)(void), extended_cif *, unsigned, unsigned, unsigned *);

void sffi_call(sffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  extended_cif ecif;

  int small_struct = (cif->flags == SFFI_TYPE_INT
        && cif->rtype->type == SFFI_TYPE_STRUCT);

  ecif.cif = cif;
  ecif.avalue = avalue;

  unsigned int temp;

  if ((rvalue == NULL) &&
      (cif->flags == SFFI_TYPE_STRUCT))
    {
      ecif.rvalue = alloca(cif->rtype->size);
    }
  else if (small_struct)
    ecif.rvalue = &temp;
  else
    ecif.rvalue = rvalue;

  switch (cif->abi)
    {
    case SFFI_SYSV:
      sffi_call_SYSV (fn, &ecif, cif->bytes, cif->flags, ecif.rvalue);
      break;

    default:
      SFFI_ASSERT(0);
      break;
    }
  if (small_struct)
#ifdef __CSKYBE__
    memcpy (rvalue, ((unsigned char *)&temp + (4 - cif->rtype->size)), cif->rtype->size);
#else
    memcpy (rvalue, &temp, cif->rtype->size);
#endif
}

static void sffi_prep_incoming_args_SYSV (char *stack, void **ret,
      void** args, sffi_cif* cif);

void sffi_closure_SYSV (sffi_closure *);

unsigned int
sffi_closure_SYSV_inner (closure, respp, args)
     sffi_closure *closure;
     void **respp;
     void *args;
{

  sffi_cif       *cif;
  void         **arg_area;

  cif         = closure->cif;
  arg_area    = (void**) alloca (cif->nargs * sizeof (void*));

  sffi_prep_incoming_args_SYSV(args, respp, arg_area, cif);

  (closure->fun) (cif, *respp, arg_area, closure->user_data);

#ifdef __CSKYBE__
  if (cif->flags == SFFI_TYPE_INT && cif->rtype->type == SFFI_TYPE_STRUCT) {
      unsigned int tmp = 0;
      tmp = *(unsigned int *)(*respp);
      *(unsigned int *)(*respp) = (tmp >> ((4 - cif->rtype->size) * 8));
  }
#endif

  return cif->flags;
}

static void
sffi_prep_incoming_args_SYSV(char *stack, void **rvalue,
       void **avalue, sffi_cif *cif)
{
  register unsigned int i;
  register void **p_argv;
  register char *argp;
  register sffi_type **p_arg;

  argp = stack;

  if ( cif->flags == SFFI_TYPE_STRUCT ) {
    *rvalue = *(void **) argp;
    argp += 4;
  }

  p_argv = avalue;

  for (i = cif->nargs, p_arg = cif->arg_types; (i != 0); i--, p_arg++)
    {
      size_t z;
      size_t alignment;

      alignment = (*p_arg)->alignment;
      if (alignment < 4)
 alignment = 4;

#ifdef __CSKYABIV1__

      if (((*p_arg)->type == SFFI_TYPE_STRUCT) && ((*p_arg)->size > 8) && (alignment == 8)) {
        alignment = 4;
      }
#endif

      if ((alignment - 1) & (unsigned) argp) {
 argp = (char *) SFFI_ALIGN(argp, alignment);
      }

      z = (*p_arg)->size;

#ifdef __CSKYBE__
      unsigned int tmp = 0;
      if ((*p_arg)->size < 4) {
        tmp = *(unsigned int *)argp;
        memcpy(argp, ((unsigned char *)&tmp + (4 - (*p_arg)->size)), (*p_arg)->size);
      }
#else

#endif
      *p_argv = (void*) argp;

      p_argv++;
      argp += z;
    }

  return;
}

extern unsigned char sffi_csky_trampoline[TRAMPOLINE_SIZE];

#define CACHEFLUSH_IN_FFI 1
#if CACHEFLUSH_IN_FFI
extern void sffi_csky_cacheflush(unsigned char *__tramp, unsigned int k,
  int i);
#define SFFI_INIT_TRAMPOLINE(TRAMP,FUN,CTX)                              \
({ unsigned char *__tramp = (unsigned char*)(TRAMP);                    \
   unsigned int  __fun = (unsigned int)(FUN);                           \
   unsigned int  __ctx = (unsigned int)(CTX);                           \
   unsigned char *insns = (unsigned char *)(CTX);                       \
   memcpy (__tramp, sffi_csky_trampoline, TRAMPOLINE_SIZE);              \
   *(unsigned int*) &__tramp[TRAMPOLINE_SIZE] = __ctx;                  \
   *(unsigned int*) &__tramp[TRAMPOLINE_SIZE + 4] = __fun;              \
   sffi_csky_cacheflush(&__tramp[0], TRAMPOLINE_SIZE, 3);  \
   sffi_csky_cacheflush(insns, TRAMPOLINE_SIZE, 3);                       \
                                                         \
 })
#else
#define SFFI_INIT_TRAMPOLINE(TRAMP,FUN,CTX)                              \
({ unsigned char *__tramp = (unsigned char*)(TRAMP);                    \
   unsigned int  __fun = (unsigned int)(FUN);                           \
   unsigned int  __ctx = (unsigned int)(CTX);                           \
   unsigned char *insns = (unsigned char *)(CTX);                       \
   memcpy (__tramp, sffi_csky_trampoline, TRAMPOLINE_SIZE);              \
   *(unsigned int*) &__tramp[TRAMPOLINE_SIZE] = __ctx;                  \
   *(unsigned int*) &__tramp[TRAMPOLINE_SIZE + 4] = __fun;              \
   __clear_cache((&__tramp[0]), (&__tramp[TRAMPOLINE_SIZE-1]));  \
   __clear_cache(insns, insns + TRAMPOLINE_SIZE);                       \
                                                         \
 })
#endif

sffi_status
sffi_prep_closure_loc (sffi_closure* closure,
        sffi_cif* cif,
        void (*fun)(sffi_cif*,void*,void**,void*),
        void *user_data,
        void *codeloc)
{
  void (*closure_func)(sffi_closure*) = NULL;

  if (cif->abi == SFFI_SYSV)
    closure_func = &sffi_closure_SYSV;
  else
    return SFFI_BAD_ABI;

  SFFI_INIT_TRAMPOLINE (&closure->tramp[0], \
         closure_func,  \
         codeloc);

  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;

  return SFFI_OK;
}
