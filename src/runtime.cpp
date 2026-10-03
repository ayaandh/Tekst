
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
#include <cctype>
#include <climits>
#include <cmath>
#include <random>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <ctime>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#endif

enum class Kind { None, Int, Float, Bool, String, List, Dict, Object, Pointer };
struct Value {
 Kind kind=Kind::None; int64_t i=0; double f=0; bool b=false; std::string s;
 std::vector<Value*> list; std::unordered_map<std::string,Value*> dict, fields; std::string cls;
 void* ptr=nullptr; void* base=nullptr; size_t size=0; size_t offset=0; bool reference=false; void* callable=nullptr;
};
static std::vector<std::unique_ptr<Value>> arena;
static Value* V(){arena.push_back(std::make_unique<Value>());return arena.back().get();}
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
static void need(Value*v,Kind k,const char*name){if(!v||v->kind!=k)fail(std::string(name)+" expects "+(k==Kind::Int?"integer":k==Kind::String?"string":"value"));}
static double num(Value*v){if(!v)fail("null value");if(v->kind==Kind::Int)return(double)v->i;if(v->kind==Kind::Float)return v->f;if(v->kind==Kind::Bool)return v->b;fail("expected number");return 0;}
extern "C" Value* rt_add(Value*a,Value*b){if(a->kind==Kind::Pointer&&b->kind==Kind::Int)return rt_ptr_add(a,b);if(a->kind==Kind::String||b->kind==Kind::String)return rt_str((text(a)+text(b)).c_str());if(a->kind==Kind::Float||b->kind==Kind::Float)return rt_float(num(a)+num(b));return rt_int((int64_t)(num(a)+num(b)));}
extern "C" Value* rt_sub(Value*a,Value*b){if(a->kind==Kind::Pointer&&b->kind==Kind::Int){auto neg=rt_int(-b->i);return rt_ptr_add(a,neg);}if(a->kind==Kind::Float||b->kind==Kind::Float)return rt_float(num(a)-num(b));return rt_int((int64_t)(num(a)-num(b)));}
extern "C" Value* rt_mul(Value*a,Value*b){if(a->kind==Kind::String&&b->kind==Kind::Int){std::string s;for(int64_t i=0;i<b->i;i++)s+=a->s;return rt_str(s.c_str());}if(a->kind==Kind::Float||b->kind==Kind::Float)return rt_float(num(a)*num(b));return rt_int((int64_t)(num(a)*num(b)));}
extern "C" Value* rt_div(Value*a,Value*b){double d=num(b);if(d==0)fail("division by zero");if(a->kind==Kind::Int&&b->kind==Kind::Int){if(a->i==INT64_MIN&&b->i==-1)fail("integer overflow");if(a->i%b->i==0)return rt_int(a->i/b->i);}return rt_float(num(a)/d);}
extern "C" Value* rt_mod(Value*a,Value*b){need(a,Kind::Int,"modulo");need(b,Kind::Int,"modulo");if(b->i==0)fail("division by zero");if(a->i==INT64_MIN&&b->i==-1)fail("integer overflow");return rt_int(a->i%b->i);}
static bool equal(Value*a,Value*b){if(a->kind!=b->kind){if((a->kind==Kind::Int||a->kind==Kind::Float)&&(b->kind==Kind::Int||b->kind==Kind::Float))return num(a)==num(b);return false;}if(a->kind==Kind::None)return true;if(a->kind==Kind::Int)return a->i==b->i;if(a->kind==Kind::Float)return a->f==b->f;if(a->kind==Kind::Bool)return a->b==b->b;if(a->kind==Kind::String)return a->s==b->s;return a==b;}
extern "C" Value* rt_eq(Value*a,Value*b){return rt_bool(equal(a,b));} extern "C" Value* rt_ne(Value*a,Value*b){return rt_bool(!equal(a,b));}
static int cmp(Value*a,Value*b){if(a->kind==Kind::String&&b->kind==Kind::String)return a->s.compare(b->s);double x=num(a),y=num(b);return x<y?-1:x>y?1:0;}
extern "C" Value* rt_lt(Value*a,Value*b){return rt_bool(cmp(a,b)<0);} extern "C" Value* rt_le(Value*a,Value*b){return rt_bool(cmp(a,b)<=0);} extern "C" Value* rt_gt(Value*a,Value*b){return rt_bool(cmp(a,b)>0);} extern "C" Value* rt_ge(Value*a,Value*b){return rt_bool(cmp(a,b)>=0);}
extern "C" Value* rt_and(Value*a,Value*b){return rt_bool(truth(a)&&truth(b));}
extern "C" Value* rt_or(Value*a,Value*b){return rt_bool(truth(a)||truth(b));}
extern "C" Value* rt_neg(Value*a){if(!a)fail("unary minus on null");if(a->kind==Kind::Float)return rt_float(-a->f);need(a,Kind::Int,"unary minus");if(a->i==INT64_MIN)fail("integer overflow");return rt_int(-a->i);}
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
 if(n->i>static_cast<int64_t>(p->size)||n->i< -static_cast<int64_t>(p->offset))fail("pointer arithmetic out of bounds");int64_t next=static_cast<int64_t>(p->offset)+n->i;if(next<0||static_cast<uint64_t>(next)>p->size)fail("pointer arithmetic out of bounds"); auto*v=make(Kind::Pointer);v->ptr=(void*)((char*)p->base+next);v->base=p->base;v->size=p->size;v->offset=(size_t)next;v->reference=p->reference;return v;
}
static void check_raw(Value*p,size_t bytes){if(!p||p->kind!=Kind::Pointer||!p->ptr||p->reference)fail("raw memory pointer required");if(p->offset>p->size||bytes>p->size-p->offset)fail("pointer access out of bounds");}
extern "C" Value* rt_ptr_load_int(Value*p){check_raw(p,sizeof(int64_t));return rt_int(*reinterpret_cast<int64_t*>(p->ptr));}
extern "C" void rt_ptr_store_int(Value*p,Value*v){check_raw(p,sizeof(int64_t));if(!v||v->kind!=Kind::Int)fail("store_int expects an integer");*reinterpret_cast<int64_t*>(p->ptr)=v->i;}
extern "C" Value* rt_ptr_load_byte(Value*p){check_raw(p,1);return rt_int(*(reinterpret_cast<unsigned char*>(p->ptr)));}
extern "C" void rt_ptr_store_byte(Value*p,Value*v){check_raw(p,1);if(!v||v->kind!=Kind::Int)fail("store_byte expects an integer");if(v->i<0||v->i>255)fail("byte value out of range");*reinterpret_cast<unsigned char*>(p->ptr)=(unsigned char)v->i;}

