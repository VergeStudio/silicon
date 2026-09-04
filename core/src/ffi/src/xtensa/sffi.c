

#include <sffi.h>
#include <sffi_common.h>




#define SFFI_TYPE_STRUCT_REGS SFFI_TYPE_LAST


extern void sffi_call_SYSV(void *rvalue, unsigned rsize, unsigned flags,
			  void(*fn)(void), unsigned nbytes, extended_cif*);
extern void sffi_closure_SYSV(void) SFFI_HIDDEN;

sffi_status sffi_prep_cif_machdep(sffi_cif *cif)
{
  switch(cif->rtype->type) {
    case SFFI_TYPE_SINT8:
    case SFFI_TYPE_UINT8:
    case SFFI_TYPE_SINT16:
    case SFFI_TYPE_UINT16:
      cif->flags = cif->rtype->type;
      break;
    case SFFI_TYPE_VOID:
    case SFFI_TYPE_FLOAT:
      cif->flags = SFFI_TYPE_UINT32;
      break;
    case SFFI_TYPE_DOUBLE:
    case SFFI_TYPE_UINT64:
    case SFFI_TYPE_SINT64:
      cif->flags = SFFI_TYPE_UINT64;
      break;
    case SFFI_TYPE_STRUCT:
      cif->flags = SFFI_TYPE_STRUCT;
      
      if (cif->rtype->size > 4 * 4) {
        
        cif->flags = SFFI_TYPE_STRUCT;	
        cif->bytes += 8;
      }
      break;

    default:
      cif->flags = SFFI_TYPE_UINT32;
      break;
  }

  
  if (cif->bytes < SFFI_REGISTER_NARGS * 4)
    cif->bytes = SFFI_REGISTER_ARGS_SPACE;
  else
    cif->bytes = SFFI_REGISTER_ARGS_SPACE +
	    SFFI_ALIGN(cif->bytes - SFFI_REGISTER_NARGS * 4,
		      XTENSA_STACK_ALIGNMENT);
  return SFFI_OK;
}

void sffi_prep_args(extended_cif *ecif, unsigned char* stack)
{
  unsigned int i;
  unsigned long *addr;
  sffi_type **ptr;

  union {
    void **v;
    char **c;
    signed char **sc;
    unsigned char **uc;
    signed short **ss;
    unsigned short **us;
    unsigned int **i;
    long long **ll;
    float **f;
    double **d;
  } p_argv;

  
  SFFI_ASSERT (((unsigned long) stack & 0x7) == 0);

  p_argv.v = ecif->avalue;
  addr = (unsigned long*)stack;

  
  if (ecif->cif->rtype->type == SFFI_TYPE_STRUCT && ecif->cif->rtype->size > 16)
  {
    *addr++ = (unsigned long)ecif->rvalue;
  }

  for (i = ecif->cif->nargs, ptr = ecif->cif->arg_types;
       i > 0;
       i--, ptr++, p_argv.v++)
  {
    switch ((*ptr)->type)
    {
      case SFFI_TYPE_SINT8:
        *addr++ = **p_argv.sc;
        break;
      case SFFI_TYPE_UINT8:
        *addr++ = **p_argv.uc;
        break;
      case SFFI_TYPE_SINT16:
        *addr++ = **p_argv.ss;
        break;
      case SFFI_TYPE_UINT16:
        *addr++ = **p_argv.us;
        break;
      case SFFI_TYPE_FLOAT:
      case SFFI_TYPE_INT:
      case SFFI_TYPE_UINT32:
      case SFFI_TYPE_SINT32:
      case SFFI_TYPE_POINTER:
        *addr++ = **p_argv.i;
        break;
      case SFFI_TYPE_DOUBLE:
      case SFFI_TYPE_UINT64:
      case SFFI_TYPE_SINT64:
        if (((unsigned long)addr & 4) != 0)
          addr++;
        *(unsigned long long*)addr = **p_argv.ll;
	addr += sizeof(unsigned long long) / sizeof (addr);
        break;

      case SFFI_TYPE_STRUCT:
      {
        unsigned long offs;
        unsigned long size;

        if (((unsigned long)addr & 4) != 0 && (*ptr)->alignment > 4)
          addr++;

        offs = (unsigned long) addr - (unsigned long) stack;
        size = (*ptr)->size;

        
        if (offs < SFFI_REGISTER_NARGS * 4
            && offs + size > SFFI_REGISTER_NARGS * 4)
          addr = (unsigned long*) (stack + SFFI_REGISTER_NARGS * 4);

        memcpy((char*) addr, *p_argv.c, size);
        addr += (size + 3) / 4;
        break;
      }

      default:
        SFFI_ASSERT(0);
    }
  }
}


