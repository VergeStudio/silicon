

#if defined(__i386__) || defined(_M_IX86)
#include <sffi.h>
#include <sffi_common.h>
#include <stdint.h>
#include <stdlib.h>
#include <tramp.h>
#include "internal.h"


#if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
# if SFFI_TYPE_LONGDOUBLE != 4
#  error SFFI_TYPE_LONGDOUBLE out of date
# endif
#else
# undef SFFI_TYPE_LONGDOUBLE
# define SFFI_TYPE_LONGDOUBLE 4
#endif

#if defined(__GNUC__) && !defined(__declspec)
# define __declspec(x)  __attribute__((x))
#endif

#if defined(_MSC_VER) && defined(_M_IX86)

#define STACK_ALIGN(bytes) (bytes)
#else
#define STACK_ALIGN(bytes) SFFI_ALIGN (bytes, 16)
#endif


sffi_status SFFI_HIDDEN
sffi_prep_cif_machdep(sffi_cif *cif)
{
  size_t bytes = 0;
  int i, n, flags, cabi = cif->abi;

  switch (cabi)
    {
    case SFFI_SYSV:
    case SFFI_STDCALL:
    case SFFI_THISCALL:
    case SFFI_FASTCALL:
    case SFFI_MS_CDECL:
    case SFFI_PASCAL:
    case SFFI_REGISTER:
      break;
    default:
      return SFFI_BAD_ABI;
    }

  switch (cif->rtype->type)
    {
    case SFFI_TYPE_VOID:
      flags = X86_RET_VOID;
      break;
    case SFFI_TYPE_FLOAT:
      flags = X86_RET_FLOAT;
      break;
    case SFFI_TYPE_DOUBLE:
      flags = X86_RET_DOUBLE;
      break;
    case SFFI_TYPE_LONGDOUBLE:
      flags = X86_RET_LDOUBLE;
      break;
    case SFFI_TYPE_UINT8:
      flags = X86_RET_UINT8;
      break;
    case SFFI_TYPE_UINT16:
      flags = X86_RET_UINT16;
      break;
    case SFFI_TYPE_SINT8:
      flags = X86_RET_SINT8;
      break;
    case SFFI_TYPE_SINT16:
      flags = X86_RET_SINT16;
      break;
    case SFFI_TYPE_INT:
    case SFFI_TYPE_SINT32:
    case SFFI_TYPE_UINT32:
    case SFFI_TYPE_POINTER:
      flags = X86_RET_INT32;
      break;
    case SFFI_TYPE_SINT64:
    case SFFI_TYPE_UINT64:
      flags = X86_RET_INT64;
      break;
    case SFFI_TYPE_STRUCT:
      {
#if defined(X86_WIN32) || defined(X86_DARWIN)
        size_t size = cif->rtype->size;
        if (size == 1)
          flags = X86_RET_STRUCT_1B;
        else if (size == 2)
          flags = X86_RET_STRUCT_2B;
        else if (size == 4)
          flags = X86_RET_INT32;
        else if (size == 8)
          flags = X86_RET_INT64;
        else
#endif
          {
          do_struct:
            switch (cabi)
              {
              case SFFI_THISCALL:
              case SFFI_FASTCALL:
              case SFFI_STDCALL:
              case SFFI_MS_CDECL:
                flags = X86_RET_STRUCTARG;
                break;
              default:
                flags = X86_RET_STRUCTPOP;
                break;
              }
            
            bytes += SFFI_ALIGN (sizeof(void*), SFFI_SIZEOF_ARG);
          }
      }
      break;
    case SFFI_TYPE_COMPLEX:
      switch (cif->rtype->elements[0]->type)
	{
	case SFFI_TYPE_DOUBLE:
	case SFFI_TYPE_LONGDOUBLE:
	case SFFI_TYPE_SINT64:
	case SFFI_TYPE_UINT64:
	  goto do_struct;
	case SFFI_TYPE_FLOAT:
	case SFFI_TYPE_INT:
	case SFFI_TYPE_SINT32:
	case SFFI_TYPE_UINT32:
	  flags = X86_RET_INT64;
	  break;
	case SFFI_TYPE_SINT16:
	case SFFI_TYPE_UINT16:
	  flags = X86_RET_INT32;
	  break;
	case SFFI_TYPE_SINT8:
	case SFFI_TYPE_UINT8:
	  flags = X86_RET_STRUCT_2B;
	  break;
	default:
	  return SFFI_BAD_TYPEDEF;
	}
      break;
    default:
      return SFFI_BAD_TYPEDEF;
    }
  cif->flags = flags;

  for (i = 0, n = cif->nargs; i < n; i++)
    {
      sffi_type *t = cif->arg_types[i];

#if defined(X86_WIN32)
      if (cabi == SFFI_STDCALL)
        bytes = SFFI_ALIGN (bytes, SFFI_SIZEOF_ARG);
      else
#endif
        bytes = SFFI_ALIGN (bytes, t->alignment);
      bytes += SFFI_ALIGN (t->size, SFFI_SIZEOF_ARG);
    }
  cif->bytes = bytes;

  return SFFI_OK;
}