extern "C" void rt_print(Value*a){std::cout<<text(a)<<std::endl;}
extern "C" void rt_print_many(int argc,...){va_list ap;va_start(ap,argc);for(int i=0;i<argc;i++){if(i)std::cout<<" ";std::cout<<text(va_arg(ap,Value*));}va_end(ap);std::cout<<std::endl;}
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
extern "C" Value* rt_range(int n,...){std::vector<Value*>a;va_list ap;va_start(ap,n);for(int i=0;i<n;i++)a.push_back(va_arg(ap,Value*));va_end(ap);int64_t start=0,stop=0,step=1;if(n<1||n>3)fail("range expects one to three integers");for(auto*x:a)need(x,Kind::Int,"range");if(n==1)stop=a[0]->i;else if(n>=2){start=a[0]->i;stop=a[1]->i;if(n>=3)step=a[2]->i;}if(step==0)fail("range step cannot be zero");auto*v=make(Kind::List);if(step>0)for(int64_t i=start;i<stop;i+=step)v->list.push_back(rt_int(i));else for(int64_t i=start;i>stop;i+=step)v->list.push_back(rt_int(i));return v;}
extern "C" Value* rt_list_empty(){return make(Kind::List);}
extern "C" void rt_list_push(Value*v,Value*x){if(!v||v->kind!=Kind::List)fail("list.push expects a list");v->list.push_back(x);}
extern "C" Value* rt_string_method(Value*o,const char*n,int argc,...){
 if(!o||o->kind!=Kind::String)fail("string method requires a string");
 std::string m=n?n:"";std::vector<Value*> a;va_list ap;va_start(ap,argc);for(int i=0;i<argc;i++)a.push_back(va_arg(ap,Value*));va_end(ap);
 if(m=="upper")return rt_str(std::string(o->s).replace(0,o->s.size(),[&]{std::string x=o->s;std::transform(x.begin(),x.end(),x.begin(),[](unsigned char c){return (char)std::toupper(c);});return x;}()).c_str());
 if(m=="lower"){std::string x=o->s;std::transform(x.begin(),x.end(),x.begin(),[](unsigned char c){return (char)std::tolower(c);});return rt_str(x.c_str());}
 if(m=="contains"){if(a.size()!=1)fail("contains expects one argument");return rt_bool(o->s.find(text(a[0]))!=std::string::npos);}
 if(m=="split"){std::string sep=a.empty()?" ":text(a[0]);auto out=make(Kind::List);size_t p=0,q;if(sep.empty())for(char c:o->s)out->list.push_back(rt_str(std::string(1,c).c_str()));else while((q=o->s.find(sep,p))!=std::string::npos){out->list.push_back(rt_str(o->s.substr(p,q-p).c_str()));p=q+sep.size();}if(!sep.empty())out->list.push_back(rt_str(o->s.substr(p).c_str()));return out;}
 if(m=="replace"){if(a.size()!=2)fail("replace expects two arguments");std::string x=o->s,from=text(a[0]),to=text(a[1]);if(from.empty())return rt_str(x.c_str());size_t p=0;while((p=x.find(from,p))!=std::string::npos){x.replace(p,from.size(),to);p+=to.size();}return rt_str(x.c_str());}
 fail("unknown string method "+m);return rt_none();
}
extern "C" Value* rt_list_method(Value*o,const char*n,int argc,...){
 if(!o||o->kind!=Kind::List)fail("list method requires a list");
 std::string m=n?n:"";std::vector<Value*> a;va_list ap;va_start(ap,argc);for(int i=0;i<argc;i++)a.push_back(va_arg(ap,Value*));va_end(ap);
 if(m=="length"||m=="len"){if(argc)fail("length takes no arguments");return rt_int(o->list.size());}
 if(m=="contains"){if(argc!=1)fail("contains expects one argument");for(auto*x:o->list)if(equal(x,a[0]))return rt_bool(true);return rt_bool(false);}
 if(m=="push"){if(argc!=1)fail("push expects one argument");o->list.push_back(a[0]);return rt_none();}
 if(m=="pop"){if(argc!=0)fail("pop takes no arguments");if(o->list.empty())fail("pop from empty list");auto*x=o->list.back();o->list.pop_back();return x;}
 if(m=="remove"){if(argc!=1)fail("remove expects one argument");auto it=std::find_if(o->list.begin(),o->list.end(),[&](auto*x){return equal(x,a[0]);});if(it==o->list.end())fail("list item not found");o->list.erase(it);return rt_none();}
 fail("unknown list method "+m);return rt_none();
}
extern "C" Value* rt_callable(void*f){auto*v=make(Kind::Object);v->cls="Function";v->callable=f;return v;}
extern "C" Value* rt_call_callable(Value*f,int argc,...){
 if(!f||f->callable==nullptr)fail("value is not callable");
 std::vector<Value*> a;va_list ap;va_start(ap,argc);for(int i=0;i<argc;i++)a.push_back(va_arg(ap,Value*));va_end(ap);
 switch(argc){
 case 0:return reinterpret_cast<Value*(*)()>(f->callable)();
 case 1:return reinterpret_cast<Value*(*) (Value*)>(f->callable)(a[0]);
 case 2:return reinterpret_cast<Value*(*) (Value*,Value*)>(f->callable)(a[0],a[1]);
 case 3:return reinterpret_cast<Value*(*) (Value*,Value*,Value*)>(f->callable)(a[0],a[1],a[2]);
 case 4:return reinterpret_cast<Value*(*) (Value*,Value*,Value*,Value*)>(f->callable)(a[0],a[1],a[2],a[3]);
 default:fail("lambda supports at most four arguments");
 }
 return rt_none();
}
extern "C" Value* rt_new_object(const char*c){auto*v=make(Kind::Object);v->cls=c?c:"Object";return v;}
extern "C" Value* rt_get_attr(Value*,const char*);
extern "C" Value* rt_optional_attr(Value*o,const char*n){if(!o||o->kind==Kind::None)return rt_none();return rt_get_attr(o,n);}
extern "C" Value* rt_get_attr(Value*o,const char*n){if(o->kind==Kind::String){if(!n)fail("missing attribute");return rt_none();}if(o->kind==Kind::List){if(!n)fail("missing attribute");return rt_none();}if(o->kind!=Kind::Object)fail("attribute access on non-object");auto it=o->fields.find(n?n:"");if(it==o->fields.end())return rt_none();return it->second;}
extern "C" void rt_set_attr(Value*o,const char*n,Value*v){if(o->kind!=Kind::Object)fail("attribute assignment on non-object");o->fields[n?n:""]=v;}
extern "C" Value* rt_call_method(Value*o,const char*n,...){
 if(o&&o->kind==Kind::String){va_list ap;va_start(ap,n);std::vector<Value*> a;Value*x;while((x=va_arg(ap,Value*))!=nullptr)a.push_back(x);va_end(ap);Value*r;switch(a.size()){case 0:r=rt_string_method(o,n,0);break;case 1:r=rt_string_method(o,n,1,a[0]);break;case 2:r=rt_string_method(o,n,2,a[0],a[1]);break;default:fail("too many method arguments");}return r;}
 if(o&&o->kind==Kind::List){va_list ap;va_start(ap,n);std::vector<Value*> a;Value*x;while((x=va_arg(ap,Value*))!=nullptr)a.push_back(x);va_end(ap);if(a.size()==0)return rt_list_method(o,n,0);if(a.size()==1)return rt_list_method(o,n,1,a[0]);fail("too many method arguments");}
 fail("dynamic method dispatch requires a supported receiver");return rt_none();
}
extern "C" Value* rt_format(Value*t,int n,...){std::string s=text(t),out;va_list ap;va_start(ap,n);size_t pos=0;for(int i=0;i<n;i++){size_t b=s.find('{',pos);size_t e=b==std::string::npos?std::string::npos:s.find('}',b);if(b==std::string::npos||e==std::string::npos)break;out+=s.substr(pos,b-pos);out+=text(va_arg(ap,Value*));pos=e+1;}out+=s.substr(pos);va_end(ap);return rt_str(out.c_str());}

