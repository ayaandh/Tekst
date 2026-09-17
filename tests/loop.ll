; Tekst LLVM IR
source_filename = "Tekst"

declare ptr @rt_none()
declare ptr @rt_int(i64)
declare ptr @rt_float(double)
declare ptr @rt_str(ptr)
declare ptr @rt_str_const_empty()
declare ptr @rt_bool(i1)
declare ptr @rt_callable(ptr)
declare ptr @rt_call_callable(ptr,i32,...)
declare ptr @rt_list_empty()
declare void @rt_list_push(ptr,ptr)
declare ptr @rt_optional_attr(ptr,ptr)
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

define i32 @main() {
entry:
  %v1 = call ptr @rt_int(i64 5)
  %v2 = call ptr (i32, ...) @rt_range(i32 1, ptr %v1)
  %foridx3 = alloca ptr
  %v4 = call ptr @rt_int(i64 0)
  store ptr %v4, ptr %foridx3
  br label %for1
for1:
  %v5 = load ptr, ptr %foridx3
  %v6 = call ptr @rt_len(ptr %v2)
  %v7 = call ptr @rt_lt(ptr %v5, ptr %v6)
  %v8 = call i1 @rt_truth(ptr %v7)
  br i1 %v8, label %forbody3, label %forend2
forbody3:
  %v9 = call ptr @rt_index(ptr %v2, ptr %v5)
  %slot_i = alloca ptr
  %v10 = call ptr @rt_none()
  store ptr %v10, ptr %slot_i
  store ptr %v9, ptr %slot_i
  %v11 = load ptr, ptr %slot_i
  %v12 = call ptr @rt_int(i64 2)
  %v13 = call ptr @rt_eq(ptr %v11, ptr %v12)
  %v14 = call i1 @rt_truth(ptr %v13)
  br i1 %v14, label %ifyes6, label %ifno7
ifyes6:
  br label %forinc4
ifno7:
  br label %ifend5
ifend5:
  %v15 = load ptr, ptr %slot_i
  %v16 = call ptr @rt_int(i64 4)
  %v17 = call ptr @rt_eq(ptr %v15, ptr %v16)
  %v18 = call i1 @rt_truth(ptr %v17)
  br i1 %v18, label %ifyes9, label %ifno10
ifyes9:
  br label %forend2
ifno10:
  br label %ifend8
ifend8:
  %v19 = load ptr, ptr %slot_i
  call void @rt_print(ptr %v19)
forinc4:
  %v20 = call ptr @rt_int(i64 1)
  %v21 = call ptr @rt_add(ptr %v5, ptr %v20)
  store ptr %v21, ptr %foridx3
  br label %for1
forend2:
  ret i32 0
}