static sffi_arg
extend_basic_type(void *arg, int type)
{
  switch (type)
    {
    case SFFI_TYPE_SINT8:
      return *(SINT8 *)arg;
    case SFFI_TYPE_UINT8:
      return *(UINT8 *)arg;
    case SFFI_TYPE_SINT16:
      return *(SINT16 *)arg;
    case SFFI_TYPE_UINT16:
      return *(UINT16 *)arg;

    case SFFI_TYPE_SINT32:
    case SFFI_TYPE_UINT32:
    case SFFI_TYPE_POINTER:
    case SFFI_TYPE_FLOAT:
      return *(UINT32 *)arg;

    default:
      abort();
    }
}

struct call_frame
{
  void *ebp;		
  void *retaddr;	
  void (*fn)(void);	
  int flags;		
  void *rvalue;		
  unsigned regs[3];	
};

struct abi_params
{
  int dir;		
  int static_chain;	
  int nregs;		
  int regs[3];
};

static const struct abi_params abi_params[SFFI_LAST_ABI] = {
  [SFFI_SYSV] = { 1, R_ECX, 0 },
  [SFFI_THISCALL] = { 1, R_EAX, 1, { R_ECX } },
  [SFFI_FASTCALL] = { 1, R_EAX, 2, { R_ECX, R_EDX } },
  [SFFI_STDCALL] = { 1, R_ECX, 0 },
  [SFFI_PASCAL] = { -1, R_ECX, 0 },
  
  [SFFI_REGISTER] = { -1, R_ECX, 3, { R_EAX, R_EDX, R_ECX } },
  [SFFI_MS_CDECL] = { 1, R_ECX, 0 }
};

#ifdef HAVE_FASTCALL
  #ifdef _MSC_VER
    #define SFFI_DECLARE_FASTCALL __fastcall
  #else
    #define SFFI_DECLARE_FASTCALL __declspec(fastcall)
  #endif
#else
  #define SFFI_DECLARE_FASTCALL
#endif

extern void SFFI_DECLARE_FASTCALL sffi_call_i386(struct call_frame *, char *) SFFI_HIDDEN;


#if defined(_MSC_VER)
#pragma runtime_checks("s", off)
#endif