static thread_local jmp_buf* active=nullptr; static thread_local Value* last=nullptr;
static void fail(const std::string&m){last=rt_str(m.c_str());if(active)longjmp(*active,1);std::cerr<<"Tekst runtime error: "<<m<<std::endl;std::exit(1);}
static std::mt19937_64& std_rng(){
 static std::mt19937_64 g([]{
   std::random_device rd;
   std::seed_seq seq{rd(),rd(),rd(),rd()};
   return std::mt19937_64(seq);
 }());
 return g;
}
static Value* std_num(double x){
 return rt_float(x);
}
static Value* std_math(const std::string& n,int argc,va_list& ap){
 auto arg=[&](int i)->Value*{ (void)i; return va_arg(ap,Value*); };
 if(n=="pi" || n=="e" || n=="tau"){
   if(argc!=0) fail("math."+n+" takes no arguments");
   if(n=="pi") return std_num(3.14159265358979323846);
   if(n=="e") return std_num(2.71828182845904523536);
   return std_num(6.28318530717958647692);
 }
 if(n=="sqrt"||n=="cbrt"||n=="abs"||n=="floor"||n=="ceil"||n=="round"||
    n=="sin"||n=="cos"||n=="tan"||n=="asin"||n=="acos"||n=="atan"||
    n=="log"||n=="log10"||n=="exp"){
   if(argc!=1) fail("math."+n+" takes one argument");
   Value* a=arg(0); double x=num(a);
   if(n=="sqrt") return std_num(std::sqrt(x));
   if(n=="cbrt") return std_num(std::cbrt(x));
   if(n=="abs") return a->kind==Kind::Int ? rt_int(a->i==INT64_MIN ? (fail("integer overflow"),0) : std::llabs(a->i)) : std_num(std::fabs(x));
   if(n=="floor") return std_num(std::floor(x));
   if(n=="ceil") return std_num(std::ceil(x));
   if(n=="round") return std_num(std::round(x));
   if(n=="sin") return std_num(std::sin(x));
   if(n=="cos") return std_num(std::cos(x));
   if(n=="tan") return std_num(std::tan(x));
   if(n=="asin") return std_num(std::asin(x));
   if(n=="acos") return std_num(std::acos(x));
   if(n=="atan") return std_num(std::atan(x));
   if(n=="log") return std_num(std::log(x));
   if(n=="log10") return std_num(std::log10(x));
   return std_num(std::exp(x));
 }
 if(n=="pow"){
   if(argc!=2) fail("math.pow takes two arguments");
   return std_num(std::pow(num(arg(0)),num(arg(1))));
 }
 if(n=="min"||n=="max"){
   if(argc<1) fail("math."+n+" needs at least one argument");
   Value* r=arg(0);
   for(int i=1;i<argc;i++){Value* x=arg(i); if((n=="min"&&cmp(x,r)<0)||(n=="max"&&cmp(x,r)>0)) r=x;}
   return r;
 }
 fail("unknown math function: "+n); return rt_none();
}
static Value* std_random(const std::string& n,int argc,va_list& ap){
 auto arg=[&](){return va_arg(ap,Value*);};
 if(n=="random"){
   if(argc!=0) fail("random.random takes no arguments");
   std::uniform_real_distribution<double> d(0.0,1.0);
   return rt_float(d(std_rng()));
 }
 if(n=="randint"){
   if(argc!=2) fail("random.randint takes two arguments");
   Value* a=arg(),*b=arg(); need(a,Kind::Int,"random.randint"); need(b,Kind::Int,"random.randint");
   if(a->i>b->i) fail("random.randint lower bound exceeds upper bound");
   std::uniform_int_distribution<int64_t> d(a->i,b->i); return rt_int(d(std_rng()));
 }
 if(n=="randrange"){
   if(argc<1||argc>3) fail("random.randrange takes one to three arguments");
   std::vector<int64_t> x; for(int i=0;i<argc;i++){auto v=arg();need(v,Kind::Int,"random.randrange");x.push_back(v->i);}
   int64_t start=0,stop=0,step=1;
   if(argc==1) stop=x[0]; else {start=x[0];stop=x[1];if(argc==3)step=x[2];}
   if(step==0) fail("random.randrange step cannot be zero");
   int64_t count=0;
   if(step>0){if(start<stop) count=(stop-start+step-1)/step;}
   else {if(start>stop) count=(start-stop+(-step)-1)/(-step);}
   if(count<=0) fail("random.randrange empty range");
   std::uniform_int_distribution<int64_t> d(0,count-1);
   return rt_int(start+d(std_rng())*step);
 }
 if(n=="uniform"){
   if(argc!=2) fail("random.uniform takes two arguments");
   double a=num(arg()),b=num(arg()); std::uniform_real_distribution<double> d(a,b); return rt_float(d(std_rng()));
 }
 if(n=="choice"){
   if(argc!=1) fail("random.choice takes one argument");
   Value* v=arg(); if(v->kind!=Kind::List) fail("random.choice expects a list");
   if(v->list.empty()) fail("random.choice cannot choose from an empty list");
   std::uniform_int_distribution<size_t> d(0,v->list.size()-1); return v->list[d(std_rng())];
 }
 if(n=="seed"){
   if(argc!=1) fail("random.seed takes one argument");
   Value* v=arg(); need(v,Kind::Int,"random.seed"); std_rng().seed((uint64_t)v->i); return rt_none();
 }
 fail("unknown random function: "+n); return rt_none();
}
static Value* std_fs(const std::string& n,int argc,va_list& ap){
 auto arg=[&](){return va_arg(ap,Value*);};
 auto path=[&]()->std::string{auto v=arg();if(v->kind!=Kind::String)fail("fs path must be a string");return v->s;};
 try {
   if(n=="exists"||n=="is_file"||n=="is_dir"){
     if(argc!=1) fail("fs."+n+" takes one argument");
     auto p=path(); bool r=n=="exists"?std::filesystem::exists(p):(n=="is_file"?std::filesystem::is_regular_file(p):std::filesystem::is_directory(p));
     return rt_bool(r);
   }
   if(n=="read"){
     if(argc!=1) fail("fs.read takes one argument");
     auto p=path(); std::ifstream in(p,std::ios::binary); if(!in) fail("cannot read file: "+p);
     std::ostringstream s;s<<in.rdbuf();return rt_str(s.str().c_str());
   }
   if(n=="write"||n=="append"){
     if(argc!=2) fail("fs."+n+" takes two arguments");
     auto p=path();auto v=arg();if(v->kind!=Kind::String)fail("fs."+n+" data must be a string");
     std::ofstream out(p,std::ios::binary|(n=="append"?std::ios::app:std::ios::trunc));if(!out)fail("cannot write file: "+p);out<<v->s;return rt_none();
   }
   if(n=="mkdir"){
     if(argc!=1) fail("fs.mkdir takes one argument");
     return rt_bool(std::filesystem::create_directories(path()));
   }
   if(n=="remove"){
     if(argc!=1) fail("fs.remove takes one argument");
     return rt_bool(std::filesystem::remove_all(path())>0);
   }
   if(n=="list"){
     if(argc!=1) fail("fs.list takes one argument");
     auto p=path(); Value* out=rt_list_empty(); if(!std::filesystem::is_directory(p)) fail("fs.list expects a directory");
     std::vector<std::string> names; for(auto const&e:std::filesystem::directory_iterator(p)) names.push_back(e.path().filename().string());
     std::sort(names.begin(),names.end()); for(auto&s:names)rt_list_push(out,rt_str(s.c_str())); return out;
   }
 } catch(const std::filesystem::filesystem_error& e){fail(e.what());}
 fail("unknown fs function: "+n); return rt_none();
}
static Value* std_time(const std::string& n,int argc,va_list& ap){
 if(n=="timestamp"){
   if(argc!=0) fail("time.timestamp takes no arguments");
   return rt_float(std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count());
 }
 if(n=="now"){
   if(argc!=0) fail("time.now takes no arguments");
   std::time_t t=std::time(nullptr); std::tm tm{};
#ifdef _WIN32
   localtime_s(&tm,&t);
#else
   localtime_r(&t,&tm);
#endif
   char buf[64];std::strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",&tm);return rt_str(buf);
 }
 if(n=="sleep"){
   if(argc!=1) fail("time.sleep takes one argument");
   double seconds=num(va_arg(ap,Value*));
   if(seconds<0) fail("time.sleep duration cannot be negative");
   auto deadline=std::chrono::steady_clock::now()+std::chrono::duration<double>(seconds);
   while(std::chrono::steady_clock::now()<deadline) {}
   return rt_none();
 }
 fail("unknown time function: "+n);return rt_none();
}
static Value* std_os(const std::string& n,int argc,va_list& ap){
 auto arg=[&](){return va_arg(ap,Value*);};
 if(n=="cwd"){
   if(argc!=0) fail("os.cwd takes no arguments");
   return rt_str(std::filesystem::current_path().string().c_str());
 }
 if(n=="chdir"){
   if(argc!=1) fail("os.chdir takes one argument");
   auto v=arg();if(v->kind!=Kind::String)fail("os.chdir expects a string");std::filesystem::current_path(v->s);return rt_none();
 }
 if(n=="env"){
   if(argc!=1) fail("os.env takes one argument");
   auto v=arg();if(v->kind!=Kind::String)fail("os.env expects a string");
   const char* x=std::getenv(v->s.c_str());return x?rt_str(x):rt_none();
 }
 fail("unknown os function: "+n);return rt_none();
}

