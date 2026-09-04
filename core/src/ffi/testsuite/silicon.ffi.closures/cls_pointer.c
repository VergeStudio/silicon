#include "ffitest.h"

void* cls_pointer_fn(void* a1, void* a2)
{
	void*	result	= (void*)((intptr_t)a1 + (intptr_t)a2);

	printf("0x%08x 0x%08x: 0x%08x\n", 
	       (unsigned int)(uintptr_t) a1,
               (unsigned int)(uintptr_t) a2,
               (unsigned int)(uintptr_t) result);

	CHECK((unsigned int)(uintptr_t) a1 == 0x12345678);
	CHECK((unsigned int)(uintptr_t) a2 == 0x89abcdef);
	CHECK((unsigned int)(uintptr_t) result == 0x9be02467);

	return result;
}

static void
cls_pointer_gn(sffi_cif* cif __UNUSED__, void* resp, 
	       void** args, void* userdata __UNUSED__)
{
	void*	a1	= *(void**)(args[0]);
	void*	a2	= *(void**)(args[1]);

	*(void**)resp = cls_pointer_fn(a1, a2);
}

int main (void)
{
	sffi_cif	cif;
        void *code;
	sffi_closure*	pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
	void*			args[3];

	sffi_type*		arg_types[3];

	void*	arg1	= (void*)0x12345678;
	void*	arg2	= (void*)0x89abcdef;
	sffi_arg	res		= 0;

	arg_types[0] = &sffi_type_pointer;
	arg_types[1] = &sffi_type_pointer;
	arg_types[2] = NULL;

	CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 2, &sffi_type_pointer,
		arg_types) == SFFI_OK);

	args[0] = &arg1;
	args[1] = &arg2;
	args[2] = NULL;

	sffi_call(&cif, SFFI_FN(cls_pointer_fn), &res, args);

	printf("res: 0x%08x\n", (unsigned int) res);

	CHECK(sffi_prep_closure_loc(pcl, &cif, cls_pointer_gn, NULL, code) == SFFI_OK);

	res = (sffi_arg)(uintptr_t)((void*(*)(void*, void*))(code))(arg1, arg2);

	printf("res: 0x%08x\n", (unsigned int) res);

	CHECK(res == 0x9be02467);

	exit(0);
}