SFFI_ASAN_NO_SANITIZE
static void
sffi_call_int (sffi_cif *cif, void (*fn)(void), void *rvalue,
	      void **avalue, void *closure)
{
  size_t rsize, bytes;
  struct call_frame *frame;
  char *stack, *argp;
  sffi_type **arg_types;
  int flags, cabi, i, n, dir, narg_reg;
  const struct abi_params *pabi;

  flags = cif->flags;
  cabi = cif->abi;
  pabi = &abi_params[cabi];
  dir = pabi->dir;

  rsize = 0;
  if (rvalue == NULL)
    {
      switch (flags)
	{
	case X86_RET_FLOAT:
	case X86_RET_DOUBLE:
	case X86_RET_LDOUBLE:
	case X86_RET_STRUCTPOP:
	case X86_RET_STRUCTARG:
	  
	  rsize = cif->rtype->size;
	  break;
	default:
	  
	  flags = X86_RET_VOID;
	  break;
	}
    }

  bytes = STACK_ALIGN (cif->bytes);
  stack = alloca(bytes + sizeof(*frame) + rsize);
  argp = (dir < 0 ? stack + bytes : stack);
  frame = (struct call_frame *)(stack + bytes);
  if (rsize)
    rvalue = frame + 1;

  frame->fn = fn;
  frame->flags = flags;
  frame->rvalue = rvalue;
  frame->regs[pabi->static_chain] = (unsigned)closure;

  narg_reg = 0;
  switch (flags)
    {
    case X86_RET_STRUCTARG:
      
      if (pabi->nregs > 0)
	{
	  frame->regs[pabi->regs[0]] = (unsigned)rvalue;
	  narg_reg = 1;
	  break;
	}
      
    case X86_RET_STRUCTPOP:
      *(void **)argp = rvalue;
      argp += sizeof(void *);
      break;
    }

  arg_types = cif->arg_types;
  for (i = 0, n = cif->nargs; i < n; i++)
    {
      sffi_type *ty = arg_types[i];
      void *valp = avalue[i];
      size_t z = ty->size;
      int t = ty->type;

      if (z <= SFFI_SIZEOF_ARG && t != SFFI_TYPE_STRUCT)
        {
	  sffi_arg val = extend_basic_type (valp, t);

	  if (t != SFFI_TYPE_FLOAT && narg_reg < pabi->nregs)
	    frame->regs[pabi->regs[narg_reg++]] = val;
	  else if (dir < 0)
	    {
	      argp -= 4;
	      *(sffi_arg *)argp = val;
	    }
	  else
	    {
	      *(sffi_arg *)argp = val;
	      argp += 4;
	    }
	}
      else
	{
	  size_t za = SFFI_ALIGN (z, SFFI_SIZEOF_ARG);
	  size_t align = SFFI_SIZEOF_ARG;

	  
	  if ((cabi == SFFI_THISCALL || cabi == SFFI_FASTCALL)
	      && (t == SFFI_TYPE_SINT64
		  || t == SFFI_TYPE_UINT64
		  || t == SFFI_TYPE_STRUCT))
	    narg_reg = 2;

	  
	  if (t == SFFI_TYPE_STRUCT && ty->alignment >= 16)
	    align = 16;

	  if (dir < 0)
	    {
	      
	      argp -= za;
	      memcpy (argp, valp, z);
	    }
	  else
	    {
	      argp = (char *)SFFI_ALIGN (argp, align);
	      memcpy (argp, valp, z);
	      argp += za;
	    }
	}
    }
  SFFI_ASSERT (dir > 0 || argp == stack);

  sffi_call_i386 (frame, stack);
}
#if defined(_MSC_VER)
#pragma runtime_checks("s", restore)
#endif

void
sffi_call (sffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  sffi_call_int (cif, fn, rvalue, avalue, NULL);
}

#ifdef SFFI_GO_CLOSURES
void
sffi_call_go (sffi_cif *cif, void (*fn)(void), void *rvalue,
	     void **avalue, void *closure)
{
  sffi_call_int (cif, fn, rvalue, avalue, closure);
}
#endif



void SFFI_HIDDEN sffi_closure_i386(void);
void SFFI_HIDDEN sffi_closure_STDCALL(void);
void SFFI_HIDDEN sffi_closure_REGISTER(void);
#if defined(SFFI_EXEC_STATIC_TRAMP)
void SFFI_HIDDEN sffi_closure_i386_alt(void);
void SFFI_HIDDEN sffi_closure_STDCALL_alt(void);
void SFFI_HIDDEN sffi_closure_REGISTER_alt(void);
#endif

struct closure_frame
{
  unsigned rettemp[4];				
  unsigned regs[3];				
  sffi_cif *cif;					
  void (*fun)(sffi_cif*,void*,void**,void*);	
  void *user_data;				
};

