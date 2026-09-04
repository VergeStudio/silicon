



#include "ffitest.h"

static int ABI_ATTR fn(int *a)
{
	if (a)
		return *a;
	return -1;
}

int main(void)
{
	sffi_cif cif;
	sffi_type *args[MAX_ARGS];
	void *values[MAX_ARGS];
	void *z[2] = { (void *)0, (void *)1 };
	sffi_arg rint;
	args[0] = &sffi_type_pointer;
	values[0] = z;
	CHECK(sffi_prep_cif(&cif, ABI_NUM, 1, &sffi_type_sint, args) == SFFI_OK);
	sffi_call(&cif, SFFI_FN(fn), &rint, values);
	CHECK((int)rint == -1);
	exit(0);
}