struct HttpRoute { std::string method; std::string path; Value* handler=nullptr; };
struct HttpStatic { std::string prefix; std::string directory; };
struct HttpContext { Value* request=nullptr; size_t middleware=0; };
static std::vector<HttpRoute> http_routes;
static std::vector<Value*> http_middleware;
static std::vector<HttpStatic> http_static;
static thread_local std::vector<HttpContext> http_contexts;

static std::string http_lower(std::string s){
 std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return (char)std::tolower(c);});
 return s;
}
static std::string http_trim(const std::string& s){
 size_t a=0,b=s.size();
 while(a<b&&std::isspace((unsigned char)s[a]))++a;
 while(b>a&&std::isspace((unsigned char)s[b-1]))--b;
 return s.substr(a,b-a);
}
static std::string http_decode(const std::string& s){
 std::string out;
 for(size_t i=0;i<s.size();++i){
  if(s[i]=='+' ){out+=' ';continue;}
  if(s[i]=='%'&&i+2<s.size()){
   auto hex=[](char c)->int{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;};
   int a=hex(s[i+1]),b=hex(s[i+2]);
   if(a>=0&&b>=0){out.push_back((char)(a*16+b));i+=2;continue;}
  }
  out+=s[i];
 }
 return out;
}
static Value* http_dict(){return rt_dict(0);}
static void http_dict_set(Value* d,const std::string& k,const std::string& v){d->dict[k]=rt_str(v.c_str());}
static Value* http_parse_pairs(const std::string& input){
 auto d=http_dict();
 size_t p=0;
 while(p<=input.size()){
  size_t e=input.find('&',p);if(e==std::string::npos)e=input.size();
  auto part=input.substr(p,e-p);size_t eq=part.find('=');
  std::string k=eq==std::string::npos?part:part.substr(0,eq);
  std::string v=eq==std::string::npos?"":part.substr(eq+1);
  if(!k.empty())http_dict_set(d,http_decode(k),http_decode(v));
  if(e==input.size())break;p=e+1;
 }
 return d;
}
static std::string http_json_escape(const std::string& s){
 std::string o="\"";
 for(unsigned char c:s){
  switch(c){case '\\':o+="\\\\";break;case '"':o+="\\\"";break;case '\n':o+="\\n";break;case '\r':o+="\\r";break;case '\t':o+="\\t";break;default:if(c<32){char b[7];std::snprintf(b,sizeof(b),"\\u%04x",c);o+=b;}else o+=(char)c;}
 }
 o+='"';return o;
}
static std::string http_json(Value* v){
 if(!v)return "null";
 switch(v->kind){
  case Kind::None:return "null";
  case Kind::Int:return std::to_string(v->i);
  case Kind::Float:{std::ostringstream o;o<<v->f;return o.str();}
  case Kind::Bool:return v->b?"true":"false";
  case Kind::String:return http_json_escape(v->s);
  case Kind::List:{std::string o="[";for(size_t i=0;i<v->list.size();++i){if(i)o+=',';o+=http_json(v->list[i]);}return o+"]";}
  case Kind::Dict:{std::string o="{";size_t i=0;for(auto&[k,x]:v->dict){if(i++)o+=',';o+=http_json_escape(k);o+=':';o+=http_json(x);}return o+"}";}
  default:return http_json_escape(text(v));
 }
}

class JsonParser {
    const std::string& s;
    size_t p=0;
    void ws(){while(p<s.size()&&std::isspace((unsigned char)s[p]))++p;}
    void error(const std::string& m){fail("json decode error: "+m);}
    bool take(char c){ws();if(p<s.size()&&s[p]==c){++p;return true;}return false;}
    Value* value(){
        ws();
        if(p>=s.size()) error("unexpected end of input");
        char c=s[p];
        if(c=='n'){if(s.compare(p,4,"null")==0){p+=4;return rt_none();}error("invalid value");}
        if(c=='t'){if(s.compare(p,4,"true")==0){p+=4;return rt_bool(true);}error("invalid value");}
        if(c=='f'){if(s.compare(p,5,"false")==0){p+=5;return rt_bool(false);}error("invalid value");}
        if(c=='"') return stringValue();
        if(c=='[') return arrayValue();
        if(c=='{') return objectValue();
        if(c=='-'||std::isdigit((unsigned char)c)) return numberValue();
        error("unexpected character");
        return rt_none();
    }
    Value* stringValue(){
        if(p>=s.size()||s[p]!='"') error("expected string");
        ++p;std::string out;
        while(p<s.size()){
            char c=s[p++];
            if(c=='"') return rt_str(out.c_str());
            if(c=='\\'){
                if(p>=s.size()) error("unterminated escape");
                char e=s[p++];
                switch(e){
                    case '"':out+='"';break;case '\\':out+='\\';break;case '/':out+='/';break;
                    case 'b':out+='\b';break;case 'f':out+='\f';break;case 'n':out+='\n';break;case 'r':out+='\r';break;case 't':out+='\t';break;
                    case 'u':{
                        if(p+4>s.size()) error("invalid unicode escape");
                        unsigned code=0;for(int i=0;i<4;i++){char h=s[p++];code<<=4;if(h>='0'&&h<='9')code+=h-'0';else if(h>='a'&&h<='f')code+=h-'a'+10;else if(h>='A'&&h<='F')code+=h-'A'+10;else error("invalid unicode escape");}
                        if(code<=0x7f)out.push_back((char)code);else if(code<=0x7ff){out.push_back((char)(0xc0|(code>>6)));out.push_back((char)(0x80|(code&0x3f)));}else{out.push_back((char)(0xe0|(code>>12)));out.push_back((char)(0x80|((code>>6)&0x3f)));out.push_back((char)(0x80|(code&0x3f)));}
                        break;
                    }
                    default:error("invalid escape");
                }
            } else out+=c;
        }
        error("unterminated string");return rt_none();
    }
    Value* numberValue(){
        size_t start=p;
        if(s[p]=='-')++p;
        if(p>=s.size()||!std::isdigit((unsigned char)s[p])) error("invalid number");
        if(s[p]=='0')++p;else while(p<s.size()&&std::isdigit((unsigned char)s[p]))++p;
        bool floating=false;
        if(p<s.size()&&s[p]=='.'){floating=true;++p;if(p>=s.size()||!std::isdigit((unsigned char)s[p]))error("invalid number");while(p<s.size()&&std::isdigit((unsigned char)s[p]))++p;}
        if(p<s.size()&&(s[p]=='e'||s[p]=='E')){floating=true;++p;if(p<s.size()&&(s[p]=='+'||s[p]=='-'))++p;if(p>=s.size()||!std::isdigit((unsigned char)s[p]))error("invalid exponent");while(p<s.size()&&std::isdigit((unsigned char)s[p]))++p;}
        auto raw=s.substr(start,p-start);try{return floating?rt_float(std::stod(raw)):rt_int(std::stoll(raw));}catch(...){error("number out of range");return rt_none();}
    }
    Value* arrayValue(){
        ++p;auto out=rt_list_empty();ws();if(take(']'))return out;
        while(true){rt_list_push(out,value());ws();if(take(']'))return out;if(!take(','))error("expected ',' or ']'");}
    }
    Value* objectValue(){
        ++p;auto out=rt_dict(0);ws();if(take('}'))return out;
        while(true){ws();auto key=stringValue();ws();if(!take(':'))error("expected ':'");out->dict[key->s]=value();ws();if(take('}'))return out;if(!take(','))error("expected ',' or '}'");}
    }
public:
    explicit JsonParser(const std::string& x):s(x){}
    Value* parse(){auto v=value();ws();if(p!=s.size())error("unexpected trailing data");return v;}
};