int SFFI_HIDDEN SFFI_DECLARE_FASTCALL
sffi_closure_inner (struct closure_frame *frame, char *stack)
{
  sffi_cif *cif = frame->cif;
  int cabi, i, n, flags, dir, narg_reg;
  const struct abi_params *pabi;
  sffi_type **arg_types;
  char *argp;
  void *rvalue;
  void **avalue;

  cabi = cif->abi;
  flags = cif->flags;
  narg_reg = 0;
  rvalue = frame->rettemp;
  pabi = &abi_params[cabi];
  dir = pabi->dir;
  argp = (dir < 0 ? stack + STACK_ALIGN (cif->bytes) : stack);

  switch (flags)
    {
    case X86_RET_STRUCTARG:
      if (pabi->nregs > 0)
	{
	  rvalue = (void *)frame->regs[pabi->regs[0]];
	  narg_reg = 1;
	  frame->rettemp[0] = (unsigned)rvalue;
	  break;
	}
      
    case X86_RET_STRUCTPOP:
      rvalue = *(void **)argp;
      argp += sizeof(void *);
      frame->rettemp[0] = (unsigned)rvalue;
      break;
    }

  n = cif->nargs;
  avalue = alloca(sizeof(void *) * n);

  arg_types = cif->arg_types;
  for (i = 0; i < n; ++i)
    {
      sffi_type *ty = arg_types[i];
      size_t z = ty->size;
      int t = ty->type;
      void *valp;

      if (z <= SFFI_SIZEOF_ARG && t != SFFI_TYPE_STRUCT)
	{
	  if (t != SFFI_TYPE_FLOAT && narg_reg < pabi->nregs)
	    valp = &frame->regs[pabi->regs[narg_reg++]];
	  else if (dir < 0)
	    {
	      argp -= 4;
	      valp = argp;
	    }
	  else
	    {
	      valp = argp;
	      argp += 4;
	    }
	}
      else
	{
	  size_t za = SFFI_ALIGN (z, SFFI_SIZEOF_ARG);
	  size_t align = SFFI_SIZEOF_ARG;

	  
	  if (t == SFFI_TYPE_STRUCT && ty->alignment >= 16)
	    align = 16;

	  
	  if ((cabi == SFFI_THISCALL || cabi == SFFI_FASTCALL)
	      && (t == SFFI_TYPE_SINT64
		  || t == SFFI_TYPE_UINT64
		  || t == SFFI_TYPE_STRUCT))
	    narg_reg = 2;

	  if (dir < 0)
	    {
	      
	      argp -= za;
	      valp = argp;
	    }
	  else
	    {
	      argp = (char *)SFFI_ALIGN (argp, align);
	      valp = argp;
	      argp += za;
	    }
	}

      avalue[i] = valp;
    }

  frame->fun (cif, rvalue, avalue, frame->user_data);

  switch (cabi)
    {
    case SFFI_STDCALL:
      return flags | (cif->bytes << X86_RET_POP_SHIFT);
    case SFFI_THISCALL:
    case SFFI_FASTCALL:
      
      return flags | (((unsigned) (argp - stack)) << X86_RET_POP_SHIFT);
    default:
      return flags;
    }
}

sffi_status
sffi_prep_closure_loc (sffi_closure* closure,
                      sffi_cif* cif,
                      void (*fun)(sffi_cif*,void*,void**,void*),
                      void *user_data,
                      void *codeloc)
{
  char *tramp = closure->tramp;
  void (*dest)(void);
  int op = 0xb8;  

  switch (cif->abi)
    {
    case SFFI_SYSV:
    case SFFI_MS_CDECL:
      dest = sffi_closure_i386;
      break;
    case SFFI_STDCALL:
    case SFFI_THISCALL:
    case SFFI_FASTCALL:
    case SFFI_PASCAL:
      dest = sffi_closure_STDCALL;
      break;
    case SFFI_REGISTER:
      dest = sffi_closure_REGISTER;
      op = 0x68;  
      break;
    default:
      return SFFI_BAD_ABI;
    }

#if defined(SFFI_EXEC_STATIC_TRAMP)
  if (sffi_tramp_is_present(closure))
    {
      
      if (dest == sffi_closure_i386)
        dest = sffi_closure_i386_alt;
      else if (dest == sffi_closure_STDCALL)
        dest = sffi_closure_STDCALL_alt;
      else
        dest = sffi_closure_REGISTER_alt;
      sffi_tramp_set_parms (closure->ftramp, dest, closure);
      goto out;
    }
#endif

  
  
  *(UINT32 *) tramp = 0xfb1e0ff3;

  
  tramp[4] = op;
  *(void **)(tramp + 5) = codeloc;

  
  tramp[9] = 0xe9;
  *(unsigned *)(tramp + 10) = (unsigned)dest - ((unsigned)codeloc + 14);

#if defined(SFFI_EXEC_STATIC_TRAMP)
out:
#endif
  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  return SFFI_OK;
}

#ifdef SFFI_GO_CLOSURES

void SFFI_HIDDEN sffi_go_closure_EAX(void);
void SFFI_HIDDEN sffi_go_closure_ECX(void);
void SFFI_HIDDEN sffi_go_closure_STDCALL(void);

sffi_status
sffi_prep_go_closure (sffi_go_closure* closure, sffi_cif* cif,
		     void (*fun)(sffi_cif*,void*,void**,void*))
{
  void (*dest)(void);

  switch (cif->abi)
    {
    case SFFI_SYSV:
    case SFFI_MS_CDECL:
      dest = sffi_go_closure_ECX;
      break;
    case SFFI_THISCALL:
    case SFFI_FASTCALL:
      dest = sffi_go_closure_EAX;
      break;
    case SFFI_STDCALL:
    case SFFI_PASCAL:
      dest = sffi_go_closure_STDCALL;
      break;
    case SFFI_REGISTER:
    default:
      return SFFI_BAD_ABI;
    }

  closure->tramp = dest;
  closure->cif = cif;
  closure->fun = fun;

  return SFFI_OK;
}

