#include <sffi.h>
#include <sffi_common.h>

#include <stdlib.h>
#include <unistd.h>

#define CIF_FLAGS_CHAR		1	
#define CIF_FLAGS_SHORT		2	
#define CIF_FLAGS_INT		4
#define CIF_FLAGS_DINT		8

void sffi_call_elfbsd (extended_cif *, unsigned, unsigned, void *,
		       void (*) ());
void *sffi_prep_args (extended_cif *ecif, void *stack);

void *
sffi_prep_args (extended_cif *ecif, void *stack)
{
  unsigned int i;
  void **p_argv;
  char *argp;
  sffi_type **p_arg;
  void *struct_value_ptr;

  argp = stack;

  if (ecif->cif->rtype->type == SFFI_TYPE_STRUCT
      && !ecif->cif->flags)
    struct_value_ptr = ecif->rvalue;
  else
    struct_value_ptr = NULL;

  p_argv = ecif->avalue;

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types;
       i != 0;
       i--, p_arg++)
    {
      size_t z;

      z = (*p_arg)->size;
      if (z < sizeof (int))
	{
	  switch ((*p_arg)->type)
	    {
	    case SFFI_TYPE_SINT8:
	      *(signed int *) argp = (signed int) *(SINT8 *) *p_argv;
	      break;

	    case SFFI_TYPE_UINT8:
	      *(unsigned int *) argp = (unsigned int) *(UINT8 *) *p_argv;
	      break;

	    case SFFI_TYPE_SINT16:
	      *(signed int *) argp = (signed int) *(SINT16 *) *p_argv;
	      break;

	    case SFFI_TYPE_UINT16:
	      *(unsigned int *) argp = (unsigned int) *(UINT16 *) *p_argv;
	      break;

	    case SFFI_TYPE_STRUCT:
	      memcpy (argp, *p_argv, z);
	      break;

	    default:
	      SFFI_ASSERT (0);
	    }
	  z = sizeof (int);
	}
      else
	{
	  memcpy (argp, *p_argv, z);

	  if ((sizeof(int) - 1) & z)
	    z = SFFI_ALIGN(z, sizeof(int));
	}

      p_argv++;
      argp += z;
    }

  return struct_value_ptr;
}

sffi_status
sffi_prep_cif_machdep (sffi_cif *cif)
{

  switch (cif->rtype->type)
    {
    case SFFI_TYPE_VOID:
      cif->flags = 0;
      break;

    case SFFI_TYPE_STRUCT:
      if (cif->rtype->elements[0]->type == SFFI_TYPE_STRUCT &&
	  cif->rtype->elements[1])
	{
	  cif->flags = 0;
	  break;
	}

      if (cif->rtype->size == sizeof (char))
	cif->flags = CIF_FLAGS_CHAR;
      else if (cif->rtype->size == sizeof (short))
	cif->flags = CIF_FLAGS_SHORT;
      else if (cif->rtype->size == sizeof (int))
	cif->flags = CIF_FLAGS_INT;
      else if (cif->rtype->size == 2 * sizeof (int))
	cif->flags = CIF_FLAGS_DINT;
      else
	cif->flags = 0;
      break;

    default:
      if (cif->rtype->size <= sizeof (int))
	cif->flags = CIF_FLAGS_INT;
      else
	cif->flags = CIF_FLAGS_DINT;
      break;
    }

  return SFFI_OK;
}

void
sffi_call (sffi_cif *cif, void (*fn) (), void *rvalue, void **avalue)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;

  if (rvalue == NULL
      && cif->rtype->type == SFFI_TYPE_STRUCT
      && cif->flags == 0)
    ecif.rvalue = alloca (cif->rtype->size);
  else
    ecif.rvalue = rvalue;

  switch (cif->abi)
    {
    case SFFI_ELFBSD:
      sffi_call_elfbsd (&ecif, cif->bytes, cif->flags, ecif.rvalue, fn);
      break;

    default:
      SFFI_ASSERT (0);
      break;
    }
}

void sffi_closure_elfbsd (void);
void sffi_closure_struct_elfbsd (void);
unsigned int sffi_closure_elfbsd_inner (sffi_closure *, void *, char *);

static void
sffi_prep_closure_elfbsd (sffi_cif *cif, void **avalue, char *stackp)
{
  unsigned int i;
  void **p_argv;
  sffi_type **p_arg;

  p_argv = avalue;

  for (i = cif->nargs, p_arg = cif->arg_types; i != 0; i--, p_arg++)
    {
      size_t z;

      z = (*p_arg)->size;
      *p_argv = stackp;

      if ((sizeof (int) - 1) & z)
	z = SFFI_ALIGN(z, sizeof (int));

      p_argv++;
      stackp += z;
    }
}

unsigned int
sffi_closure_elfbsd_inner (sffi_closure *closure, void *resp, char *stack)
{
  sffi_cif *cif;
  void **arg_area;

  cif = closure->cif;
  arg_area = (void **) alloca (cif->nargs * sizeof (void *));

  sffi_prep_closure_elfbsd (cif, arg_area, stack);

  (closure->fun) (cif, resp, arg_area, closure->user_data);

  return cif->flags;
}

sffi_status
sffi_prep_closure_loc (sffi_closure *closure, sffi_cif *cif,
		      void (*fun)(sffi_cif *, void *, void **, void *),
		      void *user_data, void *codeloc)
{
  char *tramp = (char *) codeloc;
  void *fn;

  SFFI_ASSERT (cif->abi == SFFI_ELFBSD);

  *(unsigned short *)(tramp + 0) = 0x0000;

  tramp[2] = 0xd0;
  tramp[3] = 0x8f;
  *(unsigned int *)(tramp + 4) = (unsigned int) closure;
  tramp[8] = 0x50;

  if (cif->rtype->type == SFFI_TYPE_STRUCT
      && !cif->flags)
    fn = &sffi_closure_struct_elfbsd;
  else
    fn = &sffi_closure_elfbsd;

  tramp[9] = 0x17;
  tramp[10] = 0xef;
  *(unsigned int *)(tramp + 11) = (unsigned int)fn + 2 -
				  (unsigned int)tramp - 9 - 6;

  closure->cif = cif;
  closure->user_data = user_data;
  closure->fun = fun;

  return SFFI_OK;
}