void sffi_call(sffi_cif* cif, void(*fn)(void), void *rvalue, void **avalue)
{
  extended_cif ecif;
  unsigned long rsize = cif->rtype->size;
  int flags = cif->flags;
  void *alloc = NULL;

  ecif.cif = cif;
  ecif.avalue = avalue;

  

  if (flags == SFFI_TYPE_STRUCT && (rsize <= 16 || rvalue == NULL))
  {
    alloc = alloca(SFFI_ALIGN(rsize, 4));
    ecif.rvalue = alloc;
  }
  else
  {
    ecif.rvalue = rvalue;
  }

  if (cif->abi != SFFI_SYSV)
    SFFI_ASSERT(0);

  sffi_call_SYSV (ecif.rvalue, rsize, cif->flags, fn, cif->bytes, &ecif);

  if (alloc != NULL && rvalue != NULL)
    memcpy(rvalue, alloc, rsize);
}

extern void sffi_trampoline();
extern void sffi_cacheflush(void* start, void* end);

sffi_status
sffi_prep_closure_loc (sffi_closure* closure,
                      sffi_cif* cif,
                      void (*fun)(sffi_cif*, void*, void**, void*),
                      void *user_data,
                      void *codeloc)
{
  if (cif->abi != SFFI_SYSV)
    return SFFI_BAD_ABI;

  
  memcpy(closure->tramp, sffi_trampoline, SFFI_TRAMPOLINE_SIZE);
  *(unsigned int*)(&closure->tramp[8]) = (unsigned int)sffi_closure_SYSV;



  sffi_cacheflush(closure->tramp, closure->tramp + SFFI_TRAMPOLINE_SIZE);

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;
  return SFFI_OK; 
}


long SFFI_HIDDEN
sffi_closure_SYSV_inner(sffi_closure *closure, void **values, void *rvalue)
{
  sffi_cif *cif;
  sffi_type **arg_types;
  void **avalue;
  int i, areg;

  cif = closure->cif;
  if (cif->abi != SFFI_SYSV)
    return SFFI_BAD_ABI;

  areg = 0;

  int rtype = cif->rtype->type;
  if (rtype == SFFI_TYPE_STRUCT && cif->rtype->size > 4 * 4)
  {
    rvalue = *values;
    areg++;
  }

  cif = closure->cif; 
  arg_types = cif->arg_types;
  avalue = alloca(cif->nargs * sizeof(void *));

  for (i = 0; i < cif->nargs; i++)
  {
    if (arg_types[i]->alignment == 8 && (areg & 1) != 0)
      areg++;


    if (areg == SFFI_REGISTER_NARGS)
      areg = (SFFI_REGISTER_ARGS_SPACE + 32) / 4;

    if (arg_types[i]->type == SFFI_TYPE_STRUCT)
    {
      int numregs = ((arg_types[i]->size + 3) & ~3) / 4;
      if (areg < SFFI_REGISTER_NARGS && areg + numregs > SFFI_REGISTER_NARGS)
        areg = (SFFI_REGISTER_ARGS_SPACE + 32) / 4;
    }

    avalue[i] = &values[areg];
    areg += (arg_types[i]->size + 3) / 4;
  }

  (closure->fun)(cif, rvalue, avalue, closure->user_data);

  return rtype;
}