#endif 



#if !SFFI_NO_RAW_API

void SFFI_HIDDEN sffi_closure_raw_SYSV(void);
void SFFI_HIDDEN sffi_closure_raw_THISCALL(void);

sffi_status
sffi_prep_raw_closure_loc (sffi_raw_closure *closure,
                          sffi_cif *cif,
                          void (*fun)(sffi_cif*,void*,sffi_raw*,void*),
                          void *user_data,
                          void *codeloc)
{
  char *tramp = closure->tramp;
  void (*dest)(void);
  int i;

  
  for (i = cif->nargs-1; i >= 0; i--)
    switch (cif->arg_types[i]->type)
      {
      case SFFI_TYPE_STRUCT:
      case SFFI_TYPE_LONGDOUBLE:
	return SFFI_BAD_TYPEDEF;
      }

  switch (cif->abi)
    {
    case SFFI_THISCALL:
      dest = sffi_closure_raw_THISCALL;
      break;
    case SFFI_SYSV:
      dest = sffi_closure_raw_SYSV;
      break;
    default:
      return SFFI_BAD_ABI;
    }

  
  tramp[0] = 0xb8;
  *(void **)(tramp + 1) = codeloc;

  
  tramp[5] = 0xe9;
  *(unsigned *)(tramp + 6) = (unsigned)dest - ((unsigned)codeloc + 10);

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  return SFFI_OK;
}

void
sffi_raw_call(sffi_cif *cif, void (*fn)(void), void *rvalue, sffi_raw *avalue)
{
  size_t rsize, bytes;
  struct call_frame *frame;
  char *stack, *argp;
  sffi_type **arg_types;
  int flags, cabi, i, n, narg_reg;
  const struct abi_params *pabi;

  flags = cif->flags;
  cabi = cif->abi;
  pabi = &abi_params[cabi];

  rsize = 0;
  if (rvalue == NULL)
    {
      switch (flags)
	{
	case X86_RET_FLOAT:
	case X86_RET_DOUBLE:
	case X86_RET_LDOUBLE:
	case X86_RET_STRUCTPOP:
	case X86_RET_STRUCTARG:
	  
	  rsize = cif->rtype->size;
	  break;
	default:
	  
	  flags = X86_RET_VOID;
	  break;
	}
    }

  bytes = STACK_ALIGN (cif->bytes);
  argp = stack =
      (void *)((uintptr_t)alloca(bytes + sizeof(*frame) + rsize + 15) & ~16);
  frame = (struct call_frame *)(stack + bytes);
  if (rsize)
    rvalue = frame + 1;

  frame->fn = fn;
  frame->flags = flags;
  frame->rvalue = rvalue;

  narg_reg = 0;
  switch (flags)
    {
    case X86_RET_STRUCTARG:
      
      if (pabi->nregs > 0)
	{
	  frame->regs[pabi->regs[0]] = (unsigned)rvalue;
	  narg_reg = 1;
	  break;
	}
      
    case X86_RET_STRUCTPOP:
      *(void **)argp = rvalue;
      argp += sizeof(void *);
      bytes -= sizeof(void *);
      break;
    }

  arg_types = cif->arg_types;
  for (i = 0, n = cif->nargs; narg_reg < pabi->nregs && i < n; i++)
    {
      sffi_type *ty = arg_types[i];
      size_t z = ty->size;
      int t = ty->type;

      if (z <= SFFI_SIZEOF_ARG && t != SFFI_TYPE_STRUCT && t != SFFI_TYPE_FLOAT)
	{
	  sffi_arg val = extend_basic_type (avalue, t);
	  frame->regs[pabi->regs[narg_reg++]] = val;
	  z = SFFI_SIZEOF_ARG;
	}
      else
	{
	  memcpy (argp, avalue, z);
	  z = SFFI_ALIGN (z, SFFI_SIZEOF_ARG);
	  argp += z;
	}
      avalue += z;
      bytes -= z;
    }
  if (i < n)
    memcpy (argp, avalue, bytes);

  sffi_call_i386 (frame, stack);
}
#endif 

#if defined(SFFI_EXEC_STATIC_TRAMP)
void *
sffi_tramp_arch (size_t *tramp_size, size_t *map_size)
{
  extern void *trampoline_code_table;

  *map_size = X86_TRAMP_MAP_SIZE;
  *tramp_size = X86_TRAMP_SIZE;
  return &trampoline_code_table;
}
#endif

#endif 
