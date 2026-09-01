## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = foreign dejagnu

EXTRA_DEJAGNU_SITE_CONFIG=../local.exp

CLEANFILES = *.exe core* *.log *.sum

EXTRA_DIST = config/default.exp emscripten/build.sh emscripten/conftest.py \
	emscripten/node-tests.sh emscripten/test.html emscripten/test_sffi.py \
	emscripten/build-tests.sh lib/SILICON_FFI.exp lib/target-libpath.exp \
	lib/wrapper.exp SILICON_FFI.bhaible/Makefile SILICON_FFI.bhaible/README \
	SILICON_FFI.bhaible/alignof.h SILICON_FFI.bhaible/bhaible.exp SILICON_FFI.bhaible/test-call.c \
	SILICON_FFI.bhaible/test-callback.c SILICON_FFI.bhaible/testcases.c SILICON_FFI.call/align_mixed.c \
	SILICON_FFI.call/align_stdcall.c SILICON_FFI.call/bpo_38748.c SILICON_FFI.call/call.exp \
	SILICON_FFI.call/closure_thiscall_fastcall_pop.c \
	SILICON_FFI.call/err_bad_typedef.c SILICON_FFI.call/ffitest.h SILICON_FFI.call/float.c \
	SILICON_FFI.call/float1.c SILICON_FFI.call/float2.c SILICON_FFI.call/float3.c \
	SILICON_FFI.call/float4.c SILICON_FFI.call/float_va.c SILICON_FFI.call/i128-1.c \
	SILICON_FFI.call/large_struct_by_value.c SILICON_FFI.call/many.c \
	SILICON_FFI.call/many2.c SILICON_FFI.call/many_double.c \
	SILICON_FFI.call/many_large_structs.c SILICON_FFI.call/many_mixed.c \
	SILICON_FFI.call/many_small_structs.c \
	SILICON_FFI.call/negint.c SILICON_FFI.call/offsets.c SILICON_FFI.call/overread.c \
	SILICON_FFI.call/plan.c SILICON_FFI.call/plan_mixed.c SILICON_FFI.call/plan_spill.c \
	SILICON_FFI.call/plan_struct.c SILICON_FFI.call/plan_var.c \
	SILICON_FFI.call/pr1172638.c SILICON_FFI.call/promotion.c SILICON_FFI.call/pyobjc_tc.c SILICON_FFI.call/return_dbl.c \
	SILICON_FFI.call/return_dbl1.c SILICON_FFI.call/return_dbl2.c SILICON_FFI.call/return_fl.c \
	SILICON_FFI.call/return_fl1.c SILICON_FFI.call/return_fl2.c SILICON_FFI.call/return_fl3.c \
	SILICON_FFI.call/return_ldl.c SILICON_FFI.call/return_ll.c SILICON_FFI.call/return_ll1.c \
	SILICON_FFI.call/return_sc.c SILICON_FFI.call/return_sl.c SILICON_FFI.call/return_uc.c \
	SILICON_FFI.call/return_ul.c SILICON_FFI.call/s55.c SILICON_FFI.call/strlen.c \
	SILICON_FFI.call/strlen2.c SILICON_FFI.call/strlen3.c SILICON_FFI.call/strlen4.c \
	SILICON_FFI.call/struct1.c SILICON_FFI.call/struct10.c SILICON_FFI.call/struct2.c \
	SILICON_FFI.call/struct3.c SILICON_FFI.call/struct4.c SILICON_FFI.call/struct5.c \
	SILICON_FFI.call/struct6.c SILICON_FFI.call/struct7.c SILICON_FFI.call/struct8.c \
	SILICON_FFI.call/struct9.c SILICON_FFI.call/struct_by_value_2.c SILICON_FFI.call/struct_by_value_3.c \
	SILICON_FFI.call/struct_by_value_3f.c SILICON_FFI.call/struct_by_value_4.c SILICON_FFI.call/struct_by_value_4f.c \
	SILICON_FFI.call/struct_by_value_big.c SILICON_FFI.call/struct_by_value_small.c SILICON_FFI.call/struct_return_2H.c \
	SILICON_FFI.call/struct_int_float.c SILICON_FFI.call/longjmp.c \
	SILICON_FFI.call/struct_return_8H.c SILICON_FFI.call/uninitialized.c SILICON_FFI.call/va_1.c \
	SILICON_FFI.call/va_2.c SILICON_FFI.call/va_3.c SILICON_FFI.call/va_struct1.c \
	SILICON_FFI.call/va_struct2.c SILICON_FFI.call/va_struct3.c SILICON_FFI.call/callback.c \
	SILICON_FFI.call/callback2.c SILICON_FFI.call/callback3.c SILICON_FFI.call/callback4.c SILICON_FFI.call/x32.c \
	SILICON_FFI.closures/closure.exp SILICON_FFI.closures/closure_fn0.c SILICON_FFI.closures/closure_fn1.c \
	SILICON_FFI.closures/closure_fn2.c SILICON_FFI.closures/closure_fn3.c SILICON_FFI.closures/closure_fn4.c \
	SILICON_FFI.closures/closure_fn5.c SILICON_FFI.closures/closure_fn6.c SILICON_FFI.closures/closure_loc_fn0.c \
	SILICON_FFI.closures/closure_simple.c SILICON_FFI.closures/cls_12byte.c SILICON_FFI.closures/cls_16byte.c \
	SILICON_FFI.closures/cls_18byte.c SILICON_FFI.closures/cls_19byte.c SILICON_FFI.closures/cls_1_1byte.c \
	SILICON_FFI.closures/cls_20byte.c SILICON_FFI.closures/cls_20byte1.c SILICON_FFI.closures/cls_24byte.c \
	SILICON_FFI.closures/cls_2byte.c SILICON_FFI.closures/cls_3_1byte.c SILICON_FFI.closures/cls_3byte1.c \
	SILICON_FFI.closures/cls_3byte2.c SILICON_FFI.closures/cls_3float.c SILICON_FFI.closures/cls_4_1byte.c \
	SILICON_FFI.closures/cls_4byte.c SILICON_FFI.closures/cls_5_1_byte.c SILICON_FFI.closures/cls_5byte.c \
	SILICON_FFI.closures/cls_64byte.c SILICON_FFI.closures/cls_6_1_byte.c SILICON_FFI.closures/cls_6byte.c \
	SILICON_FFI.closures/cls_7_1_byte.c SILICON_FFI.closures/cls_7byte.c SILICON_FFI.closures/cls_8byte.c \
	SILICON_FFI.closures/cls_9byte1.c SILICON_FFI.closures/cls_9byte2.c SILICON_FFI.closures/cls_align_double.c \
	SILICON_FFI.closures/cls_align_float.c SILICON_FFI.closures/cls_align_longdouble.c SILICON_FFI.closures/cls_align_longdouble_split.c \
	SILICON_FFI.closures/cls_align_longdouble_split2.c SILICON_FFI.closures/cls_align_pointer.c SILICON_FFI.closures/cls_align_sint16.c \
	SILICON_FFI.closures/cls_align_sint32.c SILICON_FFI.closures/cls_align_sint64.c SILICON_FFI.closures/cls_align_uint16.c \
	SILICON_FFI.closures/cls_align_uint32.c SILICON_FFI.closures/cls_align_uint64.c SILICON_FFI.closures/cls_dbls_struct.c \
	SILICON_FFI.closures/cls_double.c SILICON_FFI.closures/cls_double_va.c SILICON_FFI.closures/cls_float.c \
	SILICON_FFI.closures/cls_longdouble.c SILICON_FFI.closures/cls_longdouble_va.c SILICON_FFI.closures/cls_many_mixed_args.c \
	SILICON_FFI.closures/cls_many_mixed_float_double.c SILICON_FFI.closures/cls_multi_schar.c SILICON_FFI.closures/cls_multi_sshort.c \
	SILICON_FFI.closures/cls_multi_sshortchar.c SILICON_FFI.closures/cls_multi_uchar.c SILICON_FFI.closures/cls_multi_ushort.c \
	SILICON_FFI.closures/cls_multi_ushortchar.c SILICON_FFI.closures/cls_pointer.c SILICON_FFI.closures/cls_pointer_stack.c \
	SILICON_FFI.closures/cls_schar.c SILICON_FFI.closures/cls_sint.c SILICON_FFI.closures/cls_sshort.c \
	SILICON_FFI.closures/cls_struct_va1.c SILICON_FFI.closures/cls_uchar.c SILICON_FFI.closures/cls_uint.c \
	SILICON_FFI.closures/cls_uint_va.c SILICON_FFI.closures/cls_ulong_va.c SILICON_FFI.closures/cls_ulonglong.c \
	SILICON_FFI.closures/cls_ushort.c SILICON_FFI.closures/err_bad_abi.c SILICON_FFI.closures/ffitest.h \
	SILICON_FFI.closures/huge_struct.c SILICON_FFI.closures/nested_struct.c SILICON_FFI.closures/nested_struct1.c \
	SILICON_FFI.closures/nested_struct10.c SILICON_FFI.closures/nested_struct11.c SILICON_FFI.closures/nested_struct12.c \
	SILICON_FFI.closures/nested_struct13.c SILICON_FFI.closures/nested_struct2.c SILICON_FFI.closures/nested_struct3.c \
	SILICON_FFI.closures/nested_struct4.c SILICON_FFI.closures/nested_struct5.c SILICON_FFI.closures/nested_struct6.c \
	SILICON_FFI.closures/nested_struct7.c SILICON_FFI.closures/nested_struct8.c SILICON_FFI.closures/nested_struct9.c \
	SILICON_FFI.closures/problem1.c SILICON_FFI.closures/single_entry_structs1.c SILICON_FFI.closures/single_entry_structs2.c \
	SILICON_FFI.closures/single_entry_structs3.c SILICON_FFI.closures/stret_large.c SILICON_FFI.closures/stret_large2.c \
	SILICON_FFI.closures/stret_medium.c SILICON_FFI.closures/stret_medium2.c SILICON_FFI.closures/testclosure.c \
	SILICON_FFI.closures/unwindtest.cc SILICON_FFI.closures/unwindtest_ffi_call.cc SILICON_FFI.complex/cls_align_complex.inc \
	SILICON_FFI.complex/cls_align_complex_double.c SILICON_FFI.complex/cls_align_complex_float.c SILICON_FFI.complex/cls_align_complex_longdouble.c \
	SILICON_FFI.complex/cls_complex.inc SILICON_FFI.complex/cls_complex_double.c SILICON_FFI.complex/cls_complex_float.c \
	SILICON_FFI.complex/cls_complex_longdouble.c SILICON_FFI.complex/cls_complex_struct.inc SILICON_FFI.complex/cls_complex_struct_double.c \
	SILICON_FFI.complex/cls_complex_struct_float.c SILICON_FFI.complex/cls_complex_struct_longdouble.c SILICON_FFI.complex/cls_complex_va.inc \
	SILICON_FFI.complex/cls_complex_va_double.c SILICON_FFI.complex/cls_complex_va_float.c SILICON_FFI.complex/cls_complex_va_longdouble.c \
	SILICON_FFI.complex/complex.exp SILICON_FFI.complex/complex.inc SILICON_FFI.complex/complex_defs_double.inc \
	SILICON_FFI.complex/complex_defs_float.inc SILICON_FFI.complex/complex_defs_longdouble.inc SILICON_FFI.complex/complex_double.c \
	SILICON_FFI.complex/complex_float.c SILICON_FFI.complex/complex_i128.c SILICON_FFI.complex/complex_int.c SILICON_FFI.complex/complex_longdouble.c \
	SILICON_FFI.complex/ffitest.h SILICON_FFI.complex/many_complex.inc SILICON_FFI.complex/many_complex_double.c \
	SILICON_FFI.complex/many_complex_float.c SILICON_FFI.complex/many_complex_longdouble.c SILICON_FFI.complex/return_complex.inc \
	SILICON_FFI.complex/return_complex1.inc SILICON_FFI.complex/return_complex1_double.c SILICON_FFI.complex/return_complex1_float.c \
	SILICON_FFI.complex/return_complex1_longdouble.c SILICON_FFI.complex/return_complex2.inc SILICON_FFI.complex/return_complex2_double.c \
	SILICON_FFI.complex/return_complex2_float.c SILICON_FFI.complex/return_complex2_longdouble.c SILICON_FFI.complex/return_complex_double.c \
	SILICON_FFI.complex/return_complex_float.c SILICON_FFI.complex/return_complex_longdouble.c SILICON_FFI.go/aa-direct.c \
	SILICON_FFI.go/closure1.c SILICON_FFI.go/ffitest.h SILICON_FFI.go/go.exp \
	SILICON_FFI.go/static-chain.h Makefile.am Makefile.in \
	SILICON_FFI.threads/ffitest.h SILICON_FFI.threads/threads.exp SILICON_FFI.threads/tsan.c
