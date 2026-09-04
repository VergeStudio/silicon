#include "ffitest.h"

typedef struct Dbls {
	double x;
	double y;
} Dbls;

void
closure_test_fn(Dbls p)
{
	printf("%.1f %.1f\n", p.x, p.y);
	CHECK(p.x == 1);
	CHECK(p.y == 2);
}

void
closure_test_gn(sffi_cif* cif __UNUSED__, void* resp __UNUSED__,
		void** args, void* userdata __UNUSED__)
{
	closure_test_fn(*(Dbls*)args[0]);
}

int main(void)
{
	sffi_cif cif;

        void *code;
	sffi_closure*	pcl = sffi_closure_alloc(sizeof(sffi_closure), &code);
	sffi_type*		cl_arg_types[1];

	sffi_type	ts1_type;
	sffi_type*	ts1_type_elements[4];

	Dbls arg = { 1.0, 2.0 };

	ts1_type.size = 0;
	ts1_type.alignment = 0;
	ts1_type.type = SFFI_TYPE_STRUCT;
	ts1_type.elements = ts1_type_elements;

	ts1_type_elements[0] = &sffi_type_double;
	ts1_type_elements[1] = &sffi_type_double;
	ts1_type_elements[2] = NULL;

	cl_arg_types[0] = &ts1_type;

	CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 1,
				 &sffi_type_void, cl_arg_types) == SFFI_OK);

	CHECK(sffi_prep_closure_loc(pcl, &cif, closure_test_gn, NULL, code) == SFFI_OK);

	((void (*)(Dbls))(code))(arg);

	closure_test_fn(arg);

	return 0;
}
