
#include "runtime.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <setjmp.h>
#include <stdexcept>
#include <algorithm>

enum class Kind { None, Int, Float, Bool, String, List, Dict, Object, Pointer };
struct Value {
 Kind kind=Kind::None; int64_t i=0; double f=0; bool b=false; std::string s;
 std::vector<Value*> list; std::unordered_map<std::string,Value*> dict, fields; std::string cls;
 void* ptr=nullptr; void* base=nullptr; size_t size=0; size_t offset=0; bool reference=false;
};
static Value* V(){return new Value();}
static Value* make(Kind k){auto*x=V();x->kind=k;return x;}
static std::string text(Value*v){
 if(!v)return "None";
 switch(v->kind){
 case Kind::None:return "None";case Kind::Int:return std::to_string(v->i);case Kind::Float:{std::ostringstream o;o<<v->f;return o.str();}
 case Kind::Bool:return v->b?"True":"False";case Kind::String:return v->s;
 case Kind::List:{std::string o="[";for(size_t i=0;i<v->list.size();++i){if(i)o+=", ";o+=text(v->list[i]);}return o+"]";}
 case Kind::Dict:{std::string o="{";size_t i=0;for(auto&[k,x]:v->dict){if(i++)o+=", ";o+=k+": "+text(x);}return o+"}";}
 case Kind::Object:return "<"+v->cls+" object>";case Kind::Pointer:{std::ostringstream o;o<<"0x"<<std::hex<<(uintptr_t)v->ptr;return o.str();}
 }return "";
}
static bool truth(Value*v){if(!v)return false;switch(v->kind){case Kind::None:return false;case Kind::Bool:return v->b;case Kind::Int:return v->i!=0;case Kind::Float:return v->f!=0;case Kind::String:return !v->s.empty();case Kind::List:return !v->list.empty();case Kind::Dict:return !v->dict.empty();case Kind::Pointer:return v->ptr!=nullptr;default:return true;}}
static void fail(const std::string&m);
extern "C" Value* rt_none(){return make(Kind::None);}
extern "C" Value* rt_int(int64_t n){auto*v=make(Kind::Int);v->i=n;return v;}
extern "C" Value* rt_float(double n){auto*v=make(Kind::Float);v->f=n;return v;}
extern "C" Value* rt_str(const char*s){auto*v=make(Kind::String);v->s=s?s:"";return v;}
extern "C" Value* rt_str_const_empty(){return rt_str("");}
extern "C" Value* rt_bool(bool b){auto*v=make(Kind::Bool);v->b=b;return v;}
static double num(Value*v){if(v->kind==Kind::Int)return(double)v->i;if(v->kind==Kind::Float)return v->f;if(v->kind==Kind::Bool)return v->b;fail("expected number");return 0;}
extern "C" Value* rt_add(Value*a,Value*b){if(a->kind==Kind::Pointer&&b->kind==Kind::Int)return rt_ptr_add(a,b);if(a->kind==Kind::String||b->kind==Kind::String)return rt_str((text(a)+text(b)).c_str());if(a->kind==Kind::Float||b->kind==Kind::Float)return rt_float(num(a)+num(b));return rt_int((int64_t)(num(a)+num(b)));}
extern "C" Value* rt_sub(Value*a,Value*b){if(a->kind==Kind::Pointer&&b->kind==Kind::Int){auto neg=rt_int(-b->i);return rt_ptr_add(a,neg);}if(a->kind==Kind::Float||b->kind==Kind::Float)return rt_float(num(a)-num(b));return rt_int((int64_t)(num(a)-num(b)));}
extern "C" Value* rt_mul(Value*a,Value*b){if(a->kind==Kind::String&&b->kind==Kind::Int){std::string s;for(int64_t i=0;i<b->i;i++)s+=a->s;return rt_str(s.c_str());}if(a->kind==Kind::Float||b->kind==Kind::Float)return rt_float(num(a)*num(b));return rt_int((int64_t)(num(a)*num(b)));}
extern "C" Value* rt_div(Value*a,Value*b){double d=num(b);if(d==0)fail("division by zero");if(a->kind==Kind::Int&&b->kind==Kind::Int&&a->i%b->i==0)return rt_int(a->i/b->i);return rt_float(num(a)/d);}
extern "C" Value* rt_mod(Value*a,Value*b){if(b->i==0)fail("division by zero");return rt_int(a->i%b->i);}
static bool equal(Value*a,Value*b){if(a->kind!=b->kind){if((a->kind==Kind::Int||a->kind==Kind::Float)&&(b->kind==Kind::Int||b->kind==Kind::Float))return num(a)==num(b);return false;}if(a->kind==Kind::None)return true;if(a->kind==Kind::Int)return a->i==b->i;if(a->kind==Kind::Float)return a->f==b->f;if(a->kind==Kind::Bool)return a->b==b->b;if(a->kind==Kind::String)return a->s==b->s;return a==b;}
extern "C" Value* rt_eq(Value*a,Value*b){return rt_bool(equal(a,b));} extern "C" Value* rt_ne(Value*a,Value*b){return rt_bool(!equal(a,b));}
static int cmp(Value*a,Value*b){if(a->kind==Kind::String&&b->kind==Kind::String)return a->s.compare(b->s);double x=num(a),y=num(b);return x<y?-1:x>y?1:0;}
extern "C" Value* rt_lt(Value*a,Value*b){return rt_bool(cmp(a,b)<0);} extern "C" Value* rt_le(Value*a,Value*b){return rt_bool(cmp(a,b)<=0);} extern "C" Value* rt_gt(Value*a,Value*b){return rt_bool(cmp(a,b)>0);} extern "C" Value* rt_ge(Value*a,Value*b){return rt_bool(cmp(a,b)>=0);}
extern "C" Value* rt_and(Value*a,Value*b){return rt_bool(truth(a)&&truth(b));}
extern "C" Value* rt_or(Value*a,Value*b){return rt_bool(truth(a)||truth(b));}
extern "C" Value* rt_neg(Value*a){return a->kind==Kind::Float?rt_float(-a->f):rt_int(-a->i);}
extern "C" Value* rt_not(Value*a){return rt_bool(!truth(a));} extern "C" bool rt_truth(Value*a){return truth(a);}

