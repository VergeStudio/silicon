



#include "ffitest.h"

static void
dummy_fn(sffi_cif* cif __UNUSED__, void* resp __UNUSED__, 
	 void** args __UNUSED__, void* userdata __UNUSED__)
{}

int main (void)
{
	sffi_cif cif;
        void *code;
	sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
	sffi_type* arg_types[1];

	arg_types[0] = NULL;

	CHECK(sffi_prep_cif(&cif, 255, 0, &sffi_type_void,
		arg_types) == SFFI_BAD_ABI);

	CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 0, &sffi_type_void,
		arg_types) == SFFI_OK);

	cif.abi= 255;

	CHECK(sffi_prep_closure_loc(pcl, &cif, dummy_fn, NULL, code) == SFFI_BAD_ABI);

	exit(0);
}