static Value* std_json(const std::string& n,int argc,va_list& ap){
    auto arg=[&](){return va_arg(ap,Value*);};
    if(n=="decode"||n=="parse"){
        if(argc!=1)fail("json."+n+" expects one string");
        auto x=arg();need(x,Kind::String,"json decode input");return JsonParser(x->s).parse();
    }
    if(n=="encode"||n=="stringify"){
        if(argc!=1)fail("json."+n+" expects one value");
        return rt_str(http_json(arg()).c_str());
    }
    if(n=="pretty"){
        if(argc!=1)fail("json.pretty expects one value");
        std::string raw=http_json(arg()),out;int depth=0;bool inString=false,escape=false;
        for(char c:raw){
            if(inString){out+=c;if(escape)escape=false;else if(c=='\\')escape=true;else if(c=='"')inString=false;continue;}
            if(c=='"'){inString=true;out+=c;continue;}
            if(c=='{'||c=='['){out+=c;++depth;out+='\n';for(int i=0;i<depth;i++)out+="  ";}
            else if(c=='}'||c==']'){--depth;out+='\n';for(int i=0;i<depth;i++)out+="  ";out+=c;}
            else if(c==','){out+=",\n";for(int i=0;i<depth;i++)out+="  ";}
            else if(c==':'){out+=": ";}
            else out+=c;
        }
        return rt_str(out.c_str());
    }
    fail("unknown json function: "+n);return rt_none();
}

static Value* http_response(int status,const std::string& body,const std::string& contentType="text/plain; charset=utf-8"){
 auto r=rt_new_object("HttpResponse");r->fields["status"]=rt_int(status);r->fields["body"]=rt_str(body.c_str());auto h=http_dict();http_dict_set(h,"Content-Type",contentType);r->fields["headers"]=h;return r;
}
static bool http_is_response(Value* v){return v&&v->kind==Kind::Object&&v->cls=="HttpResponse";}
static void http_add_header(Value* response,const std::string& k,const std::string& v){
 auto it=response->fields.find("headers");if(it==response->fields.end()){response->fields["headers"]=http_dict();it=response->fields.find("headers");}
it->second->dict[k]=rt_str(v.c_str());
}
static Value* http_request(const std::string& method,const std::string& target,const std::string& clientIp,const std::unordered_map<std::string,std::string>& headers,const std::string& body){
 auto r=rt_new_object("HttpRequest");size_t q=target.find('?');std::string path=q==std::string::npos?target:target.substr(0,q);std::string query=q==std::string::npos?"":target.substr(q+1);
 r->fields["method"]=rt_str(method.c_str());r->fields["path"]=rt_str(http_decode(path).c_str());r->fields["query"]=http_parse_pairs(query);r->fields["headers"]=http_dict();r->fields["body"]=rt_str(body.c_str());r->fields["ip"]=rt_str(clientIp.c_str());r->fields["cookies"]=http_dict();
 for(auto&[k,v]:headers){r->fields["headers"]->dict[k]=rt_str(v.c_str());if(http_lower(k)=="cookie"){size_t p=0;while(p<v.size()){size_t e=v.find(';',p);if(e==std::string::npos)e=v.size();auto part=http_trim(v.substr(p,e-p));size_t eq=part.find('=');if(eq!=std::string::npos)http_dict_set(r->fields["cookies"],http_trim(part.substr(0,eq)),http_trim(part.substr(eq+1)));if(e==v.size())break;p=e+1;}}}
 return r;
}
static Value* http_dispatch(Value* req,size_t start);
static Value* http_next(Value* req){
 if(http_contexts.empty())return http_dispatch(req,http_middleware.size());
 return http_dispatch(req,http_contexts.back().middleware+1);
}
static Value* http_dispatch(Value* req,size_t start){
 if(start<http_middleware.size()){
  auto fn=http_middleware[start];http_contexts.push_back({req,start});auto next=rt_callable((void*)&http_next);Value* out=rt_call_callable(fn,2,req,next);http_contexts.pop_back();return out;
 }
 auto method=req->fields["method"]->s,path=req->fields["path"]->s;
 for(auto& route:http_routes)if(route.method==method&&route.path==path)return rt_call_callable(route.handler,1,req);
 for(auto& st:http_static){if(path.rfind(st.prefix,0)!=0)continue;std::string rel=path.substr(st.prefix.size());while(!rel.empty()&&rel.front()=='/')rel.erase(rel.begin());auto file=std::filesystem::path(st.directory)/rel;if(rel.empty())file/= "index.html";std::error_code ec;auto base=std::filesystem::weakly_canonical(st.directory,ec);auto target=std::filesystem::weakly_canonical(file,ec);if(ec||target.string().rfind(base.string(),0)!=0||!std::filesystem::is_regular_file(target,ec))continue;std::ifstream in(target,std::ios::binary);std::ostringstream buf;buf<<in.rdbuf();std::string ext=target.extension().string(),ct="application/octet-stream";if(ext==".html"||ext==".htm")ct="text/html; charset=utf-8";else if(ext==".css")ct="text/css; charset=utf-8";else if(ext==".js")ct="text/javascript; charset=utf-8";else if(ext==".json")ct="application/json";else if(ext==".txt")ct="text/plain; charset=utf-8";return http_response(200,buf.str(),ct);}
 return http_response(404,"Not Found");
}
static std::string http_recv_request(
#ifdef _WIN32
 SOCKET client
#else
 int client
#endif
){
 std::string data;char buffer[8192];size_t want=0;
 for(int n=0;n<64;++n){
#ifdef _WIN32
  int got=recv(client,buffer,sizeof(buffer),0);
#else
  int got=(int)recv(client,buffer,sizeof(buffer),0);
#endif
  if(got<=0)break;data.append(buffer,buffer+got);auto sep=data.find("\r\n\r\n");if(sep!=std::string::npos){auto head=data.substr(0,sep);auto pos=head.find("\r\n\r\n");(void)pos;std::istringstream hs(head);std::string line;std::getline(hs,line);while(std::getline(hs,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();auto c=line.find(':');if(c!=std::string::npos&&http_lower(http_trim(line.substr(0,c)))=="content-length"){try{want=(size_t)std::stoull(http_trim(line.substr(c+1)));}catch(...){want=0;}}}size_t bodyStart=sep+4;if(data.size()-bodyStart>=want)break;}
 }
 return data;
}
static std::unordered_map<std::string,std::string> http_headers(const std::string& head){
 std::unordered_map<std::string,std::string> h;std::istringstream in(head);std::string line;std::getline(in,line);while(std::getline(in,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();auto p=line.find(':');if(p!=std::string::npos)h[http_trim(line.substr(0,p))]=http_trim(line.substr(p+1));}return h;
}
static std::string http_reason(int status){switch(status){case 200:return "OK";case 201:return "Created";case 204:return "No Content";case 301:return "Moved Permanently";case 302:return "Found";case 400:return "Bad Request";case 401:return "Unauthorized";case 403:return "Forbidden";case 404:return "Not Found";case 500:return "Internal Server Error";default:return "OK";}}
static std::string http_packet(Value* response){
 if(!http_is_response(response))response=http_response(200,text(response));int status=200;if(auto it=response->fields.find("status");it!=response->fields.end()&&it->second->kind==Kind::Int)status=(int)it->second->i;std::string body=response->fields.count("body")?text(response->fields["body"]):"";std::string out="HTTP/1.1 "+std::to_string(status)+" "+http_reason(status)+"\r\n";auto hi=response->fields.find("headers");if(hi!=response->fields.end()&&hi->second->kind==Kind::Dict)for(auto&[k,v]:hi->second->dict)out+=k+": "+text(v)+"\r\n";out+="Content-Length: "+std::to_string(body.size())+"\r\nConnection: close\r\n\r\n"+body;return out;
}

#ifdef _WIN32
using SocketHandle = SOCKET;
static constexpr SocketHandle INVALID_SOCKET_HANDLE = INVALID_SOCKET;
#else
using SocketHandle = int;
static constexpr SocketHandle INVALID_SOCKET_HANDLE = -1;
#endif

static void socket_runtime_init(){
#ifdef _WIN32
 static bool initialized=false;
 if(!initialized){WSADATA wsa{};if(WSAStartup(MAKEWORD(2,2),&wsa)!=0)fail("WSAStartup failed");initialized=true;}
#endif
}
static void socket_runtime_close(SocketHandle s){
#ifdef _WIN32
 closesocket(s);
#else
 close(s);
#endif
}
static void socket_set_timeout(SocketHandle s,int seconds){
#ifdef _WIN32
 DWORD ms=(DWORD)seconds*1000;setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&ms),sizeof(ms));setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<const char*>(&ms),sizeof(ms));
