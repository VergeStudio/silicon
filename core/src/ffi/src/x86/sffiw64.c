#if defined(__x86_64__) || defined(_M_AMD64)
#include <sffi.h>
#include <sffi_common.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <tramp.h>

#ifdef X86_WIN64
#define EFI64(name) name
#else
#define EFI64(name) SFFI_HIDDEN name##_efi64
#endif

struct win64_call_frame
{
  UINT64 rbp;		
  UINT64 retaddr;	
  UINT64 fn;		
  UINT64 flags;		
  UINT64 rvalue;	
};

extern void sffi_call_win64 (void *stack, struct win64_call_frame *,
			    void *closure) SFFI_HIDDEN;

sffi_status SFFI_HIDDEN
EFI64(sffi_prep_cif_machdep)(sffi_cif *cif)
{
  int flags, n;

  switch (cif->abi)
    {
    case SFFI_WIN64:
    case SFFI_GNUW64:
      break;
    default:
      return SFFI_BAD_ABI;
    }

  flags = cif->rtype->type;
  switch (flags)
    {
    default:
      break;
    case SFFI_TYPE_LONGDOUBLE:

      if (cif->abi == SFFI_GNUW64)
	flags = SFFI_TYPE_STRUCT;
      break;
    case SFFI_TYPE_COMPLEX:
      flags = SFFI_TYPE_STRUCT;

    case SFFI_TYPE_STRUCT:
      switch (cif->rtype->size)
	{
	case 8:
	  flags = SFFI_TYPE_UINT64;
	  break;
	case 4:
	  flags = SFFI_TYPE_SMALL_STRUCT_4B;
	  break;
	case 2:
	  flags = SFFI_TYPE_SMALL_STRUCT_2B;
	  break;
	case 1:
	  flags = SFFI_TYPE_SMALL_STRUCT_1B;
	  break;
	}
      break;
    }
  cif->flags = flags;

  n = cif->nargs;
  n += (flags == SFFI_TYPE_STRUCT);
  if (n < 4)
    n = 4;
  cif->bytes = n * 8;

  return SFFI_OK;
}

