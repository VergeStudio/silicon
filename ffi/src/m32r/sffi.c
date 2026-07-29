/* -----------------------------------------------------------------------
   ffi.c - Copyright (c) 2004  Renesas Technology
           Copyright (c) 2008  Red Hat, Inc.
           Copyright (c) 2022, 2026  Anthony Green

   M32R Foreign Function Interface

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   ``Software''), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL RENESAS TECHNOLOGY BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.
   ----------------------------------------------------------------------- */

#include <sffi.h>
#include <sffi_common.h>

#include <stdlib.h>

/* sffi_prep_args is called by the assembly routine once stack
   space has been allocated for the function's arguments.  */

void sffi_prep_args(char *stack, extended_cif *ecif)
{
  unsigned int i;
  int tmp;
  unsigned int avn;
  void **p_argv;
  char *argp;
  sffi_type **p_arg;

  tmp = 0;
  argp = stack;

  if (ecif->cif->rtype->type == SFFI_TYPE_STRUCT && ecif->cif->rtype->size > 8)
    {
      *(void **) argp = ecif->rvalue;
      argp += 4;
    }

  avn = ecif->cif->nargs;
  p_argv = ecif->avalue;

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types;
       (i != 0) && (avn != 0);
       i--, p_arg++)
    {
      size_t z;

      /* Align if necessary.  */
      if (((*p_arg)->alignment - 1) & (unsigned) argp)
	argp = (char *) SFFI_ALIGN (argp, (*p_arg)->alignment);

      if (avn != 0)
	{
	  avn--;
	  z = (*p_arg)->size;
	  if (z < sizeof (int))
	    {
	      z = sizeof (int);

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
	  	  z = (*p_arg)->size;
	  	  if ((*p_arg)->alignment != 1)
		    memcpy (argp, *p_argv, z);
		  else
		    memcpy (argp + 4 - z, *p_argv, z);
	  	  z = sizeof (int);
		  break;

		default:
		  SFFI_ASSERT(0);
		}
	    }
	  else if (z == sizeof (int))
	    {
	       *(unsigned int *) argp = (unsigned int)*(UINT32 *)(* p_argv);
	    }
	  else
	    {
	      if ((*p_arg)->type == SFFI_TYPE_STRUCT)
	        {
		  if (z > 8)
		    {
		      *(unsigned int *) argp = (unsigned int)(void *)(* p_argv);
		      z = sizeof(void *);
		    }
		  else
		    {
	              memcpy(argp, *p_argv, z);
		      z = 8;
		    }
	        }
	      else
	        {
		  /* Double or long long 64bit.  */
	          memcpy (argp, *p_argv, z);
	        }
	    }
	  p_argv++;
	  argp += z;
	}
    }

  return;
}

/* Perform machine dependent cif processing.  */
sffi_status
sffi_prep_cif_machdep(sffi_cif *cif)
{
  /* Set the return type flag.  */
  switch (cif->rtype->type)
    {
    case SFFI_TYPE_VOID:
      cif->flags = (unsigned) cif->rtype->type;
      break;

    case SFFI_TYPE_STRUCT:
      if (cif->rtype->size <= 4)
	cif->flags = SFFI_TYPE_INT;

      else if (cif->rtype->size <= 8)
	cif->flags = SFFI_TYPE_DOUBLE;

      else
	cif->flags = (unsigned) cif->rtype->type;
      break;

    case SFFI_TYPE_SINT64:
    case SFFI_TYPE_UINT64:
    case SFFI_TYPE_DOUBLE:
      cif->flags = SFFI_TYPE_DOUBLE;
      break;

    case SFFI_TYPE_FLOAT:
    default:
      cif->flags = SFFI_TYPE_INT;
      break;
    }

  return SFFI_OK;
}

extern void sffi_call_SYSV(void (*)(char *, extended_cif *), extended_cif *,
			  unsigned, unsigned, unsigned *, void (*fn)(void));

void sffi_call(sffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  extended_cif ecif;
  sffi_type **arg_types = cif->arg_types;
  void **avalue_copy = NULL;
  int i, nargs = cif->nargs;

  ecif.cif = cif;

  /* If the return value is a struct and we don't have
     a return value address then we need to make one.  */
  if ((rvalue == NULL) &&
      (cif->rtype->type == SFFI_TYPE_STRUCT))
    {
      ecif.rvalue = alloca (cif->rtype->size);
    }
  else
    ecif.rvalue = rvalue;

  /* If we have any large structure arguments, make a copy so we are passing
     by value.  The pointer array is cloned first: the caller owns avalue[]
     and may reuse it for another call, so it must not be modified.  */
  for (i = 0; i < nargs; i++)
    {
      sffi_type *at = arg_types[i];
      int size = at->size;
      if (at->type == SFFI_TYPE_STRUCT && size > 4)
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
    case SFFI_SYSV:
      sffi_call_SYSV(sffi_prep_args, &ecif, cif->bytes,
		    cif->flags, ecif.rvalue, fn);
      if (cif->rtype->type == SFFI_TYPE_STRUCT)
	{
	  int size = cif->rtype->size;
	  int align = cif->rtype->alignment;

	  if (size < 4)
	    {
	      if (align == 1)
	        *(unsigned long *)(ecif.rvalue) <<= (4 - size) * 8;
	    }
	  else if (4 < size && size < 8)
	    {
	      if (align == 1)
		{
		  memcpy (ecif.rvalue, ecif.rvalue + 8-size, size);
		}
	      else if (align == 2)
		{
		  if (size & 1)
		    size += 1;

		  if (size != 8)
		    memcpy (ecif.rvalue, ecif.rvalue + 8-size, size);
		}
	    }
	}
      break;

    default:
      SFFI_ASSERT(0);
      break;
    }
}