#else
 timeval tv{};tv.tv_sec=seconds;setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
#endif
}
static SocketHandle socket_from_value(Value*v){if(!v||v->kind!=Kind::Object||v->cls!="Socket")fail("expected a socket");auto it=v->fields.find("handle");if(it==v->fields.end()||it->second->kind!=Kind::Int||it->second->i<0)fail("socket is closed");return (SocketHandle)it->second->i;}
static Value* socket_value(SocketHandle s){auto v=rt_new_object("Socket");v->fields["handle"]=rt_int((int64_t)s);v->fields["closed"]=rt_bool(false);return v;}
static std::string socket_recv_all(SocketHandle s,int maxBytes){std::string out;out.reserve((size_t)maxBytes);char buf[8192];while((int)out.size()<maxBytes){int want=std::min<int>(sizeof(buf),maxBytes-(int)out.size());
#ifdef _WIN32
 int n=recv(s,buf,want,0);
#else
 int n=(int)recv(s,buf,want,0);
#endif
 if(n<=0)break;out.append(buf,n);if(n<want)break;}return out;}
static int smtp_code(const std::string& response){if(response.size()<3||!std::isdigit((unsigned char)response[0])||!std::isdigit((unsigned char)response[1])||!std::isdigit((unsigned char)response[2]))return 0;return std::stoi(response.substr(0,3));}
static std::string smtp_read(SocketHandle s){return socket_recv_all(s,65536);}
static void smtp_expect(SocketHandle s,const std::string& expected){auto r=smtp_read(s);int code=smtp_code(r);if(code==0||expected.find(std::to_string(code))==std::string::npos)fail("SMTP error: "+(r.empty()?std::string("connection closed"):r));}
static void smtp_send_cmd(SocketHandle s,const std::string& cmd,const std::string& expected){std::string data=cmd+"\r\n";size_t sent=0;while(sent<data.size()){
#ifdef _WIN32
 int n=send(s,data.data()+sent,(int)(data.size()-sent),0);
#else
 int n=(int)send(s,data.data()+sent,data.size()-sent,0);
#endif
 if(n<=0)fail("SMTP send failed");sent+=(size_t)n;}smtp_expect(s,expected);}
static Value* smtp_value(SocketHandle s){auto v=rt_new_object("SMTP");v->fields["handle"]=rt_int((int64_t)s);return v;}
static SocketHandle smtp_handle(Value*v){if(!v||v->kind!=Kind::Object||v->cls!="SMTP")fail("expected an SMTP connection");auto it=v->fields.find("handle");if(it==v->fields.end()||it->second->kind!=Kind::Int||it->second->i<0)fail("SMTP connection is closed");return (SocketHandle)it->second->i;}

static Value* std_socket(const std::string& n,int argc,va_list& ap){
 auto arg=[&](){return va_arg(ap,Value*);};
 if(n=="tcp"||n=="udp"){
  if(argc!=0)fail("socket."+n+" expects no arguments");socket_runtime_init();int type=n=="tcp"?SOCK_STREAM:SOCK_DGRAM;SocketHandle s=socket(AF_INET,type,IPPROTO_IP);if(s==INVALID_SOCKET_HANDLE)fail("could not create "+n+" socket");return socket_value(s);
 }
 if(n=="connect"){
  if(argc<3||argc>4)fail("socket.connect expects socket, host, port, and optional timeout");auto sock=arg(),host=arg(),port=arg();need(host,Kind::String,"socket host");need(port,Kind::Int,"socket port");int timeout=30;if(argc==4){auto t=arg();need(t,Kind::Int,"socket timeout");timeout=(int)t->i;}if(port->i<1||port->i>65535)fail("socket port must be between 1 and 65535");SocketHandle s=socket_from_value(sock);socket_set_timeout(s,timeout);addrinfo hints{};hints.ai_family=AF_UNSPEC;hints.ai_socktype=SOCK_STREAM;hints.ai_protocol=IPPROTO_TCP;addrinfo*res=nullptr;std::string ps=std::to_string(port->i);int rc=getaddrinfo(host->s.c_str(),ps.c_str(),&hints,&res);if(rc!=0)fail("could not resolve host: "+host->s);bool ok=false;for(auto*p=res;p;p=p->ai_next){if(::connect(s,p->ai_addr,(int)p->ai_addrlen)==0){ok=true;break;}}freeaddrinfo(res);if(!ok)fail("could not connect socket");return rt_none();
 }
 if(n=="bind"){
  if(argc!=3)fail("socket.bind expects socket, host, and port");auto sock=arg(),host=arg(),port=arg();need(host,Kind::String,"socket host");need(port,Kind::Int,"socket port");if(port->i<1||port->i>65535)fail("socket port must be between 1 and 65535");SocketHandle s=socket_from_value(sock);int opt=1;
#ifdef _WIN32
 setsockopt(s,SOL_SOCKET,SO_REUSEADDR,reinterpret_cast<const char*>(&opt),sizeof(opt));
#else
 setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
#endif
 sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_port=htons((uint16_t)port->i);if(host->s.empty()||host->s=="0.0.0.0"||host->s=="*")addr.sin_addr.s_addr=htonl(INADDR_ANY);else if(inet_pton(AF_INET,host->s.c_str(),&addr.sin_addr)<=0)fail("socket.bind currently expects an IPv4 address");if(::bind(s,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))<0)fail("could not bind socket");return rt_none();
 }
 if(n=="listen"){if(argc<1||argc>2)fail("socket.listen expects socket and optional backlog");auto sock=arg();int backlog=32;if(argc==2){auto b=arg();need(b,Kind::Int,"socket backlog");backlog=(int)b->i;}if(::listen(socket_from_value(sock),backlog)<0)fail("could not listen on socket");return rt_none();}
 if(n=="accept"){if(argc!=1)fail("socket.accept expects a socket");SocketHandle s=socket_from_value(arg());sockaddr_storage peer{};
#ifdef _WIN32
 int len=sizeof(peer);SocketHandle client=::accept(s,reinterpret_cast<sockaddr*>(&peer),&len);
#else
 socklen_t len=sizeof(peer);SocketHandle client=::accept(s,reinterpret_cast<sockaddr*>(&peer),&len);
#endif
 if(client==INVALID_SOCKET_HANDLE)fail("could not accept socket connection");return socket_value(client);}
 if(n=="send"){if(argc!=2)fail("socket.send expects socket and data");auto sock=arg(),data=arg();need(data,Kind::String,"socket data");SocketHandle s=socket_from_value(sock);size_t sent=0;while(sent<data->s.size()){
#ifdef _WIN32
 int nbytes=::send(s,data->s.data()+sent,(int)(data->s.size()-sent),0);
#else
 int nbytes=(int)::send(s,data->s.data()+sent,data->s.size()-sent,0);
#endif
 if(nbytes<=0)fail("socket send failed");sent+=(size_t)nbytes;}return rt_int((int64_t)sent);}
 if(n=="recv"||n=="receive"){if(argc<1||argc>2)fail("socket.recv expects socket and optional max bytes");auto sock=arg();int maxBytes=65536;if(argc==2){auto m=arg();need(m,Kind::Int,"socket max bytes");maxBytes=(int)m->i;if(maxBytes<1)fail("socket max bytes must be positive");}return rt_str(socket_recv_all(socket_from_value(sock),maxBytes).c_str());}
 if(n=="sendto"){if(argc!=4)fail("socket.sendto expects socket, data, host, and port");auto sock=arg(),data=arg(),host=arg(),port=arg();need(data,Kind::String,"socket data");need(host,Kind::String,"socket host");need(port,Kind::Int,"socket port");if(port->i<1||port->i>65535)fail("socket port must be between 1 and 65535");SocketHandle s=socket_from_value(sock);addrinfo hints{};hints.ai_family=AF_UNSPEC;hints.ai_socktype=SOCK_DGRAM;hints.ai_protocol=IPPROTO_UDP;addrinfo*res=nullptr;std::string ps=std::to_string(port->i);if(getaddrinfo(host->s.c_str(),ps.c_str(),&hints,&res)!=0)fail("could not resolve host: "+host->s);int sent=-1;for(auto*p=res;p;p=p->ai_next){
#ifdef _WIN32
 sent=::sendto(s,data->s.data(),(int)data->s.size(),0,p->ai_addr,(int)p->ai_addrlen);
#else
 sent=(int)::sendto(s,data->s.data(),data->s.size(),0,p->ai_addr,p->ai_addrlen);
#endif
 if(sent>=0)break;}freeaddrinfo(res);if(sent<0)fail("UDP send failed");return rt_int(sent);}
 if(n=="recvfrom"){if(argc<1||argc>2)fail("socket.recvfrom expects socket and optional max bytes");auto sock=arg();int maxBytes=65536;if(argc==2){auto m=arg();need(m,Kind::Int,"socket max bytes");maxBytes=(int)m->i;if(maxBytes<1)fail("socket max bytes must be positive");}SocketHandle s=socket_from_value(sock);std::vector<char> buf((size_t)maxBytes);sockaddr_storage peer{};
#ifdef _WIN32
 int plen=sizeof(peer);int got=::recvfrom(s,buf.data(),maxBytes,0,reinterpret_cast<sockaddr*>(&peer),&plen);
#else
 socklen_t plen=sizeof(peer);int got=(int)::recvfrom(s,buf.data(),maxBytes,0,reinterpret_cast<sockaddr*>(&peer),&plen);
#endif
 if(got<0)fail("UDP receive failed");char h[NI_MAXHOST]{};char serv[NI_MAXSERV]{};if(getnameinfo(reinterpret_cast<sockaddr*>(&peer),plen,h,sizeof(h),serv,sizeof(serv),NI_NUMERICHOST|NI_NUMERICSERV)!=0)fail("could not resolve UDP peer");auto d=rt_dict(0);d->dict["data"]=rt_str(std::string(buf.data(),got).c_str());d->dict["host"]=rt_str(h);d->dict["port"]=rt_int(std::stoll(serv));return d;}
 if(n=="close"){if(argc!=1)fail("socket.close expects a socket");auto sock=arg();SocketHandle s=socket_from_value(sock);socket_runtime_close(s);sock->fields["handle"]=rt_int(-1);sock->fields["closed"]=rt_bool(true);return rt_none();}
 if(n=="timeout"){if(argc!=2)fail("socket.timeout expects socket and seconds");auto sock=arg(),sec=arg();need(sec,Kind::Int,"socket timeout");if(sec->i<0)fail("socket timeout cannot be negative");socket_set_timeout(socket_from_value(sock),(int)sec->i);return rt_none();}
 if(n=="peer"){if(argc!=1)fail("socket.peer expects a socket");auto sock=arg();SocketHandle s=socket_from_value(sock);sockaddr_storage addr{};
#ifdef _WIN32
 int len=sizeof(addr);if(getpeername(s,reinterpret_cast<sockaddr*>(&addr),&len)<0)fail("could not get socket peer");
#else
 socklen_t len=sizeof(addr);if(getpeername(s,reinterpret_cast<sockaddr*>(&addr),&len)<0)fail("could not get socket peer");
#endif
 char host[NI_MAXHOST]{},service[NI_MAXSERV]{};if(getnameinfo(reinterpret_cast<sockaddr*>(&addr),len,host,sizeof(host),service,sizeof(service),NI_NUMERICHOST|NI_NUMERICSERV)!=0)fail("could not resolve socket peer");auto d=rt_dict(0);d->dict["host"]=rt_str(host);d->dict["port"]=rt_int(std::stoll(service));return d;}
 fail("unknown socket function: "+n);return rt_none();
}