#if defined(_MSC_VER)
#pragma runtime_checks("s", off)
#endif
SFFI_ASAN_NO_SANITIZE
static void
sffi_call_int (sffi_cif *cif, void (*fn)(void), void *rvalue,
	      void **avalue, void *closure)
{
  int i, j, n, flags;
  UINT64 *stack;
  size_t rsize;
  struct win64_call_frame *frame;
  sffi_type **arg_types = cif->arg_types;
  void **avalue_copy = NULL;
  int nargs = cif->nargs;

  SFFI_ASSERT(cif->abi == SFFI_GNUW64 || cif->abi == SFFI_WIN64);

  for (i = 0; i < nargs; i++)
    {
      sffi_type *at = arg_types[i];
      int size = at->size;
      bool needcopy = false;

      switch (at->type)
	{
	case SFFI_TYPE_UINT128:
	case SFFI_TYPE_SINT128:
	  needcopy = true;
	  break;
	case SFFI_TYPE_STRUCT:
	  switch (size)
	    {
	    case 1:
	    case 2:
	    case 4:
	    case 8:
	      break;
	    default:
	      needcopy = true;
	    }
	}
      if (needcopy)
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

  flags = cif->flags;
  rsize = 0;

  if (rvalue == NULL)
    {
      if (flags == SFFI_TYPE_STRUCT)
	rsize = cif->rtype->size;
      else
	flags = SFFI_TYPE_VOID;
    }

  stack = alloca(cif->bytes + sizeof(struct win64_call_frame) + rsize);
  frame = (struct win64_call_frame *)((char *)stack + cif->bytes);
  if (rsize)
    rvalue = frame + 1;

  frame->fn = (uintptr_t)fn;
  frame->flags = flags;
  frame->rvalue = (uintptr_t)rvalue;

  j = 0;
  if (flags == SFFI_TYPE_STRUCT)
    {
      stack[0] = (uintptr_t)rvalue;
      j = 1;
    }

  for (i = 0, n = cif->nargs; i < n; ++i, ++j)
    {
      switch (cif->arg_types[i]->size)
	{
	case 8:
	  stack[j] = *(UINT64 *)avalue[i];
	  break;
	case 4:
	  stack[j] = *(UINT32 *)avalue[i];
	  break;
	case 2:
	  stack[j] = *(UINT16 *)avalue[i];
	  break;
	case 1:
	  stack[j] = *(UINT8 *)avalue[i];
	  break;
	default:
	  stack[j] = (uintptr_t)avalue[i];
	  break;
	}
    }

  sffi_call_win64 (stack, frame, closure);
}
#if defined(_MSC_VER)
#pragma runtime_checks("s", restore)
#endif

void
EFI64(sffi_call)(sffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  sffi_call_int (cif, fn, rvalue, avalue, NULL);
}

void
EFI64(sffi_call_go)(sffi_cif *cif, void (*fn)(void), void *rvalue,
	     void **avalue, void *closure)
{
  sffi_call_int (cif, fn, rvalue, avalue, closure);
}

extern void sffi_closure_win64(void) SFFI_HIDDEN;
#if defined(SFFI_EXEC_STATIC_TRAMP)
extern void sffi_closure_win64_alt(void) SFFI_HIDDEN;
#endif

#ifdef SFFI_GO_CLOSURES
extern void sffi_go_closure_win64(void) SFFI_HIDDEN;
#endif

sffi_status
EFI64(sffi_prep_closure_loc)(sffi_closure* closure,
		      sffi_cif* cif,
		      void (*fun)(sffi_cif*, void*, void**, void*),
		      void *user_data,
		      void *codeloc MAYBE_UNUSED)
{
  static const unsigned char trampoline[SFFI_TRAMPOLINE_SIZE - 8] = {

    0xf3, 0x0f, 0x1e, 0xfa,

    0x4c, 0x8d, 0x15, 0xf5, 0xff, 0xff, 0xff,

    0xff, 0x25, 0x07, 0x00, 0x00, 0x00,

    0x0f, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00
  };
  char *tramp = closure->tramp;

  switch (cif->abi)
    {
    case SFFI_WIN64:
    case SFFI_GNUW64:
      break;
    default:
      return SFFI_BAD_ABI;
    }

#if defined(SFFI_EXEC_STATIC_TRAMP)
  if (sffi_tramp_is_present(closure))
    {

      sffi_tramp_set_parms (closure->ftramp, sffi_closure_win64_alt, closure);
      goto out;
    }
#endif

  memcpy (tramp, trampoline, sizeof(trampoline));
  *(UINT64 *)(tramp + sizeof (trampoline)) = (uintptr_t)sffi_closure_win64;

#if defined(SFFI_EXEC_STATIC_TRAMP)
out:
#endif
  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  return SFFI_OK;
}

#ifdef SFFI_GO_CLOSURES
sffi_status
EFI64(sffi_prep_go_closure)(sffi_go_closure* closure, sffi_cif* cif,
		     void (*fun)(sffi_cif*, void*, void**, void*))
{
  switch (cif->abi)
    {
    case SFFI_WIN64:
    case SFFI_GNUW64:
      break;
    default:
      return SFFI_BAD_ABI;
    }

  closure->tramp = sffi_go_closure_win64;
  closure->cif = cif;
  closure->fun = fun;

  return SFFI_OK;
}
#endif

struct win64_closure_frame
{
  UINT64 rvalue[2];
  UINT64 fargs[4];
  UINT64 retaddr;
  UINT64 args[];
};

int SFFI_HIDDEN __attribute__((ms_abi))
sffi_closure_win64_inner(sffi_cif *cif,
			void (*fun)(sffi_cif*, void*, void**, void*),
			void *user_data,
			struct win64_closure_frame *frame)
{
  void **avalue;
  void *rvalue;
  int i, n, nreg, flags;

  avalue = alloca(cif->nargs * sizeof(void *));
  rvalue = frame->rvalue;
  nreg = 0;

  flags = cif->flags;
  if (flags == SFFI_TYPE_STRUCT)
    {
      rvalue = (void *)(uintptr_t)frame->args[0];
      frame->rvalue[0] = frame->args[0];
      nreg = 1;
    }

  for (i = 0, n = cif->nargs; i < n; ++i, ++nreg)
    {
      size_t size = cif->arg_types[i]->size;
      size_t type = cif->arg_types[i]->type;
      void *a;

      if (type == SFFI_TYPE_DOUBLE || type == SFFI_TYPE_FLOAT)
	{
	  if (nreg < 4)
	    a = &frame->fargs[nreg];
	  else
	    a = &frame->args[nreg];
	}
      else if (size == 1 || size == 2 || size == 4 || size == 8)
	a = &frame->args[nreg];
      else
	a = (void *)(uintptr_t)frame->args[nreg];

      avalue[i] = a;
    }

  fun (cif, rvalue, avalue, user_data);
  return flags;
}

#endif 
