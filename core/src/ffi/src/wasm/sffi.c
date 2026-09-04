#include <sffi.h>
#include <sffi_common.h>

#include <stdlib.h>
#include <stdint.h>

#include <emscripten/emscripten.h>

#ifdef DEBUG_F
#define LOG_DEBUG(args...)  \
  console.warn(`====SILICON_FFI(line __LINE__)`, args)
#else
#define LOG_DEBUG(args...) 0
#endif

#define EM_JS_MACROS(ret, name, args, body...) EM_JS(ret, name, args, body)

EM_JS_DEPS(SILICON_FFI, "$getWasmTableEntry,$setWasmTableEntry,$getEmptyTableSlot,$convertJsFunctionToWasm");

#define DEREF_U8(addr, offset) HEAPU8[addr + offset]
#define DEREF_S8(addr, offset) HEAP8[addr + offset]
#define DEREF_U16(addr, offset) HEAPU16[(addr >> 1) + offset]
#define DEREF_S16(addr, offset) HEAP16[(addr >> 1) + offset]
#define DEREF_U32(addr, offset) HEAPU32[(addr >> 2) + offset]
#define DEREF_S32(addr, offset) HEAP32[(addr >> 2) + offset]

#define DEREF_F32(addr, offset) HEAPF32[(addr >> 2) + offset]
#define DEREF_F64(addr, offset) HEAPF64[(addr >> 3) + offset]
#define DEREF_U64(addr, offset) HEAPU64[(addr >> 3) + offset]