static Value* std_smtp(const std::string& n,int argc,va_list& ap){
 auto arg=[&](){return va_arg(ap,Value*);};
 if(n=="connect"){
  if(argc<1||argc>3)fail("smtp.connect expects host and optional port and timeout");auto host=arg();need(host,Kind::String,"smtp host");int port=25,timeout=30;if(argc>=2){auto p=arg();need(p,Kind::Int,"smtp port");port=(int)p->i;}if(argc==3){auto t=arg();need(t,Kind::Int,"smtp timeout");timeout=(int)t->i;}if(port<1||port>65535)fail("smtp port must be between 1 and 65535");socket_runtime_init();SocketHandle s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(s==INVALID_SOCKET_HANDLE)fail("could not create SMTP socket");socket_set_timeout(s,timeout);addrinfo hints{};hints.ai_family=AF_UNSPEC;hints.ai_socktype=SOCK_STREAM;hints.ai_protocol=IPPROTO_TCP;addrinfo*res=nullptr;std::string ps=std::to_string(port);if(getaddrinfo(host->s.c_str(),ps.c_str(),&hints,&res)!=0)fail("could not resolve SMTP host: "+host->s);bool ok=false;for(auto*p=res;p;p=p->ai_next){if(::connect(s,p->ai_addr,(int)p->ai_addrlen)==0){ok=true;break;}}freeaddrinfo(res);if(!ok)fail("could not connect to SMTP server");smtp_expect(s,"220");return smtp_value(s);
 }
 auto send=[&](SocketHandle s,const std::string&cmd,const std::string&codes){smtp_send_cmd(s,cmd,codes);};
 if(n=="ehlo"||n=="helo"){if(argc!=2)fail("smtp."+n+" expects connection and hostname");auto c=arg(),h=arg();need(h,Kind::String,"smtp hostname");send(smtp_handle(c),(n=="ehlo"?"EHLO ":"HELO ")+h->s,"250");return rt_none();}
 if(n=="mail"){if(argc!=2)fail("smtp.mail expects connection and sender");auto c=arg(),from=arg();need(from,Kind::String,"smtp sender");send(smtp_handle(c),"MAIL FROM:<"+from->s+">","250");return rt_none();}
 if(n=="rcpt"){if(argc!=2)fail("smtp.rcpt expects connection and recipient");auto c=arg(),to=arg();need(to,Kind::String,"smtp recipient");send(smtp_handle(c),"RCPT TO:<"+to->s+">","250251");return rt_none();}
 if(n=="data"){if(argc!=4)fail("smtp.data expects connection, from, to, and body");auto c=arg(),from=arg(),to=arg(),body=arg();need(from,Kind::String,"smtp sender");need(to,Kind::String,"smtp recipient");need(body,Kind::String,"smtp body");SocketHandle s=smtp_handle(c);send(s,"MAIL FROM:<"+from->s+">","250");send(s,"RCPT TO:<"+to->s+">","250251");send(s,"DATA","354");std::string msg=body->s;if(!msg.empty()&&msg.back()!='\n')msg+='\n';std::string payload=msg+".\r\n";size_t pos=0;while(pos<payload.size()){
#ifdef _WIN32
 int z=::send(s,payload.data()+pos,(int)(payload.size()-pos),0);
#else
 int z=(int)::send(s,payload.data()+pos,payload.size()-pos,0);
#endif
 if(z<=0)fail("SMTP data send failed");pos+=(size_t)z;}smtp_expect(s,"250");return rt_none();}
 if(n=="send"){if(argc!=5)fail("smtp.send expects connection, sender, recipient, subject, and body");auto c=arg(),from=arg(),to=arg(),subject=arg(),body=arg();need(from,Kind::String,"smtp sender");need(to,Kind::String,"smtp recipient");need(subject,Kind::String,"smtp subject");need(body,Kind::String,"smtp body");SocketHandle s=smtp_handle(c);send(s,"MAIL FROM:<"+from->s+">","250");send(s,"RCPT TO:<"+to->s+">","250251");send(s,"DATA","354");std::string msg="From: "+from->s+"\r\nTo: "+to->s+"\r\nSubject: "+subject->s+"\r\n\r\n"+body->s;if(msg.empty()||msg.back()!='\n')msg+='\n';std::string payload=msg+".\r\n";size_t pos=0;while(pos<payload.size()){
#ifdef _WIN32
 int z=::send(s,payload.data()+pos,(int)(payload.size()-pos),0);
#else
 int z=(int)::send(s,payload.data()+pos,payload.size()-pos,0);
#endif
 if(z<=0)fail("SMTP data send failed");pos+=(size_t)z;}smtp_expect(s,"250");return rt_none();}
 if(n=="quit"||n=="close"){if(argc!=1)fail("smtp."+n+" expects a connection");auto c=arg();SocketHandle s=smtp_handle(c);if(n=="quit")send(s,"QUIT","221");socket_runtime_close(s);c->fields["handle"]=rt_int(-1);return rt_none();}
 if(n=="login"){if(argc!=3)fail("smtp.login expects connection, username, and password");auto c=arg(),u=arg(),pw=arg();need(u,Kind::String,"smtp username");need(pw,Kind::String,"smtp password");SocketHandle s=smtp_handle(c);send(s,"AUTH LOGIN","334");auto b64=[](const std::string& x){static const char* chars="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";std::string o;int val=0,bits=-6;for(unsigned char c:x){val=(val<<8)+c;bits+=8;while(bits>=0){o.push_back(chars[(val>>bits)&0x3F]);bits-=6;}}if(bits>-6)o.push_back(chars[((val<<8)>>(bits+8))&0x3F]);while(o.size()%4)o+='=';return o;};send(s,b64(u->s),"334");send(s,b64(pw->s),"235");return rt_none();}
 if(n=="starttls")fail("smtp.starttls requires a TLS-enabled runtime build");
 fail("unknown smtp function: "+n);return rt_none();
}
static Value* std_http(const std::string& n,int argc,va_list& ap){
 auto arg=[&](){return va_arg(ap,Value*);};
 if(n=="get"||n=="post"||n=="put"||n=="delete"){
  if(argc!=2)fail("http."+n+" expects path and handler");auto path=arg(),handler=arg();need(path,Kind::String,"http route path");if(!handler||handler->callable==nullptr)fail("http route handler must be callable");http_routes.push_back({n=="delete"?"DELETE":std::string(n.size(),(char)0),path->s,handler});http_routes.back().method=n=="get"?"GET":n=="post"?"POST":n=="put"?"PUT":"DELETE";return rt_none();
 }
 if(n=="use"){if(argc!=1)fail("http.use expects a handler");auto fn=arg();if(!fn||fn->callable==nullptr)fail("http middleware must be callable");http_middleware.push_back(fn);return rt_none();}
 if(n=="static"){if(argc<1||argc>2)fail("http.static expects one or two arguments");auto prefix=arg();need(prefix,Kind::String,"http.static prefix");std::string dir=prefix->s;if(argc==2){auto d=arg();need(d,Kind::String,"http.static directory");dir=d->s;}else if(dir.empty()||dir[0]!='/'){}else dir="."+dir;http_static.push_back({prefix->s,dir});return rt_none();}
 if(n=="json"){if(argc<1||argc>2)fail("http.json expects a value and optional status");auto value=arg();int status=200;if(argc==2){auto code=arg();need(code,Kind::Int,"http.json status");status=(int)code->i;}return http_response(status,http_json(value),"application/json; charset=utf-8");}
 if(n=="response"){if(argc<2||argc>3)fail("http.response expects status, body, and optional headers");auto status=arg(),body=arg();need(status,Kind::Int,"http response status");need(body,Kind::String,"http response body");auto r=http_response((int)status->i,body->s);if(argc==3){auto h=arg();if(h->kind!=Kind::Dict)fail("http response headers expects a dictionary");for(auto&[k,v]:h->dict)http_add_header(r,k,text(v));}return r;}
 if(n=="redirect"){if(argc!=1)fail("http.redirect expects a path");auto path=arg();need(path,Kind::String,"http redirect path");auto r=http_response(302,"");http_add_header(r,"Location",path->s);return r;}
 if(n!="serve"&&n!="server")fail("unknown http function: "+n);
 if(argc!=1&&argc!=2)fail("http."+n+" expects port and optional response");auto port=arg();need(port,Kind::Int,"http port");Value* legacy=argc==2?arg():nullptr;if(legacy)need(legacy,Kind::String,"http response");if(port->i<1||port->i>65535)fail("http port must be between 1 and 65535");
#ifdef _WIN32
 WSADATA wsa{};if(WSAStartup(MAKEWORD(2,2),&wsa)!=0)fail("WSAStartup failed");SOCKET server=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(server==INVALID_SOCKET){WSACleanup();fail("could not create HTTP socket");}
#else
 int server=socket(AF_INET,SOCK_STREAM,0);if(server<0)fail("could not create HTTP socket");int opt=1;setsockopt(server,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
#endif
 sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_ANY);addr.sin_port=htons((uint16_t)port->i);
 if(bind(server,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))<0){
#ifdef _WIN32
  closesocket(server);WSACleanup();
#else
  close(server);
#endif
  fail("could not bind HTTP port");}
 if(listen(server,32)<0){
#ifdef _WIN32
  closesocket(server);WSACleanup();
#else
  close(server);
#endif
  fail("could not listen on HTTP port");}
 for(;;){
#ifdef _WIN32
  sockaddr_in peer{};int plen=sizeof(peer);SOCKET client=accept(server,(sockaddr*)&peer,&plen);if(client==INVALID_SOCKET)break;
#else
  sockaddr_in peer{};socklen_t plen=sizeof(peer);int client=accept(server,(sockaddr*)&peer,&plen);if(client<0)break;
#endif
  std::string raw=http_recv_request(client);auto sep=raw.find("\r\n\r\n");if(sep==std::string::npos){
#ifdef _WIN32
   closesocket(client);
#else
   close(client);
#endif
   continue;
  }
  std::string head=raw.substr(0,sep),body=raw.substr(sep+4);std::istringstream first(head);std::string method,target,version;first>>method>>target>>version;auto headers=http_headers(head);size_t len=0;auto it=headers.find("Content-Length");if(it!=headers.end())try{len=(size_t)std::stoull(it->second);}catch(...){len=0;}if(body.size()>len)body.resize(len);
  char ipbuf[INET6_ADDRSTRLEN]{};inet_ntop(AF_INET,&peer.sin_addr,ipbuf,sizeof(ipbuf));auto req=http_request(method,target,ipbuf,headers,body);Value* response=legacy?http_response(200,legacy->s):http_dispatch(req,0);if(!response)response=http_response(500,"Internal Server Error");auto packet=http_packet(response);
#ifdef _WIN32
  send(client,packet.data(),(int)packet.size(),0);closesocket(client);
#else
  send(client,packet.data(),packet.size(),0);close(client);
#endif
 }
