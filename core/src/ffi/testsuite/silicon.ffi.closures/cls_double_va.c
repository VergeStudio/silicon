#include "ffitest.h"

#define BUF_SIZE 50
static char buffer[BUF_SIZE];

static void
cls_double_va_fn(sffi_cif* cif __UNUSED__, void* resp,
		 void** args, void* userdata __UNUSED__)
{
	char*	format		= *(char**)args[0];
	double	doubleValue	= *(double*)args[1];

	*(sffi_arg*)resp = printf(format, doubleValue);
	CHECK(*(sffi_arg*)resp == 4);
	snprintf(buffer, BUF_SIZE, format, doubleValue);
	CHECK(strncmp(buffer, "7.0\n", 4) == 0);
}

int main (void)
{
	sffi_cif cif;
        void *code;
	sffi_closure *pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
	void* args[3];
	sffi_type* arg_types[3];

	char*	format		= "%.1f\n";
	double	doubleArg	= 7;
	sffi_arg	res			= 0;

	arg_types[0] = &sffi_type_pointer;
	arg_types[1] = &sffi_type_double;
	arg_types[2] = NULL;

	CHECK(sffi_prep_cif_var(&cif, SFFI_DEFAULT_ABI, 1, 2, &sffi_type_sint,
			       arg_types) == SFFI_OK);

	args[0] = &format;
	args[1] = &doubleArg;
	args[2] = NULL;

	sffi_call(&cif, SFFI_FN(printf), &res, args);

	printf("res: %d\n", (int) res);

	CHECK(res == 4);

	CHECK(sffi_prep_closure_loc(pcl, &cif, cls_double_va_fn, NULL,
				   code) == SFFI_OK);

	res = ((int(*)(char*, ...))(code))(format, doubleArg);

	printf("res: %d\n", (int) res);

	CHECK(res == 4);

	exit(0);
}