#define CHECK_FIELD_OFFSET(struct, field, offset)                                  \
  _Static_assert(                                                                  \
    offsetof(struct, field) == offset,                                             \
    "Memory layout of '" #struct "' has changed: '" #field "' is in an unexpected location");

#if __SIZEOF_POINTER__ == 4

#define SFFI_EMSCRIPTEN_ABI SFFI_WASM32_EMSCRIPTEN
#define PTR_SIG 'i'

#define DEC_PTR(p) p
#define ENC_PTR(p) p

#define DEREF_PTR(addr, offset) DEREF_U32(addr, offset)
#define DEREF_PTR_NUMBER(addr, offset) DEREF_PTR(addr, offset)

#define STORE_ARG_WIDENED_UNSIGNED(rvalue, x) (DEREF_U32(rvalue, 0) = (x))
#define STORE_ARG_WIDENED_SIGNED(rvalue, x) (DEREF_U32(rvalue, 0) = (x))

CHECK_FIELD_OFFSET(sffi_cif, abi, 4*0);
CHECK_FIELD_OFFSET(sffi_cif, nargs, 4*1);
CHECK_FIELD_OFFSET(sffi_cif, arg_types, 4*2);
CHECK_FIELD_OFFSET(sffi_cif, rtype, 4*3);
CHECK_FIELD_OFFSET(sffi_cif, flags, 4*5);
CHECK_FIELD_OFFSET(sffi_cif, nfixedargs, 4*6);

#define CIF__ABI(addr) DEREF_U32(addr, 0)
#define CIF__NARGS(addr) DEREF_U32(addr, 1)
#define CIF__ARGTYPES(addr) DEREF_U32(addr, 2)
#define CIF__RTYPE(addr) DEREF_U32(addr, 3)
#define CIF__FLAGS(addr) DEREF_U32(addr, 5)
#define CIF__NFIXEDARGS(addr) DEREF_U32(addr, 6)

CHECK_FIELD_OFFSET(sffi_type, size, 0);
CHECK_FIELD_OFFSET(sffi_type, alignment, 4);
CHECK_FIELD_OFFSET(sffi_type, type, 6);
CHECK_FIELD_OFFSET(sffi_type, elements, 8);

#define SFFI_TYPE__SIZE(addr) DEREF_U32(addr, 0)
#define SFFI_TYPE__ALIGN(addr) DEREF_U16(addr + 4, 0)
#define SFFI_TYPE__TYPEID(addr) DEREF_U16(addr + 6, 0)
#define SFFI_TYPE__ELEMENTS(addr) DEREF_U32(addr + 8, 0)

#elif __SIZEOF_POINTER__ == 8

#define SFFI_EMSCRIPTEN_ABI SFFI_WASM64_EMSCRIPTEN
#define PTR_SIG 'j'

#define DEC_PTR(p) bigintToI53Checked(p)

#define ENC_PTR(p) BigInt(p)

#define DEREF_PTR(addr, offset) DEREF_U64(addr, offset)
#define DEREF_PTR_NUMBER(addr, offset) DEC_PTR(DEREF_PTR(addr, offset))

#define STORE_ARG_WIDENED_UNSIGNED(rvalue, x) \
  (DEREF_U64(rvalue, 0) = BigInt(x) & BigInt(4294967295))
#define STORE_ARG_WIDENED_SIGNED(rvalue, x) (DEREF_U64(rvalue, 0) = BigInt(x))

CHECK_FIELD_OFFSET(sffi_cif, abi, 0);
CHECK_FIELD_OFFSET(sffi_cif, nargs, 4);
CHECK_FIELD_OFFSET(sffi_cif, arg_types, 8);
CHECK_FIELD_OFFSET(sffi_cif, rtype, 16);
CHECK_FIELD_OFFSET(sffi_cif, flags, 28);
CHECK_FIELD_OFFSET(sffi_cif, nfixedargs, 32);

#define CIF__ABI(addr) DEREF_U32(addr, 0)
#define CIF__NARGS(addr) DEREF_U32(addr + 4,  0)
#define CIF__ARGTYPES(addr) DEREF_U64(addr + 8,  0)
#define CIF__RTYPE(addr) DEREF_U64(addr + 16, 0)
#define CIF__FLAGS(addr) DEREF_U32(addr + 28, 0)
#define CIF__NFIXEDARGS(addr) DEREF_U32(addr + 32, 0)

CHECK_FIELD_OFFSET(sffi_type, size, 0);
CHECK_FIELD_OFFSET(sffi_type, alignment, 8);
CHECK_FIELD_OFFSET(sffi_type, type, 10);
CHECK_FIELD_OFFSET(sffi_type, elements, 16);

#define SFFI_TYPE__SIZE(addr) DEREF_U64(addr, 0)
#define SFFI_TYPE__ALIGN(addr) DEREF_U16(addr + 8, 0)
#define SFFI_TYPE__TYPEID(addr) DEREF_U16(addr + 10, 0)
#define SFFI_TYPE__ELEMENTS(addr) DEREF_U64(addr + 16, 0)

#else
#error "Unknown pointer size"
#endif

#define ALIGN_ADDRESS(addr, align) (addr &= (~((align) - 1)))
#define STACK_ALLOC(stack, size, align) ((stack -= (size)), ALIGN_ADDRESS(stack, align))

#define MAX_ARGS 1000

#include <stddef.h>

#define VARARGS_FLAG 1

#define SFFI_OK_MACRO 0
_Static_assert(SFFI_OK_MACRO == SFFI_OK, "SFFI_OK must be 0");

#define SFFI_BAD_TYPEDEF_MACRO 1
_Static_assert(SFFI_BAD_TYPEDEF_MACRO == SFFI_BAD_TYPEDEF, "SFFI_BAD_TYPEDEF must be 1");

sffi_status SFFI_HIDDEN
sffi_prep_cif_machdep(sffi_cif *cif)
{
  if (cif->abi != SFFI_EMSCRIPTEN_ABI)
    return SFFI_BAD_ABI;

  if (!(cif->flags & VARARGS_FLAG))
    cif->nfixedargs = cif->nargs;
  if (cif->nargs > MAX_ARGS)
    return SFFI_BAD_TYPEDEF;
  if (cif->rtype->type == SFFI_TYPE_COMPLEX)
    return SFFI_BAD_TYPEDEF;

  for (int i = 0; i < cif->nargs; i++)
    if (cif->arg_types[i]->type == SFFI_TYPE_COMPLEX)
      return SFFI_BAD_TYPEDEF;
  return SFFI_OK;
}

sffi_status SFFI_HIDDEN
sffi_prep_cif_machdep_var(sffi_cif *cif, unsigned nfixedargs, unsigned ntotalargs)
{
  cif->flags |= VARARGS_FLAG;
  cif->nfixedargs = nfixedargs;

  if (cif->nfixedargs + 1 > MAX_ARGS)
    return SFFI_BAD_TYPEDEF;
  return SFFI_OK;
}

EM_JS_MACROS(
void,
unbox_small_structs, (sffi_type type_ptr), {
  type_ptr = DEC_PTR(type_ptr);
  var type_id = SFFI_TYPE__TYPEID(type_ptr);
  while (type_id === SFFI_TYPE_STRUCT) {

    if (DEC_PTR(SFFI_TYPE__SIZE(type_ptr)) > 16) {
      break;
    }
    var elements = DEC_PTR(SFFI_TYPE__ELEMENTS(type_ptr));
    var first_element = DEREF_PTR_NUMBER(elements, 0);
    if (first_element === 0) {
      type_id = SFFI_TYPE_VOID;
      break;
    } else if (DEREF_PTR_NUMBER(elements, 1) === 0) {
      type_ptr = first_element;
      type_id = SFFI_TYPE__TYPEID(first_element);
    } else {
      break;
    }
  }
  return [type_ptr, type_id];
})

EM_JS_MACROS(
void,
sffi_call_js, (sffi_cif *cif, sffi_fp fn, void *rvalue, void **avalue),
{
  cif = DEC_PTR(cif);
  fn = DEC_PTR(fn);
  rvalue = DEC_PTR(rvalue);
  avalue = DEC_PTR(avalue);
  var abi = CIF__ABI(cif);
  var nargs = CIF__NARGS(cif);
  var nfixedargs = CIF__NFIXEDARGS(cif);
  var arg_types_ptr = DEC_PTR(CIF__ARGTYPES(cif));
  var flags = CIF__FLAGS(cif);
  var rtype_unboxed = unbox_small_structs(CIF__RTYPE(cif));
  var rtype_ptr = rtype_unboxed[0];
  var rtype_id = rtype_unboxed[1];

  var rtype_widen = SFFI_TYPE__TYPEID(DEC_PTR(CIF__RTYPE(cif))) !== SFFI_TYPE_STRUCT;
  var orig_stack_ptr = stackSave();
  var cur_stack_ptr = orig_stack_ptr;

  var args = [];

  var ret_by_arg = false;

  if (rtype_id === SFFI_TYPE_COMPLEX) {
    throw new Error('complex ret marshalling nyi');
  }
  if (rtype_id < 0 || rtype_id > SFFI_TYPE_LAST) {
    throw new Error('Unexpected rtype ' + rtype_id);
  }

  if (rtype_id === SFFI_TYPE_LONGDOUBLE || rtype_id === SFFI_TYPE_STRUCT) {
    if (rvalue === 0) {

      var rsize = DEC_PTR(SFFI_TYPE__SIZE(rtype_ptr));
      var ralign = SFFI_TYPE__ALIGN(rtype_ptr);
      STACK_ALLOC(cur_stack_ptr, rsize, ralign);
      rvalue = cur_stack_ptr;
    }
    args.push(ENC_PTR(rvalue));
    ret_by_arg = true;
  }

  for (var i = 0; i < nfixedargs; i++) {
    var arg_ptr = DEREF_PTR_NUMBER(avalue, i);
    var arg_unboxed = unbox_small_structs(DEREF_PTR(arg_types_ptr, i));
    var arg_type_ptr = arg_unboxed[0];
    var arg_type_id = arg_unboxed[1];

    switch (arg_type_id) {
    case SFFI_TYPE_INT:
    case SFFI_TYPE_SINT32:
    case SFFI_TYPE_UINT32:
      args.push(DEREF_U32(arg_ptr, 0));
      break;
    case SFFI_TYPE_FLOAT:
      args.push(DEREF_F32(arg_ptr, 0));
      break;
    case SFFI_TYPE_DOUBLE:
      args.push(DEREF_F64(arg_ptr, 0));
      break;
    case SFFI_TYPE_UINT8:
      args.push(DEREF_U8(arg_ptr, 0));
      break;
    case SFFI_TYPE_SINT8:
      args.push(DEREF_S8(arg_ptr, 0));
      break;
    case SFFI_TYPE_UINT16:
      args.push(DEREF_U16(arg_ptr, 0));
      break;
    case SFFI_TYPE_SINT16:
      args.push(DEREF_S16(arg_ptr, 0));
      break;
    case SFFI_TYPE_UINT64:
    case SFFI_TYPE_SINT64:
      args.push(DEREF_U64(arg_ptr, 0));
      break;
    case SFFI_TYPE_LONGDOUBLE:

      args.push(DEREF_U64(arg_ptr, 0));
      args.push(DEREF_U64(arg_ptr, 1));
      break;
    case SFFI_TYPE_STRUCT:

      var size = DEC_PTR(SFFI_TYPE__SIZE(arg_type_ptr));
      var align = SFFI_TYPE__ALIGN(arg_type_ptr);
      STACK_ALLOC(cur_stack_ptr, size, align);
      HEAP8.subarray(cur_stack_ptr, cur_stack_ptr+size).set(HEAP8.subarray(arg_ptr, arg_ptr + size));
      args.push(ENC_PTR(cur_stack_ptr));
      break;
    case SFFI_TYPE_POINTER:
      args.push(DEREF_PTR(arg_ptr, 0));
      break;
    case SFFI_TYPE_COMPLEX:
      throw new Error('complex marshalling nyi');
    default:
      throw new Error('Unexpected type ' + arg_type_id);
    }
  }

  if (flags & VARARGS_FLAG) {
    var struct_arg_info = [];
    for (var i = nargs - 1;  i >= nfixedargs; i--) {
      var arg_ptr = DEREF_PTR_NUMBER(avalue, i);
      var arg_unboxed = unbox_small_structs(DEREF_PTR(arg_types_ptr, i));
      var arg_type_ptr = arg_unboxed[0];
      var arg_type_id = arg_unboxed[1];
      switch (arg_type_id) {
      case SFFI_TYPE_UINT8:
      case SFFI_TYPE_SINT8:
        STACK_ALLOC(cur_stack_ptr, 1, 1);
        DEREF_U8(cur_stack_ptr, 0) = DEREF_U8(arg_ptr, 0);
        break;
      case SFFI_TYPE_UINT16:
      case SFFI_TYPE_SINT16:
        STACK_ALLOC(cur_stack_ptr, 2, 2);
        DEREF_U16(cur_stack_ptr, 0) = DEREF_U16(arg_ptr, 0);
        break;
      case SFFI_TYPE_INT:
      case SFFI_TYPE_UINT32:
      case SFFI_TYPE_SINT32:
      case SFFI_TYPE_FLOAT:
        STACK_ALLOC(cur_stack_ptr, 4, 4);
        DEREF_U32(cur_stack_ptr, 0) = DEREF_U32(arg_ptr, 0);
        break;
      case SFFI_TYPE_DOUBLE:
      case SFFI_TYPE_UINT64:
      case SFFI_TYPE_SINT64:
        STACK_ALLOC(cur_stack_ptr, 8, 8);
        DEREF_U32(cur_stack_ptr, 0) = DEREF_U32(arg_ptr, 0);
        DEREF_U32(cur_stack_ptr, 1) = DEREF_U32(arg_ptr, 1);
        break;
      case SFFI_TYPE_LONGDOUBLE:
        STACK_ALLOC(cur_stack_ptr, 16, 8);
        DEREF_U32(cur_stack_ptr, 0) = DEREF_U32(arg_ptr, 0);
        DEREF_U32(cur_stack_ptr, 1) = DEREF_U32(arg_ptr, 1);
        DEREF_U32(cur_stack_ptr, 2) = DEREF_U32(arg_ptr, 2);
        DEREF_U32(cur_stack_ptr, 3) = DEREF_U32(arg_ptr, 3);
        break;
      case SFFI_TYPE_STRUCT:

        STACK_ALLOC(cur_stack_ptr, __SIZEOF_POINTER__, __SIZEOF_POINTER__);
        struct_arg_info.push([cur_stack_ptr, arg_ptr, DEC_PTR(SFFI_TYPE__SIZE(arg_type_ptr)), SFFI_TYPE__ALIGN(arg_type_ptr)]);
        break;
      case SFFI_TYPE_POINTER:
        STACK_ALLOC(cur_stack_ptr, __SIZEOF_POINTER__, __SIZEOF_POINTER__);
        DEREF_PTR(cur_stack_ptr, 0) = DEREF_PTR(arg_ptr, 0);
        break;
      case SFFI_TYPE_COMPLEX:
        throw new Error('complex arg marshalling nyi');
      default:
        throw new Error('Unexpected argtype ' + arg_type_id);
      }
    }

    args.push(ENC_PTR(cur_stack_ptr));

    for (var i = 0; i < struct_arg_info.length; i++) {
      var struct_info = struct_arg_info[i];
      var arg_target = struct_info[0];
      var arg_ptr = struct_info[1];
      var size = struct_info[2];
      var align = struct_info[3];
      STACK_ALLOC(cur_stack_ptr, size, align);
      HEAP8.subarray(cur_stack_ptr, cur_stack_ptr+size).set(HEAP8.subarray(arg_ptr, arg_ptr + size));
      DEREF_PTR(arg_target, 0) = ENC_PTR(cur_stack_ptr);
    }
  }
  stackRestore(cur_stack_ptr);
  stackAlloc(0);
  LOG_DEBUG("CALL_FUNC_PTR", "fn:", fn, "args:", args);
  var result = getWasmTableEntry(fn).apply(null, args);

  stackRestore(orig_stack_ptr);

  if (ret_by_arg) {
    return;
  }

  if (rvalue === 0) {
    return;
  }

  switch (rtype_id) {
  case SFFI_TYPE_VOID:
    break;
  case SFFI_TYPE_INT:
  case SFFI_TYPE_SINT32:
    if (rtype_widen)
      STORE_ARG_WIDENED_SIGNED(rvalue, result | 0);
    else
      DEREF_U32(rvalue, 0) = result;
    break;
  case SFFI_TYPE_UINT32:
    if (rtype_widen)
      STORE_ARG_WIDENED_UNSIGNED(rvalue, result | 0);
    else
      DEREF_U32(rvalue, 0) = result;
    break;
  case SFFI_TYPE_FLOAT:
    DEREF_F32(rvalue, 0) = result;
    break;
  case SFFI_TYPE_DOUBLE:
    DEREF_F64(rvalue, 0) = result;
    break;
  case SFFI_TYPE_UINT8:
    if (rtype_widen)
      STORE_ARG_WIDENED_UNSIGNED(rvalue, result & 0xff);
    else
      DEREF_U8(rvalue, 0) = result;
    break;
  case SFFI_TYPE_SINT8:
    if (rtype_widen)
      STORE_ARG_WIDENED_SIGNED(rvalue, (result << 24) >> 24);
    else
      DEREF_U8(rvalue, 0) = result;
    break;
  case SFFI_TYPE_UINT16:
    if (rtype_widen)
      STORE_ARG_WIDENED_UNSIGNED(rvalue, result & 0xffff);
    else
      DEREF_U16(rvalue, 0) = result;
    break;
  case SFFI_TYPE_SINT16:
    if (rtype_widen)
      STORE_ARG_WIDENED_SIGNED(rvalue, (result << 16) >> 16);
    else
      DEREF_U16(rvalue, 0) = result;
    break;
  case SFFI_TYPE_UINT64:
  case SFFI_TYPE_SINT64:
    DEREF_U64(rvalue, 0) = result;
    break;
  case SFFI_TYPE_POINTER:
    DEREF_PTR(rvalue, 0) = result;
    break;
  case SFFI_TYPE_COMPLEX:
    throw new Error('complex ret marshalling nyi');
  default:
    throw new Error('Unexpected rtype ' + rtype_id);
  }
});

void sffi_call(sffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue) {
  sffi_call_js(cif, fn, rvalue, avalue);
}

#if __SIZEOF_POINTER__ == 4

CHECK_FIELD_OFFSET(sffi_closure, ftramp, 4*0);
CHECK_FIELD_OFFSET(sffi_closure, cif, 4*1);
CHECK_FIELD_OFFSET(sffi_closure, fun, 4*2);
CHECK_FIELD_OFFSET(sffi_closure, user_data, 4*3);

#define CLOSURE__wrapper(addr) DEREF_U32(addr, 0)
#define CLOSURE__cif(addr) DEREF_U32(addr, 1)
#define CLOSURE__fun(addr) DEREF_U32(addr, 2)
#define CLOSURE__user_data(addr) DEREF_U32(addr, 3)

#elif __SIZEOF_POINTER__ == 8

CHECK_FIELD_OFFSET(sffi_closure, ftramp, 0);
CHECK_FIELD_OFFSET(sffi_closure, cif, 8);
CHECK_FIELD_OFFSET(sffi_closure, fun, 16);
CHECK_FIELD_OFFSET(sffi_closure, user_data, 24);

#define CLOSURE__wrapper(addr) DEREF_U64(addr, 0)
#define CLOSURE__cif(addr) DEREF_U64(addr, 1)
#define CLOSURE__fun(addr) DEREF_U64(addr, 2)
#define CLOSURE__user_data(addr) DEREF_U64(addr, 3)

#else
#error "Unknown pointer size"
#endif

EM_JS_MACROS(void *, sffi_closure_alloc_js, (size_t size, void **code), {
  size = DEC_PTR(size);
  code = DEC_PTR(code);
  var closure = _malloc(size);
  var index = getEmptyTableSlot();
  DEREF_PTR(code, 0) = ENC_PTR(index);
  CLOSURE__wrapper(closure) = ENC_PTR(index);
  return ENC_PTR(closure);
})

void * __attribute__ ((visibility ("default")))
sffi_closure_alloc(size_t size, void **code) {
  return sffi_closure_alloc_js(size, code);
}

EM_JS_MACROS(void, sffi_closure_free_js, (void *closure), {
  closure = DEC_PTR(closure);
  var index = DEC_PTR(CLOSURE__wrapper(closure));
  freeTableIndexes.push(index);
  _free(closure);
})

void __attribute__ ((visibility ("default")))
sffi_closure_free(void *closure) {
  return sffi_closure_free_js(closure);
}

EM_JS_MACROS(
sffi_status,
sffi_prep_closure_loc_js,
(sffi_closure *closure, sffi_cif *cif, void *fun, void *user_data, void *codeloc),
{
  closure = DEC_PTR(closure);
  cif = DEC_PTR(cif);
  fun = DEC_PTR(fun);
  user_data = DEC_PTR(user_data);
  codeloc = DEC_PTR(codeloc);
  var abi = CIF__ABI(cif);
  var nargs = CIF__NARGS(cif);
  var nfixedargs = CIF__NFIXEDARGS(cif);
  var arg_types_ptr = DEC_PTR(CIF__ARGTYPES(cif));
  var rtype_unboxed = unbox_small_structs(CIF__RTYPE(cif));
  var rtype_ptr = rtype_unboxed[0];
  var rtype_id = rtype_unboxed[1];

  var sig;
  var ret_by_arg = false;
  switch (rtype_id) {
  case SFFI_TYPE_VOID:
    sig = 'v';
    break;
  case SFFI_TYPE_STRUCT:
  case SFFI_TYPE_LONGDOUBLE:

    sig = 'v' + PTR_SIG;
    ret_by_arg = true;
    break;
  case SFFI_TYPE_INT:
  case SFFI_TYPE_UINT8:
  case SFFI_TYPE_SINT8:
  case SFFI_TYPE_UINT16:
  case SFFI_TYPE_SINT16:
  case SFFI_TYPE_UINT32:
  case SFFI_TYPE_SINT32:
    sig = 'i';
    break;
  case SFFI_TYPE_FLOAT:
    sig = 'f';
    break;
  case SFFI_TYPE_DOUBLE:
    sig = 'd';
    break;
  case SFFI_TYPE_UINT64:
  case SFFI_TYPE_SINT64:
    sig = 'j';
    break;
  case SFFI_TYPE_POINTER:
    sig = PTR_SIG;
    break;
  case SFFI_TYPE_COMPLEX:
    throw new Error('complex ret marshalling nyi');
  default:
    throw new Error('Unexpected rtype ' + rtype_id);
  }
  var unboxed_arg_type_id_list = [];
  var unboxed_arg_type_info_list = [];
  for (var i = 0; i < nargs; i++) {
    var arg_unboxed = unbox_small_structs(DEREF_PTR(arg_types_ptr, i));
    var arg_type_ptr = arg_unboxed[0];
    var arg_type_id = arg_unboxed[1];
    unboxed_arg_type_id_list.push(arg_type_id);
    unboxed_arg_type_info_list.push([DEC_PTR(SFFI_TYPE__SIZE(arg_type_ptr)), SFFI_TYPE__ALIGN(arg_type_ptr)]);
  }
  for (var i = 0; i < nfixedargs; i++) {
    switch (unboxed_arg_type_id_list[i]) {
    case SFFI_TYPE_INT:
    case SFFI_TYPE_UINT8:
    case SFFI_TYPE_SINT8:
    case SFFI_TYPE_UINT16:
    case SFFI_TYPE_SINT16:
    case SFFI_TYPE_UINT32:
    case SFFI_TYPE_SINT32:
      sig += 'i';
      break;
    case SFFI_TYPE_FLOAT:
      sig += 'f';
      break;
    case SFFI_TYPE_DOUBLE:
      sig += 'd';
      break;
    case SFFI_TYPE_LONGDOUBLE:
      sig += 'jj';
      break;
    case SFFI_TYPE_UINT64:
    case SFFI_TYPE_SINT64:
      sig += 'j';
      break;
    case SFFI_TYPE_STRUCT:
    case SFFI_TYPE_POINTER:
      sig += PTR_SIG;
      break;
    case SFFI_TYPE_COMPLEX:
      throw new Error('complex marshalling nyi');
    default:
      throw new Error('Unexpected argtype ' + arg_type_id);
    }
  }
  if (nfixedargs < nargs) {

    sig += PTR_SIG;
  }
  LOG_DEBUG("CREATE_CLOSURE", "sig:", sig);
  function trampoline() {
    var args = Array.prototype.slice.call(arguments);
    var size = 0;
    var orig_stack_ptr = stackSave();
    var cur_ptr = orig_stack_ptr;
    var ret_ptr;
    var jsarg_idx = 0;

    if (ret_by_arg) {
      ret_ptr = args[jsarg_idx++];
    } else {

      STACK_ALLOC(cur_ptr, 8, 8);
      ret_ptr = cur_ptr;
    }
    cur_ptr -= __SIZEOF_POINTER__ * nargs;
    var args_ptr = cur_ptr;
    var carg_idx = 0;

    for (; carg_idx < nfixedargs; carg_idx++) {

      var cur_arg = args[jsarg_idx++];
      var arg_type_info = unboxed_arg_type_info_list[carg_idx];
      var arg_size = arg_type_info[0];
      var arg_align = arg_type_info[1];
      var arg_type_id = unboxed_arg_type_id_list[carg_idx];
      switch (arg_type_id) {
      case SFFI_TYPE_UINT8:
      case SFFI_TYPE_SINT8:

        STACK_ALLOC(cur_ptr, 1, 4);
        DEREF_PTR(args_ptr, carg_idx) = ENC_PTR(cur_ptr);
        DEREF_U8(cur_ptr, 0) = cur_arg;
        break;
      case SFFI_TYPE_UINT16:
      case SFFI_TYPE_SINT16:

        STACK_ALLOC(cur_ptr, 2, 4);
        DEREF_PTR(args_ptr, carg_idx) = ENC_PTR(cur_ptr);
        DEREF_U16(cur_ptr, 0) = cur_arg;
        break;
      case SFFI_TYPE_INT:
      case SFFI_TYPE_UINT32:
      case SFFI_TYPE_SINT32:
        STACK_ALLOC(cur_ptr, 4, 4);
        DEREF_PTR(args_ptr, carg_idx) = ENC_PTR(cur_ptr);
        DEREF_U32(cur_ptr, 0) = cur_arg;
        break;
      case SFFI_TYPE_STRUCT:

        STACK_ALLOC(cur_ptr, arg_size, arg_align);
        HEAP8.subarray(cur_ptr, cur_ptr + arg_size).set(HEAP8.subarray(DEC_PTR(cur_arg), DEC_PTR(cur_arg) + arg_size));
        DEREF_PTR(args_ptr, carg_idx) = ENC_PTR(cur_ptr);
        break;
      case SFFI_TYPE_FLOAT:
        STACK_ALLOC(cur_ptr, 4, 4);
        DEREF_PTR(args_ptr, carg_idx) = ENC_PTR(cur_ptr);
        DEREF_F32(cur_ptr, 0) = cur_arg;
        break;
      case SFFI_TYPE_DOUBLE:
        STACK_ALLOC(cur_ptr, 8, 8);
        DEREF_PTR(args_ptr, carg_idx) = ENC_PTR(cur_ptr);
        DEREF_F64(cur_ptr, 0) = cur_arg;
        break;
      case SFFI_TYPE_UINT64:
      case SFFI_TYPE_SINT64:
        STACK_ALLOC(cur_ptr, 8, 8);
        DEREF_PTR(args_ptr, carg_idx) = ENC_PTR(cur_ptr);
        DEREF_U64(cur_ptr, 0) = cur_arg;
        break;
      case SFFI_TYPE_LONGDOUBLE:
        STACK_ALLOC(cur_ptr, 16, 8);
        DEREF_PTR(args_ptr, carg_idx) = ENC_PTR(cur_ptr);
        DEREF_U64(cur_ptr, 0) = cur_arg;
        cur_arg = args[jsarg_idx++];
        DEREF_U64(cur_ptr, 1) = cur_arg;
        break;
      case SFFI_TYPE_POINTER:
        STACK_ALLOC(cur_ptr, __SIZEOF_POINTER__, __SIZEOF_POINTER__);
        DEREF_PTR(args_ptr, carg_idx) = ENC_PTR(cur_ptr);
        DEREF_PTR(cur_ptr, 0) = cur_arg;
        break;
      }
    }

    var varargs = DEC_PTR(args[args.length - 1]);

    for (; carg_idx < nargs; carg_idx++) {
      var arg_type_id = unboxed_arg_type_id_list[carg_idx];
      var arg_type_info = unboxed_arg_type_info_list[carg_idx];
      var arg_size = arg_type_info[0];
      var arg_align = arg_type_info[1];
      if (arg_type_id === SFFI_TYPE_STRUCT) {

        var struct_ptr = DEREF_PTR_NUMBER(varargs, 0);
        STACK_ALLOC(cur_ptr, arg_size, arg_align);
        HEAP8.subarray(cur_ptr, cur_ptr + arg_size).set(HEAP8.subarray(struct_ptr, struct_ptr + arg_size));
        DEREF_PTR(args_ptr, carg_idx) = ENC_PTR(cur_ptr);
      } else {
        DEREF_PTR(args_ptr, carg_idx) = ENC_PTR(varargs);
      }
      varargs += __SIZEOF_POINTER__;
    }
    stackRestore(cur_ptr);
    stackAlloc(0);
    LOG_DEBUG("CALL_CLOSURE", "closure:", closure, "fptr", CLOSURE__fun(closure), "cif", CLOSURE__cif(closure));
    getWasmTableEntry(CLOSURE__fun(closure))(
        CLOSURE__cif(closure), ENC_PTR(ret_ptr), ENC_PTR(args_ptr),
        CLOSURE__user_data(closure)
    );
    stackRestore(orig_stack_ptr);

    if (!ret_by_arg) {
      switch (sig[0]) {
      case 'i':
        return DEREF_U32(ret_ptr, 0);
      case 'j':
        return DEREF_U64(ret_ptr, 0);
      case 'd':
        return DEREF_F64(ret_ptr, 0);
      case 'f':
        return DEREF_F32(ret_ptr, 0);
      }
    }
  }
  try {
    var wasm_trampoline = convertJsFunctionToWasm(trampoline, sig);
  } catch(e) {
    return SFFI_BAD_TYPEDEF_MACRO;
  }
  setWasmTableEntry(codeloc, wasm_trampoline);
  CLOSURE__cif(closure) = ENC_PTR(cif);
  CLOSURE__fun(closure) = ENC_PTR(fun);
  CLOSURE__user_data(closure) = ENC_PTR(user_data);
  return SFFI_OK_MACRO;
})

sffi_status sffi_prep_closure_loc(sffi_closure *closure, sffi_cif *cif,
                                void (*fun)(sffi_cif *, void *, void **, void *),
                                void *user_data, void *codeloc) {
  if (cif->abi != SFFI_EMSCRIPTEN_ABI)
    return SFFI_BAD_ABI;
  return sffi_prep_closure_loc_js(closure, cif, (void *)fun, user_data,
                                     codeloc);
}
