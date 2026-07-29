/* -----------------------------------------------------------------------
   ffi.c - Copyright (c) 2003, 2004, 2006, 2007, 2012 Kaz Kojima
           Copyright (c) 2008, 2026 Anthony Green
   
   SuperH SHmedia Foreign Function Interface 

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   ``Software''), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
   ----------------------------------------------------------------------- */

#include <sffi.h>
#include <sffi_common.h>

#include <stdlib.h>

#define NGREGARG 8
#define NFREGARG 12

static int
return_type (sffi_type *arg)
{

  if (arg->type != SFFI_TYPE_STRUCT)
    return arg->type;

  /* gcc uses r2 if the result can be packed in on register.  */
  if (arg->size <= sizeof (UINT8))
    return SFFI_TYPE_UINT8;
  else if (arg->size <= sizeof (UINT16))
    return SFFI_TYPE_UINT16;
  else if (arg->size <= sizeof (UINT32))
    return SFFI_TYPE_UINT32;
  else if (arg->size <= sizeof (UINT64))
    return SFFI_TYPE_UINT64;

  return SFFI_TYPE_STRUCT;
}

/* sffi_prep_args is called by the assembly routine once stack space
   has been allocated for the function's arguments */

void sffi_prep_args(char *stack, extended_cif *ecif)
{
  register unsigned int i;
  register unsigned int avn;
  register void **p_argv;
  register char *argp;
  register sffi_type **p_arg;

  argp = stack;

  if (return_type (ecif->cif->rtype) == SFFI_TYPE_STRUCT)
    {
      *(void **) argp = ecif->rvalue;
      argp += sizeof (UINT64);
    }

  avn = ecif->cif->nargs;
  p_argv = ecif->avalue;

  for (i = 0, p_arg = ecif->cif->arg_types; i < avn; i++, p_arg++, p_argv++)
    {
      size_t z;
      int align;

      z = (*p_arg)->size;
      align = (*p_arg)->alignment;
      if (z < sizeof (UINT32))
	{
	  switch ((*p_arg)->type)
	    {
	    case SFFI_TYPE_SINT8:
	      *(SINT64 *) argp = (SINT64) *(SINT8 *)(*p_argv);
	      break;
  
	    case SFFI_TYPE_UINT8:
	      *(UINT64 *) argp = (UINT64) *(UINT8 *)(*p_argv);
	      break;
  
	    case SFFI_TYPE_SINT16:
	      *(SINT64 *) argp = (SINT64) *(SINT16 *)(*p_argv);
	      break;
  
	    case SFFI_TYPE_UINT16:
	      *(UINT64 *) argp = (UINT64) *(UINT16 *)(*p_argv);
	      break;
  
	    case SFFI_TYPE_STRUCT:
	      memcpy (argp, *p_argv, z);
	      break;

	    default:
	      SFFI_ASSERT(0);
	    }
	  argp += sizeof (UINT64);
	}
      else if (z == sizeof (UINT32) && align == sizeof (UINT32))
	{
	  switch ((*p_arg)->type)
	    {
	    case SFFI_TYPE_INT:
	    case SFFI_TYPE_SINT32:
	      *(SINT64 *) argp = (SINT64) *(SINT32 *) (*p_argv);
	      break;

	    case SFFI_TYPE_FLOAT:
	    case SFFI_TYPE_POINTER:
	    case SFFI_TYPE_UINT32:
	    case SFFI_TYPE_STRUCT:
	      *(UINT64 *) argp = (UINT64) *(UINT32 *) (*p_argv);
	      break;

	    default:
	      SFFI_ASSERT(0);
	      break;
	    }
	  argp += sizeof (UINT64);
	}
      else if (z == sizeof (UINT64)
	       && align == sizeof (UINT64)
	       && ((int) *p_argv & (sizeof (UINT64) - 1)) == 0)
	{
	  *(UINT64 *) argp = *(UINT64 *) (*p_argv);
	  argp += sizeof (UINT64);
	}
      else
	{
	  int n = (z + sizeof (UINT64) - 1) / sizeof (UINT64);

	  memcpy (argp, *p_argv, z);
	  argp += n * sizeof (UINT64);
	}
    }

  return;
}

