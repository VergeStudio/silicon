


#include "ffitest.h"
#include <float.h>
#include <math.h>

#define NARGS 16

static void cls_mixed_float_double_fn(sffi_cif* cif , void* ret, void** args,
			      void* userdata __UNUSED__)
{
    double r = 0;
    unsigned int i;
    double t;
    for(i=0; i < cif->nargs; i++)
    {
        if(cif->arg_types[i] == &sffi_type_double) {
				t = *(((double**)(args))[i]);
        } else {
				t = *(((float**)(args))[i]);
        }
        r += t;
    }
    *((double*)ret) = r;
}
typedef double (*cls_mixed)(double, float, double, double, double, double, double, float, float, double, float, float);

int main (void)
{
    sffi_cif cif;
    sffi_closure *closure;
	void* code;
    sffi_type *argtypes[12] = {&sffi_type_double, &sffi_type_float, &sffi_type_double,
                          &sffi_type_double, &sffi_type_double, &sffi_type_double,
                          &sffi_type_double, &sffi_type_float, &sffi_type_float,
                          &sffi_type_double, &sffi_type_float, &sffi_type_float};


    closure = sffi_closure_alloc(sizeof(sffi_closure), (void**)&code);
    if(closure ==NULL)
		abort();
    CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 12, &sffi_type_double, argtypes) == SFFI_OK);
	CHECK(sffi_prep_closure_loc(closure, &cif, cls_mixed_float_double_fn, NULL,  code) == SFFI_OK);
    double ret = ((cls_mixed)code)(0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2);
    sffi_closure_free(closure);
	if(fabs(ret - 7.8) < FLT_EPSILON)
		exit(0);
	else
		abort();
}