static std::unordered_set<void*> allocations;
extern "C" Value* rt_ref(Value*slot){
 if(!slot) fail("cannot create reference to null slot");
 auto*v=make(Kind::Pointer); v->ptr=slot; v->base=slot; v->size=sizeof(Value*); v->reference=true; return v;
}
extern "C" Value* rt_deref(Value*p){
 if(!p||p->kind!=Kind::Pointer||!p->ptr) fail("null or invalid pointer dereference");
 if(!p->reference) fail("raw pointer requires load_int/load_byte or another typed load");
 return *reinterpret_cast<Value**>(p->ptr);
}
extern "C" void rt_store(Value*p,Value*v){
 if(!p||p->kind!=Kind::Pointer||!p->ptr) fail("null or invalid pointer store");
 if(!p->reference) fail("raw pointer requires store_int/store_byte");
 *reinterpret_cast<Value**>(p->ptr)=v;
}
extern "C" Value* rt_alloc(Value*n){
 if(!n||n->kind!=Kind::Int||n->i<0) fail("alloc expects a non-negative integer size");
 size_t sz=(size_t)n->i; if(sz==0) sz=1;
 void*mem=std::malloc(sz); if(!mem) fail("out of memory");
 allocations.insert(mem); auto*v=make(Kind::Pointer);v->ptr=mem;v->base=mem;v->size=sz;v->reference=false;return v;
}
extern "C" void rt_free(Value*p){
 if(!p||p->kind!=Kind::Pointer||!p->base) fail("free expects a valid pointer");
 if(p->reference) fail("cannot free a reference");
 auto it=allocations.find(p->base); if(it==allocations.end()) fail("invalid or double free");
 std::free(p->base); allocations.erase(it); p->ptr=nullptr;p->base=nullptr;p->size=0;
}
extern "C" Value* rt_ptr_add(Value*p,Value*n){
 if(!p||p->kind!=Kind::Pointer||!p->ptr) fail("pointer arithmetic requires a valid pointer");
 if(!n||n->kind!=Kind::Int) fail("pointer offset must be an integer");
 intptr_t off=(intptr_t)n->i; long long next=(long long)p->offset+(long long)off; if(next<0 || (size_t)next>p->size) fail("pointer arithmetic out of bounds"); auto*v=make(Kind::Pointer);v->ptr=(void*)((char*)p->base+next);v->base=p->base;v->size=p->size;v->offset=(size_t)next;v->reference=p->reference;return v;
}
static void check_raw(Value*p,size_t bytes){if(!p||p->kind!=Kind::Pointer||!p->ptr||p->reference)fail("raw memory pointer required"); if(bytes>p->size)fail("pointer access out of bounds");}
extern "C" Value* rt_ptr_load_int(Value*p){check_raw(p,sizeof(int64_t));return rt_int(*reinterpret_cast<int64_t*>(p->ptr));}
extern "C" void rt_ptr_store_int(Value*p,Value*v){check_raw(p,sizeof(int64_t));if(!v||v->kind!=Kind::Int)fail("store_int expects an integer");*reinterpret_cast<int64_t*>(p->ptr)=v->i;}
extern "C" Value* rt_ptr_load_byte(Value*p){check_raw(p,1);return rt_int(*(reinterpret_cast<unsigned char*>(p->ptr)));}
extern "C" void rt_ptr_store_byte(Value*p,Value*v){check_raw(p,1);if(!v||v->kind!=Kind::Int)fail("store_byte expects an integer");if(v->i<0||v->i>255)fail("byte value out of range");*reinterpret_cast<unsigned char*>(p->ptr)=(unsigned char)v->i;}

