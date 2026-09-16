; Tekst LLVM IR
source_filename = "Tekst"

declare ptr @rt_none()
declare ptr @rt_int(i64)
declare ptr @rt_float(double)
declare ptr @rt_str(ptr)
declare ptr @rt_str_const_empty()
declare ptr @rt_bool(i1)
declare ptr @rt_add(ptr,ptr)
declare ptr @rt_sub(ptr,ptr)
declare ptr @rt_mul(ptr,ptr)
declare ptr @rt_div(ptr,ptr)
declare ptr @rt_mod(ptr,ptr)
declare ptr @rt_eq(ptr,ptr)
declare ptr @rt_ne(ptr,ptr)
declare ptr @rt_lt(ptr,ptr)
declare ptr @rt_le(ptr,ptr)
declare ptr @rt_gt(ptr,ptr)
declare ptr @rt_ge(ptr,ptr)
declare ptr @rt_neg(ptr)
declare ptr @rt_not(ptr)
declare ptr @rt_ref(ptr)
declare ptr @rt_deref(ptr)
declare void @rt_store(ptr,ptr)
declare ptr @rt_alloc(ptr)
declare void @rt_free(ptr)
declare ptr @rt_ptr_add(ptr,ptr)
declare ptr @rt_ptr_load_int(ptr)
declare void @rt_ptr_store_int(ptr,ptr)
declare ptr @rt_ptr_load_byte(ptr)
declare void @rt_ptr_store_byte(ptr,ptr)
declare ptr @rt_and(ptr,ptr)
declare ptr @rt_or(ptr,ptr)
declare i1 @rt_truth(ptr)
declare void @rt_print(ptr)
declare ptr @rt_input(ptr)
declare ptr @rt_to_int(ptr)
declare ptr @rt_to_str(ptr)
declare ptr @rt_to_bool(ptr)
declare ptr @rt_to_float(ptr)
declare ptr @rt_len(ptr)
declare ptr @rt_index(ptr,ptr)
declare void @rt_set_index(ptr,ptr,ptr)
declare ptr @rt_list(i32,...)
declare ptr @rt_dict(i32,...)
declare ptr @rt_range(i32,...)
declare ptr @rt_new_object(ptr)
declare ptr @rt_get_attr(ptr,ptr)
declare void @rt_set_attr(ptr,ptr,ptr)
declare ptr @rt_call_method(ptr,ptr,...)
declare ptr @rt_format(ptr,i32,...)
declare i32 @rt_try_begin()
declare void @rt_try_end()
declare void @rt_throw(ptr)
declare ptr @rt_last_error()

define ptr @square(ptr %a0) {
entry:
  %slot_x = alloca ptr
  %v1 = call ptr @rt_none()
  store ptr %v1, ptr %slot_x
  store ptr %a0, ptr %slot_x
  %v2 = load ptr, ptr %slot_x
  %v3 = load ptr, ptr %slot_x
  %v4 = call ptr @rt_mul(ptr %v2, ptr %v3)
  ret ptr %v4
  %v5 = call ptr @rt_none()
  ret ptr %v5
}
define i32 @main() {
entry:
  %v1 = call ptr @rt_int(i64 9)
  %v2 = call ptr @square(ptr %v1)
  call void @rt_print(ptr %v2)
  ret i32 0
}
