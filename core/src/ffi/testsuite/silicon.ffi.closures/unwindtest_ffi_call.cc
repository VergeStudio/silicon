



#include "ffitest.h"

static int checking(int a __UNUSED__, short b __UNUSED__, signed char c __UNUSED__) {
    throw 9;
}

#ifdef __EMSCRIPTEN__
extern "C"
#endif
        int main(void) {
    sffi_cif cif;
    sffi_type *args[MAX_ARGS];
    void *values[MAX_ARGS];
    sffi_arg rint;

    signed int si;
    signed short ss;
    signed char sc;

    args[0] = &sffi_type_sint;
    values[0] = &si;
    args[1] = &sffi_type_sshort;
    values[1] = &ss;
    args[2] = &sffi_type_schar;
    values[2] = &sc;

    
    CHECK(sffi_prep_cif(&cif, SFFI_DEFAULT_ABI, 3, &sffi_type_sint, args) == SFFI_OK);

    si = -6;
    ss = -12;
    sc = -1;
    {
        try {
            sffi_call(&cif, SFFI_FN(checking), &rint, values);
        } catch(int exception_code) {
            CHECK(exception_code == 9);
        }
        printf("part one OK\n");
        
    }
    exit(0);
}