/* Perform machine dependent cif processing */
sffi_status sffi_prep_cif_machdep(sffi_cif *cif)
{
  int i, j;
  int size, type;
  int n, m;
  int greg;
  int freg;
  int fpair = -1;

  greg = (return_type (cif->rtype) == SFFI_TYPE_STRUCT ? 1 : 0);
  freg = 0;
  cif->flags2 = 0;

  for (i = j = 0; i < cif->nargs; i++)
    {
      type = (cif->arg_types)[i]->type;
      switch (type)
	{
	case SFFI_TYPE_FLOAT:
	  greg++;
	  cif->bytes += sizeof (UINT64) - sizeof (float);
	  if (freg >= NFREGARG - 1)
	    continue;
	  if (fpair < 0)
	    {
	      fpair = freg;
	      freg += 2;
	    }
	  else
	    fpair = -1;
	  cif->flags2 += ((cif->arg_types)[i]->type) << (2 * j++);
	  break;

	case SFFI_TYPE_DOUBLE:
	  if (greg++ >= NGREGARG && (freg + 1) >= NFREGARG)
	    continue;
	  if ((freg + 1) < NFREGARG)
	    {
	      freg += 2;
	      cif->flags2 += ((cif->arg_types)[i]->type) << (2 * j++);
	    }
	  else
	    cif->flags2 += SFFI_TYPE_INT << (2 * j++);
	  break;
	      
	default:
	  size = (cif->arg_types)[i]->size;
	  if (size < sizeof (UINT64))
	    cif->bytes += sizeof (UINT64) - size;
	  n = (size + sizeof (UINT64) - 1) / sizeof (UINT64);
	  if (greg >= NGREGARG)
	    continue;
	  else if (greg + n - 1 >= NGREGARG)
	    greg = NGREGARG;
	  else
	    greg += n;
	  for (m = 0; m < n; m++)
	    cif->flags2 += SFFI_TYPE_INT << (2 * j++);
	  break;
	}
    }

  /* Set the return type flag */
  switch (cif->rtype->type)
    {
    case SFFI_TYPE_STRUCT:
      cif->flags = return_type (cif->rtype);
      break;

    case SFFI_TYPE_VOID:
    case SFFI_TYPE_FLOAT:
    case SFFI_TYPE_DOUBLE:
    case SFFI_TYPE_SINT64:
    case SFFI_TYPE_UINT64:
      cif->flags = cif->rtype->type;
      break;

    default:
      cif->flags = SFFI_TYPE_INT;
      break;
    }

  return SFFI_OK;
}

/*@-declundef@*/
/*@-exportheader@*/
extern void sffi_call_SYSV(void (*)(char *, extended_cif *), 
			  /*@out@*/ extended_cif *, 
			  unsigned, unsigned, long long,
			  /*@out@*/ unsigned *, 
			  void (*fn)(void));
/*@=declundef@*/
/*@=exportheader@*/

void sffi_call(/*@dependent@*/ sffi_cif *cif, 
	      void (*fn)(void), 
	      /*@out@*/ void *rvalue, 
	      /*@dependent@*/ void **avalue)
{
  extended_cif ecif;
  UINT64 trvalue;

  ecif.cif = cif;
  ecif.avalue = avalue;
  
  /* If the return value is a struct and we don't have a return	*/
  /* value address then we need to make one		        */

  if (cif->rtype->type == SFFI_TYPE_STRUCT
      && return_type (cif->rtype) != SFFI_TYPE_STRUCT)
    ecif.rvalue = &trvalue;
  else if ((rvalue == NULL) && 
      (cif->rtype->type == SFFI_TYPE_STRUCT))
    {
      ecif.rvalue = alloca(cif->rtype->size);
    }
  else
    ecif.rvalue = rvalue;

  switch (cif->abi) 
    {
    case SFFI_SYSV:
      sffi_call_SYSV(sffi_prep_args, &ecif, cif->bytes, cif->flags, cif->flags2,
		    ecif.rvalue, fn);
      break;
    default:
      SFFI_ASSERT(0);
      break;
    }

  if (rvalue
      && cif->rtype->type == SFFI_TYPE_STRUCT
      && return_type (cif->rtype) != SFFI_TYPE_STRUCT)
    memcpy (rvalue, &trvalue, cif->rtype->size);
}

extern void sffi_closure_SYSV (void);
extern void __ic_invalidate (void *line);