#ifdef _WIN32
 closesocket(server);WSACleanup();
#else
 close(server);
#endif
 return rt_none();
}
extern "C" Value* rt_std_call(const char* module,const char* name,int argc,...){
 std::string m=module?module:"", n=name?name:"";va_list ap;va_start(ap,argc);
 Value* r=nullptr;
 if(m=="math")r=std_math(n,argc,ap);
 else if(m=="random")r=std_random(n,argc,ap);
 else if(m=="fs")r=std_fs(n,argc,ap);
 else if(m=="time")r=std_time(n,argc,ap);
 else if(m=="os")r=std_os(n,argc,ap);
 else if(m=="http")r=std_http(n,argc,ap);
 else if(m=="socket")r=std_socket(n,argc,ap);
 else if(m=="smtp"||m=="stmp")r=std_smtp(n,argc,ap);
 else if(m=="json")r=std_json(n,argc,ap);
 else fail("unknown standard library module: "+m);
 va_end(ap);return r;
}
extern "C" Value* rt_std_get(const char* module,const char* name){
 std::string m=module?module:"",n=name?name:"";
 if(m=="math"){
 if(n=="pi") return rt_float(3.14159265358979323846);
 if(n=="e") return rt_float(2.71828182845904523536);
 if(n=="tau") return rt_float(6.28318530717958647692);
}
 fail("unknown standard library constant: "+m+"."+n);return rt_none();
}

extern "C" int rt_try_begin(){static thread_local jmp_buf env;active=&env;return setjmp(env);}
extern "C" void rt_try_end(){active=nullptr;} extern "C" void rt_throw(Value*v){last=v;if(active)longjmp(*active,1);fail(text(v));} extern "C" Value* rt_last_error(){return last?last:rt_none();}
