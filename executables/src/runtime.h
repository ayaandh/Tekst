
#pragma once
#include <cstdint>
struct Value;
extern "C" {
Value* rt_none(); Value* rt_int(int64_t); Value* rt_float(double); Value* rt_str(const char*); Value* rt_str_const_empty(); Value* rt_bool(bool); Value* rt_callable(void*); Value* rt_call_callable(Value*,int,...); Value* rt_list_empty(); void rt_list_push(Value*,Value*); Value* rt_optional_attr(Value*,const char*);
Value* rt_add(Value*,Value*); Value* rt_sub(Value*,Value*); Value* rt_mul(Value*,Value*); Value* rt_div(Value*,Value*); Value* rt_mod(Value*,Value*);
Value* rt_eq(Value*,Value*); Value* rt_ne(Value*,Value*); Value* rt_lt(Value*,Value*); Value* rt_le(Value*,Value*); Value* rt_gt(Value*,Value*); Value* rt_ge(Value*,Value*);
Value* rt_neg(Value*); Value* rt_not(Value*); bool rt_truth(Value*);
Value* rt_ref(Value*); Value* rt_deref(Value*); void rt_store(Value*,Value*);
Value* rt_alloc(Value*); void rt_free(Value*); Value* rt_ptr_add(Value*,Value*);
Value* rt_ptr_load_int(Value*); void rt_ptr_store_int(Value*,Value*);
Value* rt_ptr_load_byte(Value*); void rt_ptr_store_byte(Value*,Value*);
void rt_print(Value*); Value* rt_input(Value*); Value* rt_to_int(Value*); Value* rt_to_str(Value*); Value* rt_to_bool(Value*); Value* rt_to_float(Value*); Value* rt_len(Value*);
Value* rt_index(Value*,Value*); void rt_set_index(Value*,Value*,Value*);
Value* rt_list(int,...); Value* rt_dict(int,...); Value* rt_range(int,...);
Value* rt_new_object(const char*); Value* rt_get_attr(Value*,const char*); void rt_set_attr(Value*,const char*,Value*);
Value* rt_call_method(Value*,const char*,...); Value* rt_format(Value*,int,...);
int rt_try_begin(); void rt_try_end(); void rt_throw(Value*); Value* rt_last_error();
}
