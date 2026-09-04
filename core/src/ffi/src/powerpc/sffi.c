#include "sffi.h"
#include "sffi_common.h"
#include "sffi_powerpc.h"
#include "internal.h"
#include <tramp.h>

#if HAVE_LONG_DOUBLE_VARIANT

void SFFI_HIDDEN
sffi_prep_types (sffi_abi abi)
{
# if SFFI_TYPE_LONGDOUBLE != SFFI_TYPE_DOUBLE
#  ifdef POWERPC64
  sffi_prep_types_linux64 (abi);
#  else
  sffi_prep_types_sysv (abi);
#  endif
# endif
}
#endif

sffi_status SFFI_HIDDEN
sffi_prep_cif_machdep (sffi_cif *cif)
{
#ifdef POWERPC64
  return sffi_prep_cif_linux64 (cif);
#else
  return sffi_prep_cif_sysv (cif);
#endif
}

sffi_status SFFI_HIDDEN
sffi_prep_cif_machdep_var (sffi_cif *cif,
			  unsigned int nfixedargs MAYBE_UNUSED,
			  unsigned int ntotalargs MAYBE_UNUSED)
{
#ifdef POWERPC64
  return sffi_prep_cif_linux64_var (cif, nfixedargs, ntotalargs);
#else
  return sffi_prep_cif_sysv (cif);
#endif
}

static void
sffi_call_int (sffi_cif *cif,
	      void (*fn) (void),
	      void *rvalue,
	      void **avalue,
	      void *closure)
{

  float128 smst_buffer[8];
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;

  ecif.rvalue = rvalue;
  if ((cif->flags & FLAG_RETURNS_SMST) != 0)
    ecif.rvalue = smst_buffer;

  else if (!rvalue && cif->rtype->type == SFFI_TYPE_STRUCT)
    ecif.rvalue = alloca (cif->rtype->size);

#ifdef POWERPC64
  sffi_call_LINUX64 (&ecif, fn, ecif.rvalue, cif->flags, closure,
		    -(long) cif->bytes);
#else
  sffi_call_SYSV (&ecif, fn, ecif.rvalue, cif->flags, closure, -cif->bytes);
#endif

  if (rvalue && ecif.rvalue == smst_buffer)
    {
      unsigned int rsize = cif->rtype->size;
#ifdef SFFI_TARGET_HAS_COMPLEX_TYPE

      if (cif->rtype->type == SFFI_TYPE_COMPLEX
	  && (cif->flags & (FLAG_RETURNS_FP | FLAG_RETURNS_VEC)) == 0)
	{
	  size_t hsize = cif->rtype->elements[0]->size;
#ifndef __LITTLE_ENDIAN__
	  size_t off = 8 - hsize;
#else
	  size_t off = 0;
#endif
	  memcpy ((char *) rvalue,         (char *) smst_buffer + off, hsize);
	  memcpy ((char *) rvalue + hsize, (char *) smst_buffer + 8 + off, hsize);
	}
      else
#endif
#ifndef __LITTLE_ENDIAN__

# ifndef POWERPC64
      if (rsize <= 4)
	memcpy (rvalue, (char *) smst_buffer + 4 - rsize, rsize);
      else
# endif

	if (rsize <= 8 && (cif->flags & FLAG_RETURNS_FP) == 0)
	  memcpy (rvalue, (char *) smst_buffer + 8 - rsize, rsize);
	else
#endif
	  memcpy (rvalue, smst_buffer, rsize);
    }
}

void
sffi_call (sffi_cif *cif, void (*fn) (void), void *rvalue, void **avalue)
{
  sffi_call_int (cif, fn, rvalue, avalue, NULL);
}

void
sffi_call_go (sffi_cif *cif, void (*fn) (void), void *rvalue, void **avalue,
	     void *closure)
{
  sffi_call_int (cif, fn, rvalue, avalue, closure);
}

sffi_status
sffi_prep_closure_loc (sffi_closure *closure,
		      sffi_cif *cif,
		      void (*fun) (sffi_cif *, void *, void **, void *),
		      void *user_data,
		      void *codeloc)
{
#ifdef POWERPC64
  return sffi_prep_closure_loc_linux64 (closure, cif, fun, user_data, codeloc);
#else
  return sffi_prep_closure_loc_sysv (closure, cif, fun, user_data, codeloc);
#endif
}

sffi_status
sffi_prep_go_closure (sffi_go_closure *closure,
		     sffi_cif *cif,
		     void (*fun) (sffi_cif *, void *, void **, void *))
{
#ifdef POWERPC64
  closure->tramp = sffi_go_closure_linux64;
#else
  closure->tramp = sffi_go_closure_sysv;
#endif
  closure->cif = cif;
  closure->fun = fun;
  return SFFI_OK;
}

#ifdef SFFI_EXEC_STATIC_TRAMP
void *
sffi_tramp_arch (size_t *tramp_size, size_t *map_size)
{
  extern void *trampoline_code_table;
  *tramp_size = PPC_TRAMP_SIZE;
  *map_size = PPC_TRAMP_MAP_SIZE;
#if defined (_CALL_AIX) || _CALL_ELF == 1

  return *(void **)trampoline_code_table;
#else
  return &trampoline_code_table;
#endif
}
#endif
