#include <sffi.h>
#include <sffi_common.h>

#include <stdlib.h>

void *sffi_prep_args(char *stack, extended_cif *ecif)
{
  register unsigned int i;
  register void **p_argv;
  register char *argp;
  register sffi_type **p_arg;
  register int count = 0;

  p_argv = ecif->avalue;
  argp = stack;

  if (ecif->cif->rtype->type == SFFI_TYPE_STRUCT)
    {
      *(void **) argp = ecif->rvalue;
      argp += 4;
    }

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types;
       (i != 0);
       i--, p_arg++)
    {
      size_t z;

      z = (*p_arg)->size;

      if ((*p_arg)->type == SFFI_TYPE_STRUCT)
	{
	  z = sizeof(void*);
	  *(void **) argp = *p_argv;
	}
      else if (z < sizeof(int))
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
      count += z;
    }

  return (stack + ((count > 24) ? 24 : SFFI_ALIGN_DOWN(count, 8)));
}

sffi_status sffi_prep_cif_machdep(sffi_cif *cif)
{
  if (cif->rtype->type == SFFI_TYPE_STRUCT)
    cif->flags = -1;
  else
    cif->flags = cif->rtype->size;

  cif->bytes = SFFI_ALIGN (cif->bytes, 8);

  return SFFI_OK;
}

extern void sffi_call_EABI(void *(*)(char *, extended_cif *),
			  extended_cif *,
			  unsigned, unsigned,
			  unsigned *,
			  void (*fn)(void));

void sffi_call(sffi_cif *cif,
	      void (*fn)(void),
	      void *rvalue,
	      void **avalue)
{
  extended_cif ecif;
  sffi_type **arg_types = cif->arg_types;
  void **avalue_copy = NULL;
  int i, nargs = cif->nargs;

  ecif.cif = cif;

  if ((rvalue == NULL) &&
      (cif->rtype->type == SFFI_TYPE_STRUCT))
    {
      ecif.rvalue = alloca(cif->rtype->size);
    }
  else
    ecif.rvalue = rvalue;

  for (i = 0; i < nargs; i++)
    {
      sffi_type *at = arg_types[i];
      int size = at->size;
      if (at->type == SFFI_TYPE_STRUCT) 
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
  ecif.avalue = avalue;

  switch (cif->abi)
    {
    case SFFI_EABI:
      sffi_call_EABI(sffi_prep_args, &ecif, cif->bytes,
		    cif->flags, ecif.rvalue, fn);
      break;
    default:
      SFFI_ASSERT(0);
      break;
    }
}

void sffi_closure_eabi (unsigned arg1, unsigned arg2, unsigned arg3,
		       unsigned arg4, unsigned arg5, unsigned arg6)
{

  register sffi_closure *creg __asm__ ("$r12");
  sffi_closure *closure = creg;

  register char *frame_pointer __asm__ ("$fp");

  void *struct_rvalue = (void *) arg1;

  char *stack_args = frame_pointer + 9*4;

  unsigned register_args[6] =
    { arg1, arg2, arg3, arg4, arg5, arg6 };
  char *register_args_ptr = (char *) register_args;

  sffi_cif *cif = closure->cif;
  sffi_type **arg_types = cif->arg_types;
  void **avalue = alloca (cif->nargs * sizeof(void *));
  char *ptr = (char *) register_args;
  int i;

  if ((cif->rtype != NULL) && (cif->rtype->type == SFFI_TYPE_STRUCT)) {
    ptr += 4;
    register_args_ptr = (char *)&register_args[1];
  }

  for (i = 0; i < cif->nargs; i++)
    {
      switch (arg_types[i]->type)
	{
	case SFFI_TYPE_SINT8:
	case SFFI_TYPE_UINT8:
	  avalue[i] = ptr + 3;
	  break;
	case SFFI_TYPE_SINT16:
	case SFFI_TYPE_UINT16:
	  avalue[i] = ptr + 2;
	  break;
	case SFFI_TYPE_SINT32:
	case SFFI_TYPE_UINT32:
	case SFFI_TYPE_FLOAT:
	case SFFI_TYPE_POINTER:
	  avalue[i] = ptr;
	  break;
	case SFFI_TYPE_STRUCT:
          {
            if (arg_types[i]->size > 4)
              {
                void *copy = alloca(arg_types[i]->size);
                memcpy(copy, *(void**)ptr, arg_types[i]->size);
                avalue[i] = copy;
              }
            else
              avalue[i] = *(void**)ptr;
          }
	  break;
	default:

	  if (ptr == (char *) &register_args[5])
	    {

	      unsigned *ip = alloca(8);
	      avalue[i] = ip;
	      ip[0] = *(unsigned *) ptr;
	      ip[1] = *(unsigned *) stack_args;
	    }
	  else
	    {
	      avalue[i] = ptr;
	    }
	  ptr += 4;
	  break;
	}
      ptr += 4;

      if (ptr == (char *) &register_args[6])
	ptr = stack_args;
      else if (ptr == (char *) &register_args[7])
	ptr = stack_args + 4;
    }

  if (cif->rtype && (cif->rtype->type == SFFI_TYPE_STRUCT))
    {
      (closure->fun) (cif, struct_rvalue, avalue, closure->user_data);
    }
  else
    {

      long long rvalue;
      (closure->fun) (cif, &rvalue, avalue, closure->user_data);
      __asm__ ("mov $r12, %0\n ld.l $r0, ($r12)\n ldo.l $r1, 4($r12)" : : "r" (&rvalue));
    }
}

sffi_status
sffi_prep_closure_loc (sffi_closure* closure,
		      sffi_cif* cif,
		      void (*fun)(sffi_cif*, void*, void**, void*),
		      void *user_data,
		      void *codeloc)
{
  unsigned short *tramp = (unsigned short *) &closure->tramp[0];
  unsigned long fn = (long) sffi_closure_eabi;
  unsigned long cls = (long) codeloc;

  if (cif->abi != SFFI_EABI)
    return SFFI_BAD_ABI;

  fn = (unsigned long) sffi_closure_eabi;

  tramp[0] = 0x01e0; 
  tramp[1] = cls >> 16;
  tramp[2] = cls & 0xffff;
  tramp[3] = 0x1a00; 
  tramp[4] = fn >> 16;
  tramp[5] = fn & 0xffff;

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  return SFFI_OK;
}