extern "C" void rt_print(Value*a){std::cout<<text(a)<<std::endl;}
extern "C" Value* rt_input(Value*p){std::cout<<text(p);std::string s;std::getline(std::cin,s);return rt_str(s.c_str());}
extern "C" Value* rt_to_int(Value*a){if(a->kind==Kind::Int)return a;if(a->kind==Kind::Bool)return rt_int(a->b);try{return rt_int(std::stoll(text(a)));}catch(...){fail("invalid int conversion");return rt_none();}}
extern "C" Value* rt_to_str(Value*a){return rt_str(text(a).c_str());}
extern "C" Value* rt_to_bool(Value*a){return rt_bool(truth(a));}
extern "C" Value* rt_to_float(Value*a){return rt_float(num(a));}
extern "C" Value* rt_len(Value*a){if(a->kind==Kind::String)return rt_int(a->s.size());if(a->kind==Kind::List)return rt_int(a->list.size());if(a->kind==Kind::Dict)return rt_int(a->dict.size());return rt_int(0);}
static int64_t idx(Value*x){if(x->kind!=Kind::Int)fail("index must be an integer");return x->i;}
extern "C" Value* rt_index(Value*a,Value*i){if(a->kind==Kind::Dict){auto it=a->dict.find(text(i));if(it==a->dict.end())fail("dictionary key not found");return it->second;}auto n=idx(i);if(a->kind==Kind::List){if(n<0)n+=a->list.size();if(n<0||n>=(int64_t)a->list.size())fail("list index out of range");return a->list[n];}if(a->kind==Kind::String){if(n<0)n+=a->s.size();if(n<0||n>=(int64_t)a->s.size())fail("string index out of range");return rt_str(std::string(1,a->s[n]).c_str());}fail("object is not indexable");return rt_none();}
extern "C" void rt_set_index(Value*a,Value*i,Value*v){if(a->kind==Kind::Dict){a->dict[text(i)]=v;return;}auto n=idx(i);if(a->kind==Kind::List){if(n<0)n+=a->list.size();if(n<0||n>=(int64_t)a->list.size())fail("list index out of range");a->list[n]=v;return;}fail("object is not assignable by index");}
extern "C" Value* rt_list(int n,...){auto*v=make(Kind::List);va_list ap;va_start(ap,n);for(int i=0;i<n;i++)v->list.push_back(va_arg(ap,Value*));va_end(ap);return v;}
extern "C" Value* rt_dict(int n,...){auto*v=make(Kind::Dict);va_list ap;va_start(ap,n);for(int i=0;i<n;i++){auto*k=va_arg(ap,Value*),*x=va_arg(ap,Value*);v->dict[text(k)]=x;}va_end(ap);return v;}
extern "C" Value* rt_range(int n,...){std::vector<Value*>a;va_list ap;va_start(ap,n);for(int i=0;i<n;i++)a.push_back(va_arg(ap,Value*));va_end(ap);int64_t start=0,stop=0,step=1;if(n==1)stop=a[0]->i;else if(n>=2){start=a[0]->i;stop=a[1]->i;if(n>=3)step=a[2]->i;}if(step==0)fail("range step cannot be zero");auto*v=make(Kind::List);if(step>0)for(int64_t i=start;i<stop;i+=step)v->list.push_back(rt_int(i));else for(int64_t i=start;i>stop;i+=step)v->list.push_back(rt_int(i));return v;}
extern "C" Value* rt_new_object(const char*c){auto*v=make(Kind::Object);v->cls=c?c:"Object";return v;}
extern "C" Value* rt_get_attr(Value*o,const char*n){if(o->kind!=Kind::Object)fail("attribute access on non-object");auto it=o->fields.find(n?n:"");if(it==o->fields.end())return rt_none();return it->second;}
extern "C" void rt_set_attr(Value*o,const char*n,Value*v){if(o->kind!=Kind::Object)fail("attribute assignment on non-object");o->fields[n?n:""]=v;}
extern "C" Value* rt_call_method(Value*,const char*,...){fail("dynamic method dispatch requires a statically known class");return rt_none();}
extern "C" Value* rt_format(Value*t,int n,...){std::string s=text(t),out;va_list ap;va_start(ap,n);size_t pos=0;for(int i=0;i<n;i++){size_t b=s.find('{',pos);size_t e=b==std::string::npos?std::string::npos:s.find('}',b);if(b==std::string::npos||e==std::string::npos)break;out+=s.substr(pos,b-pos);out+=text(va_arg(ap,Value*));pos=e+1;}out+=s.substr(pos);va_end(ap);return rt_str(out.c_str());}

static thread_local jmp_buf* active=nullptr; static thread_local Value* last=nullptr;
static void fail(const std::string&m){last=rt_str(m.c_str());if(active)longjmp(*active,1);std::cerr<<"Tekst runtime error: "<<m<<std::endl;std::exit(1);}
extern "C" int rt_try_begin(){static thread_local jmp_buf env;active=&env;return setjmp(env);}
extern "C" void rt_try_end(){active=nullptr;} extern "C" void rt_throw(Value*v){last=v;if(active)longjmp(*active,1);fail(text(v));} extern "C" Value* rt_last_error(){return last?last:rt_none();}
