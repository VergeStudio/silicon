

#include <sffi.h>
#include <sffi_common.h>

#include <stdlib.h>

#define MIN(a,b) (((a) < (b)) ? (a) : (b))



unsigned int sffi_prep_args(char *stack, extended_cif *ecif)
{
	register unsigned int i;
	register void **p_argv;
	register char *argp;
	register sffi_type **p_arg;

	argp = stack;

	
	if ( ecif->cif->flags == SFFI_TYPE_STRUCT ) {
		argp -= 4;
		*(void **) argp = ecif->rvalue;
	}

	p_argv = ecif->avalue;

	
	for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types; (i != 0); i--, p_arg++, p_argv++)
	{
		size_t z;

		
		z = (*p_arg)->size;
		argp -= z;

		
		argp = (char *) SFFI_ALIGN_DOWN(SFFI_ALIGN_DOWN(argp, (*p_arg)->alignment), 4);

		if (z < sizeof(int)) {
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
			case SFFI_TYPE_STRUCT:
				memcpy(argp, *p_argv, (*p_arg)->size);
				break;
			default:
				SFFI_ASSERT(0);
			}
		} else if ( z == sizeof(int)) {
			*(unsigned int *) argp = (unsigned int)*(UINT32 *)(* p_argv);
		} else {
			memcpy(argp, *p_argv, z);
		}
	}

	
	return SFFI_ALIGN(MIN(stack - argp, 6*4), 8);
}


sffi_status sffi_prep_cif_machdep(sffi_cif *cif)
{
	sffi_type **ptr;
	unsigned i, bytes = 0;

	for (ptr = cif->arg_types, i = cif->nargs; i > 0; i--, ptr++) {
		if ((*ptr)->size == 0)
			return SFFI_BAD_TYPEDEF;

		
		SFFI_ASSERT_VALID_TYPE(*ptr);

		
		if (((*ptr)->alignment - 1) & bytes)
			bytes = SFFI_ALIGN(bytes, (*ptr)->alignment);

		bytes += SFFI_ALIGN((*ptr)->size, 4);
	}

	
	bytes = SFFI_ALIGN(bytes, 8);

	
	if (cif->rtype->type == SFFI_TYPE_STRUCT) {
		bytes += sizeof(void*);

		
		bytes = SFFI_ALIGN(bytes, 8);
	}

	cif->bytes = bytes;

	
	switch (cif->rtype->type) {
	case SFFI_TYPE_VOID:
	case SFFI_TYPE_FLOAT:
	case SFFI_TYPE_DOUBLE:
		cif->flags = (unsigned) cif->rtype->type;
		break;
	case SFFI_TYPE_SINT64:
	case SFFI_TYPE_UINT64:
		cif->flags = (unsigned) SFFI_TYPE_SINT64;
		break;
	case SFFI_TYPE_STRUCT:
		
		if (cif->rtype->size <= 4)
			
			cif->flags = (unsigned)SFFI_TYPE_INT;
		else if ((cif->rtype->size > 4) && (cif->rtype->size <= 8))
			
			cif->flags = (unsigned)SFFI_TYPE_DOUBLE;
		else
			
			cif->flags = (unsigned)SFFI_TYPE_STRUCT;
		break;
	default:
		cif->flags = (unsigned)SFFI_TYPE_INT;
		break;
	}
	return SFFI_OK;
}

extern void sffi_call_SYSV(void (*fn)(void), extended_cif *, unsigned, unsigned, double *);


void sffi_call(sffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
	extended_cif ecif;

	int small_struct = (((cif->flags == SFFI_TYPE_INT) || (cif->flags == SFFI_TYPE_DOUBLE)) && (cif->rtype->type == SFFI_TYPE_STRUCT));
	ecif.cif = cif;
	ecif.avalue = avalue;

	double temp;

	

	if ((rvalue == NULL ) && (cif->flags == SFFI_TYPE_STRUCT))
		ecif.rvalue = alloca(cif->rtype->size);
	else if (small_struct)
		ecif.rvalue = &temp;
	else
		ecif.rvalue = rvalue;

	switch (cif->abi) {
	case SFFI_SYSV:
		sffi_call_SYSV(fn, &ecif, cif->bytes, cif->flags, ecif.rvalue);
		break;
	default:
		SFFI_ASSERT(0);
		break;
	}

	if (small_struct)
		memcpy (rvalue, &temp, cif->rtype->size);
}



static void sffi_prep_incoming_args_SYSV (char *, void **, void **,
	sffi_cif*, float *);

void sffi_closure_SYSV (sffi_closure *);


extern unsigned int sffi_metag_trampoline[10]; 




void sffi_init_trampoline(unsigned char *__tramp, unsigned int __fun, unsigned int __ctx) {
	memcpy (__tramp, sffi_metag_trampoline, sizeof(sffi_metag_trampoline));
	*(unsigned int*) &__tramp[40] = __ctx;
	*(unsigned int*) &__tramp[44] = __fun;
	
	__builtin_meta2_cachewd(&__tramp[0], 1);
	__builtin_meta2_cachewd(&__tramp[47], 1);
}





sffi_status
sffi_prep_closure_loc (sffi_closure *closure,
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

	sffi_init_trampoline(
		(unsigned char*)&closure->tramp[0],
		(unsigned int)closure_func,
		(unsigned int)codeloc);

	closure->cif = cif;
	closure->user_data = user_data;
	closure->fun = fun;

	return SFFI_OK;
}



unsigned int sffi_closure_SYSV_inner (closure, respp, args, vfp_args)
	sffi_closure *closure;
	void **respp;
	void *args;
	void *vfp_args;
{
	sffi_cif *cif;
	void **arg_area;

	cif = closure->cif;
	arg_area = (void**) alloca (cif->nargs * sizeof (void*));

	
	sffi_prep_incoming_args_SYSV(args, respp, arg_area, cif, vfp_args);

	(closure->fun) ( cif, *respp, arg_area, closure->user_data);

	return cif->flags;
}

static void sffi_prep_incoming_args_SYSV(char *stack, void **rvalue,
	void **avalue, sffi_cif *cif,
	float *vfp_stack)
{
	register unsigned int i;
	register void **p_argv;
	register char *argp;
	register sffi_type **p_arg;

	
	argp = stack;

	
	if ( cif->flags == SFFI_TYPE_STRUCT ) {
		argp -= 4;
		*rvalue = *(void **) argp;
	}

	p_argv = avalue;

	for (i = cif->nargs, p_arg = cif->arg_types; (i != 0); i--, p_arg++) {
		size_t z;
		size_t alignment;

		alignment = (*p_arg)->alignment;
		if (alignment < 4)
			alignment = 4;
		if ((alignment - 1) & (unsigned)argp)
			argp = (char *) SFFI_ALIGN(argp, alignment);

		z = (*p_arg)->size;
		*p_argv = (void*) argp;
		p_argv++;
		argp -= z;
	}
	return;
}
