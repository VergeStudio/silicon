#define PPC_LD_NONE 0
#define PPC_LD_R3 1
#define PPC_LD_R3R4 2
#define PPC_LD_F32 3
#define PPC_LD_F64 4
#define PPC_LD_F128 5
#define PPC_LD_U8 6
#define PPC_LD_S8 7
#define PPC_LD_U16 8
#define PPC_LD_S16 9

#ifndef POWERPC64

#    define PPC_LD_U32 PPC_LD_R3
#    define PPC_LD_S32 PPC_LD_R3
#    define PPC_LD_PTR PPC_LD_R3
#    define PPC_LD_I64 PPC_LD_R3R4

#    define PPC32_LD_R3R6 10

#    ifdef __LITTLE_ENDIAN__
#        define PPC32_SYSV_LD_STRUCT_3 PPC32_LD_R3
#        define PPC32_SYSV_LD_STRUCT_5 PPC32_LD_R3R4
#        define PPC32_SYSV_LD_STRUCT_6 PPC32_LD_R3R4
#        define PPC32_SYSV_LD_STRUCT_7 PPC32_LD_R3R4
#    else
#        define PPC32_SYSV_LD_STRUCT_3 11
#        define PPC32_SYSV_LD_STRUCT_5 12
#        define PPC32_SYSV_LD_STRUCT_6 13
#        define PPC32_SYSV_LD_STRUCT_7 14
#    endif

#else 

#    define PPC_LD_U32 10
#    define PPC_LD_S32 11
#    define PPC_LD_PTR PPC_LD_R3
#    define PPC_LD_I64 PPC_LD_R3

#    define PPC64_LD_VECTOR 12
#    define PPC64_LD_VECTOR_HOMOG 13
#    define PPC64_LD_FLOAT_HOMOG 14
#    define PPC64_LD_DOUBLE_HOMOG 15
#    ifdef __LITTLE_ENDIAN__
#        define PPC64_LD_STRUCT_3 PPC_LD_U32
#        define PPC64_LD_STRUCT_5 PPC_LD_I64
#        define PPC64_LD_STRUCT_6 PPC_LD_I64
#        define PPC64_LD_STRUCT_7 PPC_LD_I64
#    else
#        define PPC64_LD_STRUCT_3 16
#        define PPC64_LD_STRUCT_5 17
#        define PPC64_LD_STRUCT_6 18
#        define PPC64_LD_STRUCT_7 19
#    endif

#endif 

#ifndef SILICON_FFI_ASM

enum {

    FLAG_RETURNS_SMST = 1 << (31 - 31), 
    FLAG_RETURNS_NOTHING = 1 << (31 - 30),
    FLAG_RETURNS_FP = 1 << (31 - 29),
    FLAG_RETURNS_VEC = 1 << (31 - 28),

    FLAG_RETURNS_64BITS = 1 << (31 - 27),
    FLAG_RETURNS_128BITS = 1 << (31 - 26),

    FLAG_COMPAT = 1 << (31 - 8), 

    FLAG_ARG_NEEDS_COPY = 1 << (31 - 7),        
    FLAG_ARG_NEEDS_PSAVE = FLAG_ARG_NEEDS_COPY, 
    FLAG_FP_ARGUMENTS = 1 << (31 - 6),          
    FLAG_4_GPR_ARGUMENTS = 1 << (31 - 5),
    FLAG_RETVAL_REFERENCE = 1 << (31 - 4),
    FLAG_VEC_ARGUMENTS = 1 << (31 - 3),
};

typedef union {
    float f;
    double d;
} sffi_dblfl;

#    if defined(__FLOAT128_TYPE__) && defined(__HAVE_FLOAT128)
typedef _Float128 float128;
#    elif defined(__FLOAT128__)
typedef __float128 float128;
#    else
typedef char float128[16] __attribute__((aligned(16)));
#    endif

void SFFI_HIDDEN sffi_closure_SYSV(void);
void SFFI_HIDDEN sffi_go_closure_sysv(void);
void SFFI_HIDDEN sffi_call_SYSV(extended_cif *, void (*)(void), void *, unsigned, void *, int);

void SFFI_HIDDEN sffi_prep_types_sysv(sffi_abi);
sffi_status SFFI_HIDDEN sffi_prep_cif_sysv(sffi_cif *);
sffi_status SFFI_HIDDEN sffi_prep_closure_loc_sysv(sffi_closure *, sffi_cif *, void (*)(sffi_cif *, void *, void **, void *), void *, void *);
int SFFI_HIDDEN sffi_closure_helper_SYSV(sffi_cif *, void (*)(sffi_cif *, void *, void **, void *), void *, void *, unsigned long *, sffi_dblfl *, unsigned long *);

void SFFI_HIDDEN sffi_call_LINUX64(extended_cif *, void (*)(void), void *, unsigned long, void *, long);
void SFFI_HIDDEN sffi_closure_LINUX64(void);
void SFFI_HIDDEN sffi_go_closure_linux64(void);

void SFFI_HIDDEN sffi_prep_types_linux64(sffi_abi);
sffi_status SFFI_HIDDEN sffi_prep_cif_linux64(sffi_cif *);
sffi_status SFFI_HIDDEN sffi_prep_cif_linux64_var(sffi_cif *, unsigned int, unsigned int);
void SFFI_HIDDEN sffi_prep_args64(extended_cif *, unsigned long *const);
sffi_status SFFI_HIDDEN sffi_prep_closure_loc_linux64(sffi_closure *, sffi_cif *, void (*)(sffi_cif *, void *, void **, void *), void *, void *);
int SFFI_HIDDEN sffi_closure_helper_LINUX64(sffi_cif *, void (*)(sffi_cif *, void *, void **, void *), void *, void *, unsigned long *, sffi_dblfl *, float128 *);

#endif 