sffi_status
sffi_prep_closure_loc (sffi_closure *closure,
		      sffi_cif *cif,
		      void (*fun)(sffi_cif*, void*, void**, void*),
		      void *user_data,
		      void *codeloc)
{
  unsigned int *tramp;

  if (cif->abi != SFFI_SYSV)
    return SFFI_BAD_ABI;

  tramp = (unsigned int *) &closure->tramp[0];
  /* Since sffi_closure is an aligned object, the ffi trampoline is
     called as an SHcompact code.  Sigh.
     SHcompact part:
     mova @(1,pc),r0; add #1,r0; jmp @r0; nop;
     SHmedia part:
     movi fnaddr >> 16,r1; shori fnaddr,r1; ptabs/l r1,tr0
     movi cxt >> 16,r1; shori cxt,r1; blink tr0,r63  */
#ifdef __LITTLE_ENDIAN__
  tramp[0] = 0x7001c701;
  tramp[1] = 0x0009402b;
#else
  tramp[0] = 0xc7017001;
  tramp[1] = 0x402b0009;
#endif
  tramp[2] = 0xcc000010 | (((UINT32) sffi_closure_SYSV) >> 16) << 10;
  tramp[3] = 0xc8000010 | (((UINT32) sffi_closure_SYSV) & 0xffff) << 10;
  tramp[4] = 0x6bf10600;
  tramp[5] = 0xcc000010 | (((UINT32) codeloc) >> 16) << 10;
  tramp[6] = 0xc8000010 | (((UINT32) codeloc) & 0xffff) << 10;
  tramp[7] = 0x4401fff0;

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  /* Flush the icache.  */
  __asm__ volatile ("ocbwb %0,0; synco; icbi %1,0; synci" : : "r" (tramp),
		"r"(codeloc));

  return SFFI_OK;
}

/* Basically the trampoline invokes sffi_closure_SYSV, and on 
 * entry, r3 holds the address of the closure.
 * After storing the registers that could possibly contain
 * parameters to be passed into the stack frame and setting
 * up space for a return value, sffi_closure_SYSV invokes the 
 * following helper function to do most of the work.
 */

int
sffi_closure_helper_SYSV (sffi_closure *closure, UINT64 *rvalue, 
			 UINT64 *pgr, UINT64 *pfr, UINT64 *pst)
{
  void **avalue;
  sffi_type **p_arg;
  int i, avn;
  int greg, freg;
  sffi_cif *cif;
  int fpair = -1;

  cif = closure->cif;
  avalue = alloca (cif->nargs * sizeof (void *));

  /* Copy the caller's structure return value address so that the closure
     returns the data directly to the caller.  */
  if (return_type (cif->rtype) == SFFI_TYPE_STRUCT)
    {
      rvalue = (UINT64 *) *pgr;
      greg = 1;
    }
  else
    greg = 0;

  freg = 0;
  cif = closure->cif;
  avn = cif->nargs;

  /* Grab the addresses of the arguments from the stack frame.  */
  for (i = 0, p_arg = cif->arg_types; i < avn; i++, p_arg++)
    {
      size_t z;
      void *p;

      z = (*p_arg)->size;
      if (z < sizeof (UINT32))
	{
	  p = pgr + greg++;

	  switch ((*p_arg)->type)
	    {
	    case SFFI_TYPE_SINT8:
	    case SFFI_TYPE_UINT8:
	    case SFFI_TYPE_SINT16:
	    case SFFI_TYPE_UINT16:
	    case SFFI_TYPE_STRUCT:
#ifdef __LITTLE_ENDIAN__
	      avalue[i] = p;
#else
	      avalue[i] = ((char *) p) + sizeof (UINT32) - z;
#endif
	      break;

	    default:
	      SFFI_ASSERT(0);
	    }
	}
      else if (z == sizeof (UINT32))
	{
	  if ((*p_arg)->type == SFFI_TYPE_FLOAT)
	    {
	      if (freg < NFREGARG - 1)
		{
		  if (fpair >= 0)
		    {
		      avalue[i] = (UINT32 *) pfr + fpair;
		      fpair = -1;
		    }
		  else
		    {
#ifdef __LITTLE_ENDIAN__
		      fpair = freg;
		      avalue[i] = (UINT32 *) pfr + (1 ^ freg);
#else
		      fpair = 1 ^ freg;
		      avalue[i] = (UINT32 *) pfr + freg;
#endif
		      freg += 2;
		    }
		}
	      else
#ifdef __LITTLE_ENDIAN__
		avalue[i] = pgr + greg;
#else
		avalue[i] = (UINT32 *) (pgr + greg) + 1;
#endif
	    }
	  else
#ifdef __LITTLE_ENDIAN__
	    avalue[i] = pgr + greg;
#else
	    avalue[i] = (UINT32 *) (pgr + greg) + 1;
#endif
	  greg++;
	}
      else if ((*p_arg)->type == SFFI_TYPE_DOUBLE)
	{
	  if (freg + 1 >= NFREGARG)
	    avalue[i] = pgr + greg;
	  else
	    {
	      avalue[i] = pfr + (freg >> 1);
	      freg += 2;
	    }
	  greg++;
	}
      else
	{
	  int n = (z + sizeof (UINT64) - 1) / sizeof (UINT64);

	  avalue[i] = pgr + greg;
	  greg += n;
	}
    }

  (closure->fun) (cif, rvalue, avalue, closure->user_data);

  /* Tell sffi_closure_SYSV how to perform return type promotions.  */
  return return_type (cif->rtype);
}

