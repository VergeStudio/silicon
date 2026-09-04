



#include "ffitest.h"

void ABI_ATTR
closure_test_fn(sffi_cif *cif __UNUSED__, void *resp __UNUSED__, void **args __UNUSED__, void *userdata __UNUSED__) {
    throw 9;
}

typedef void (*closure_test_type)();

void closure_test_fn1(sffi_cif *cif __UNUSED__, void *resp, void **args, void *userdata __UNUSED__) {
    *(sffi_arg *)resp =
            (int)*(float *)args[0] + (int)(*(float *)args[1]) +
            (int)(*(float *)args[2]) + (int)*(float *)args[3] +
            (int)(*(signed short *)args[4]) + (int)(*(float *)args[5]) +
            (int)*(float *)args[6] + (int)(*(int *)args[7]) +
            (int)(*(double *)args[8]) + (int)*(int *)args[9] +
            (int)(*(int *)args[10]) + (int)(*(float *)args[11]) +
            (int)*(int *)args[12] + (int)(*(int *)args[13]) +
            (int)(*(int *)args[14]) + *(int *)args[15] + (int)(intptr_t)userdata;

    printf("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d: %d\n",
           (int)*(float *)args[0], (int)(*(float *)args[1]),
           (int)(*(float *)args[2]), (int)*(float *)args[3],
           (int)(*(signed short *)args[4]), (int)(*(float *)args[5]),
           (int)*(float *)args[6], (int)(*(int *)args[7]),
           (int)(*(double *)args[8]), (int)*(int *)args[9],
           (int)(*(int *)args[10]), (int)(*(float *)args[11]),
           (int)*(int *)args[12], (int)(*(int *)args[13]),
           (int)(*(int *)args[14]), *(int *)args[15],
           (int)(intptr_t)userdata, (int)*(sffi_arg *)resp);

    throw (int)*(sffi_arg *)resp;
}

typedef int (*closure_test_type1)(float, float, float, float, signed short, float, float, int, double, int, int, float, int, int, int, int);

#ifdef __EMSCRIPTEN__
extern "C"
#endif
        int main(void) {
    sffi_cif cif;
    void *code;
    sffi_closure *pcl = (sffi_closure *)sffi_closure_alloc(sizeof(sffi_closure), &code);
    sffi_type *cl_arg_types[17];

    {
        cl_arg_types[1] = NULL;

        CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 0, &sffi_type_void, cl_arg_types) == SFFI_OK);
        CHECK(sffi_prep_closure_loc(pcl, &cif, closure_test_fn, NULL, code) == SFFI_OK);

        try {
            (*((closure_test_type)(code)))();
        } catch(int exception_code) {
            CHECK(exception_code == 9);
        }

        printf("part one OK\n");
        
    }

    {

        cl_arg_types[0] = &sffi_type_float;
        cl_arg_types[1] = &sffi_type_float;
        cl_arg_types[2] = &sffi_type_float;
        cl_arg_types[3] = &sffi_type_float;
        cl_arg_types[4] = &sffi_type_sshort;
        cl_arg_types[5] = &sffi_type_float;
        cl_arg_types[6] = &sffi_type_float;
        cl_arg_types[7] = &sffi_type_uint;
        cl_arg_types[8] = &sffi_type_double;
        cl_arg_types[9] = &sffi_type_uint;
        cl_arg_types[10] = &sffi_type_uint;
        cl_arg_types[11] = &sffi_type_float;
        cl_arg_types[12] = &sffi_type_uint;
        cl_arg_types[13] = &sffi_type_uint;
        cl_arg_types[14] = &sffi_type_uint;
        cl_arg_types[15] = &sffi_type_uint;
        cl_arg_types[16] = NULL;

        
        CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 16, &sffi_type_sint, cl_arg_types) == SFFI_OK);

        CHECK(sffi_prep_closure_loc(pcl, &cif, closure_test_fn1, (void *)3 , code) == SFFI_OK);
        try {
            (*((closure_test_type1)code))(1.1, 2.2, 3.3, 4.4, 127, 5.5, 6.6, 8, 9, 10, 11, 12.0, 13, 19, 21, 1);
            
        } catch(int exception_code) {
            CHECK(exception_code == 255);
        }
        printf("part two OK\n");
        
    }
    exit(0);
}
