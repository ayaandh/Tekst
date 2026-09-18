
#include "codegen.h"
#include <stdexcept>
#include <algorithm>

static std::string q(const std::string&s){
 std::string r;for(unsigned char c:s){if(c=='\\'||c=='"')r+='\\';if(c=='\n')r+="\\0A";else if(c=='\r')r+="\\0D";else if(c=='\t')r+="\\09";else if(c<32)r+='?';else r+=c;}return r;
}
std::string Codegen::escape(const std::string&s){return q(s);}
static bool isStdModule(const std::string& m){return m=="math"||m=="random"||m=="fs"||m=="time"||m=="os"||m=="http";}

void Codegen::ensureSlot(const std::string& n){
 if(slots.count(n)) return;
 std::string s="%slot_"+n;
 allocas<<"  "<<s<<" = alloca ptr\n";
 auto z=tmp(); body<<"  "<<z<<" = call ptr @rt_none()\n"; body<<"  store ptr "<<z<<", ptr "<<s<<"\n";
 slots[n]=s;
}
std::string Codegen::loadVar(const std::string& n){
 ensureSlot(n);auto r=tmp();body<<"  "<<r<<" = load ptr, ptr "<<slots[n]<<"\n";return r;
}
void Codegen::storeVar(const std::string& n,const std::string&v,const std::string&type){
 ensureSlot(n);body<<"  store ptr "<<v<<", ptr "<<slots[n]<<"\n";if(!type.empty())types[n]=type;
}

std::string Codegen::emitLambda(Lambda* l){
    std::string oldBody=body.str();
    auto oldSlots=std::move(slots);
    auto oldTypes=std::move(types);
    auto oldDefers=std::move(defers);
    auto oldFn=currentFn, oldOwner=currentOwner;
    int oldId=nextId, oldBlock=nextBlock;
    bool oldTerm=terminated;
    body.str("");body.clear();allocas.str("");allocas.clear();slots.clear();types.clear();defers.clear();nextId=0;nextBlock=0;terminated=false;controlKind=0;
    std::string name="__lambda_"+std::to_string(++lambdaId);
    currentFn=name;currentOwner="";
    lambdaFunctions<<"define ptr @"<<name<<"(";
    for(size_t i=0;i<l->params.size();++i){if(i)lambdaFunctions<<", ";lambdaFunctions<<"ptr %a"<<i;}
    lambdaFunctions<<") {\nentry:\n";
    for(size_t i=0;i<l->params.size();++i){ensureSlot(l->params[i]);storeVar(l->params[i],"%a"+std::to_string(i));}
    auto v=emitExpr(l->body.get());
    lambdaFunctions<<allocas.str()<<body.str()<<"  ret ptr "<<v<<"\n}\n";
    body.str(oldBody);body.clear();body.seekp(0,std::ios::end);
    slots=std::move(oldSlots);types=std::move(oldTypes);defers=std::move(oldDefers);
    currentFn=oldFn;currentOwner=oldOwner;nextId=oldId;nextBlock=oldBlock;terminated=oldTerm;
    auto r=tmp();body<<"  "<<r<<" = call ptr @rt_callable(ptr @"<<name<<")\n";return r;
}

std::string Codegen::emitExpr(Expr*e){
 if(auto n=dynamic_cast<Number*>(e)){auto r=tmp(); if(n->v.find('.')!=std::string::npos) body<<"  "<<r<<" = call ptr @rt_float(double "<<n->v<<")\n"; else body<<"  "<<r<<" = call ptr @rt_int(i64 "<<n->v<<")\n";return r;}
 if(auto s=dynamic_cast<String*>(e)){
   static int sid=0;
   auto makeStr=[&](const std::string& val){std::string g="@.s"+std::to_string(sid++);globals<<g<<" = private unnamed_addr constant ["<<val.size()+1<<" x i8] c\""<<q(val)<<"\\00\"\n";auto p=tmp();body<<"  "<<p<<" = getelementptr inbounds ["<<val.size()+1<<" x i8], ptr "<<g<<", i64 0, i64 0\n";auto r=tmp();body<<"  "<<r<<" = call ptr @rt_str(ptr "<<p<<")\n";return r;};
   size_t pos=0;std::string result;bool interp=false;
   while(pos<s->v.size()){
     size_t b=s->v.find('{',pos);if(b==std::string::npos)break;size_t epos=s->v.find('}',b+1);if(epos==std::string::npos)break;
     interp=true;std::string lit=s->v.substr(pos,b-pos);auto part=makeStr(lit);if(result.empty())result=part;else{auto rr=tmp();body<<"  "<<rr<<" = call ptr @rt_add(ptr "<<result<<", ptr "<<part<<")\n";result=rr;}
     std::string n=s->v.substr(b+1,epos-b-1);auto val=emitExpr(new Name(n));auto strv=tmp();body<<"  "<<strv<<" = call ptr @rt_to_str(ptr "<<val<<")\n";auto rr=tmp();body<<"  "<<rr<<" = call ptr @rt_add(ptr "<<result<<", ptr "<<strv<<")\n";result=rr;pos=epos+1;
   }
   if(!interp)return makeStr(s->v);
   auto tail=makeStr(s->v.substr(pos));auto rr=tmp();body<<"  "<<rr<<" = call ptr @rt_add(ptr "<<result<<", ptr "<<tail<<")\n";return rr;
 }
 if(auto b=dynamic_cast<Bool*>(e)){auto r=tmp();body<<"  "<<r<<" = call ptr @rt_bool(i1 "<<(b->v?"true":"false")<<")\n";return r;}
 if(dynamic_cast<NoneExpr*>(e)){auto r=tmp();body<<"  "<<r<<" = call ptr @rt_none()\n";return r;}
 if(auto n=dynamic_cast<Name*>(e)){
   if(n->v=="True"||n->v=="False"){auto r=tmp();body<<"  "<<r<<" = call ptr @rt_bool(i1 "<<(n->v=="True"?"true":"false")<<")\n";return r;}
   return loadVar(n->v);
 }
 if(auto l=dynamic_cast<List*>(e)){std::vector<std::string>v;for(auto&x:l->xs)v.push_back(emitExpr(x.get()));auto r=tmp();body<<"  "<<r<<" = call ptr (i32, ...) @rt_list(i32 "<<v.size();for(auto&x:v)body<<", ptr "<<x;body<<")\n";return r;} if(auto t=dynamic_cast<Tuple*>(e)){std::vector<std::string>v;for(auto&x:t->xs)v.push_back(emitExpr(x.get()));auto r=tmp();body<<"  "<<r<<" = call ptr (i32, ...) @rt_list(i32 "<<v.size();for(auto&x:v)body<<", ptr "<<x;body<<")\n";return r;}
 if(auto lc=dynamic_cast<ListComp*>(e)){auto it=emitExpr(lc->iterable.get());auto out=tmp();body<<"  "<<out<<" = call ptr @rt_list_empty()\n";ensureSlot(lc->var);auto oldvar=tmp();body<<"  "<<oldvar<<" = load ptr, ptr "<<slots[lc->var]<<"\n";auto idxslot="%compi"+std::to_string(++nextId);allocas<<"  "<<idxslot<<" = alloca ptr\n";auto z=tmp();body<<"  "<<z<<" = call ptr @rt_int(i64 0)\n  store ptr "<<z<<", ptr "<<idxslot<<"\n";auto idx=tmp();body<<"  "<<idx<<" = load ptr, ptr "<<idxslot<<"\n";auto head=label("comph"),done=label("compend"),inside=label("compbody");body<<"  br label %"<<head<<"\n"<<head<<":\n";idx=tmp();body<<"  "<<idx<<" = load ptr, ptr "<<idxslot<<"\n";auto n=tmp();body<<"  "<<n<<" = call ptr @rt_len(ptr "<<it<<")\n";auto cond=tmp();body<<"  "<<cond<<" = call ptr @rt_lt(ptr "<<idx<<", ptr "<<n<<")\n";auto tr=tmp();body<<"  "<<tr<<" = call i1 @rt_truth(ptr "<<cond<<")\n  br i1 "<<tr<<", label %"<<inside<<", label %"<<done<<"\n"<<inside<<":\n";auto item=tmp();body<<"  "<<item<<" = call ptr @rt_index(ptr "<<it<<", ptr "<<idx<<")\n";storeVar(lc->var,item);auto ev=emitExpr(lc->value.get());body<<"  call void @rt_list_push(ptr "<<out<<", ptr "<<ev<<" )\n";auto one=tmp();body<<"  "<<one<<" = call ptr @rt_int(i64 1)\n";auto ni=tmp();body<<"  "<<ni<<" = call ptr @rt_add(ptr "<<idx<<", ptr "<<one<<")\n  store ptr "<<ni<<", ptr "<<idxslot<<"\n";body<<"  br label %"<<head<<"\n"<<done<<":\n";body<<"  store ptr "<<oldvar<<", ptr "<<slots[lc->var]<<"\n";return out;}
 if(auto l=dynamic_cast<Lambda*>(e))return emitLambda(l);
 if(auto d=dynamic_cast<Dict*>(e)){std::vector<std::string>v;for(auto&x:d->xs){v.push_back(emitExpr(x.first.get()));v.push_back(emitExpr(x.second.get()));}auto r=tmp();body<<"  "<<r<<" = call ptr (i32, ...) @rt_dict(i32 "<<d->xs.size();for(auto&x:v)body<<", ptr "<<x;body<<")\n";return r;}
 if(auto u=dynamic_cast<Unary*>(e)){
   if(u->op=="&"){
     auto n=dynamic_cast<Name*>(u->x.get());
     if(!n) throw std::runtime_error("address-of (&) currently requires a variable");
     ensureSlot(n->v); auto r=tmp(); body<<"  "<<r<<" = call ptr @rt_ref(ptr "<<slots[n->v]<<")\n"; return r;
   }
   if(u->op=="*"){
     auto x=emitExpr(u->x.get()),r=tmp(); body<<"  "<<r<<" = call ptr @rt_deref(ptr "<<x<<")\n"; return r;
   }
   auto x=emitExpr(u->x.get()),r=tmp();body<<"  "<<r<<" = call ptr @rt_"<<(u->op=="-"?"neg":"not")<<"(ptr "<<x<<")\n";return r;
 }
 if(auto b=dynamic_cast<Binary*>(e)){
   if(b->op=="and"||b->op=="or"){
     auto a=emitExpr(b->a.get());
     auto tr=tmp();body<<"  "<<tr<<" = call i1 @rt_truth(ptr "<<a<<")\n";
     auto lhs=label("logic_lhs"),rhs=label("logic_rhs"),done=label("logic_done");
     if(b->op=="and") body<<"  br i1 "<<tr<<", label %"<<rhs<<", label %"<<lhs<<"\n"<<lhs<<":\n";
     else body<<"  br i1 "<<tr<<", label %"<<lhs<<", label %"<<rhs<<"\n"<<lhs<<":\n";
     auto av=tmp();body<<"  "<<av<<" = call ptr @rt_bool(i1 "<<(b->op=="or"?"true":"false")<<")\n  br label %"<<done<<"\n"<<rhs<<":\n";
     auto bv=emitExpr(b->b.get());auto btr=tmp();body<<"  "<<btr<<" = call i1 @rt_truth(ptr "<<bv<<")\n";auto brv=tmp();body<<"  "<<brv<<" = call ptr @rt_bool(i1 "<<btr<<")\n";
     auto res=tmp();body<<"  br label %"<<done<<"\n"<<done<<":\n";
     body<<"  "<<res<<" = phi ptr ["<<av<<", %"<<lhs<<"], ["<<brv<<", %"<<rhs<<"]\n";return res;
   }
   auto a=emitExpr(b->a.get()),c=emitExpr(b->b.get()),r=tmp();std::string fn;
   if(b->op=="+")fn="add";else if(b->op=="-")fn="sub";else if(b->op=="*")fn="mul";else if(b->op=="/")fn="div";else if(b->op=="%")fn="mod";
   else if(b->op=="==" )fn="eq";else if(b->op=="!=")fn="ne";else if(b->op=="<")fn="lt";else if(b->op=="<=")fn="le";else if(b->op==">")fn="gt";else if(b->op==">=")fn="ge";
   else throw std::runtime_error("unknown operator "+b->op);
   body<<"  "<<r<<" = call ptr @rt_"<<fn<<"(ptr "<<a<<", ptr "<<c<<")\n";return r;
 }
 if(auto i=dynamic_cast<Index*>(e)){auto a=emitExpr(i->a.get()),x=emitExpr(i->i.get()),r=tmp();body<<"  "<<r<<" = call ptr @rt_index(ptr "<<a<<", ptr "<<x<<")\n";return r;}
 if(auto a=dynamic_cast<Attr*>(e)){
    if(auto mod=dynamic_cast<Name*>(a->a.get()); mod && importedModules.count(mod->v) && isStdModule(importedModules[mod->v])){
      std::string module=importedModules[mod->v],key=a->n;
      if(module=="math"&&(key=="pi"||key=="e"||key=="tau")){
        static int sid=52000;std::string gm="@.stdm"+std::to_string(sid++),gn="@.stdn"+std::to_string(sid++);
        globals<<gm<<" = private unnamed_addr constant ["<<module.size()+1<<" x i8] c\""<<q(module)<<"\\00\"\n";
        globals<<gn<<" = private unnamed_addr constant ["<<key.size()+1<<" x i8] c\""<<q(key)<<"\\00\"\n";
        auto pm=tmp(),pn=tmp();body<<"  "<<pm<<" = getelementptr inbounds ["<<module.size()+1<<" x i8], ptr "<<gm<<", i64 0, i64 0\n";
        body<<"  "<<pn<<" = getelementptr inbounds ["<<key.size()+1<<" x i8], ptr "<<gn<<", i64 0, i64 0\n";
        auto r=tmp();body<<"  "<<r<<" = call ptr @rt_std_get(ptr "<<pm<<", ptr "<<pn<<")\n";return r;
      }
    }
    auto x=emitExpr(a->a.get()),r=tmp();static int sid=10000;std::string g="@.a"+std::to_string(sid++);globals<<g<<" = private unnamed_addr constant ["<<a->n.size()+1<<" x i8] c\""<<q(a->n)<<"\\00\"\n";auto p=tmp();body<<"  "<<p<<" = getelementptr inbounds ["<<a->n.size()+1<<" x i8], ptr "<<g<<", i64 0, i64 0\n";body<<"  "<<r<<" = call ptr @"<<(a->optional?"rt_optional_attr":"rt_get_attr")<<"(ptr "<<x<<", ptr "<<p<<")\n";return r;}
 if(auto c=dynamic_cast<Call*>(e)){
   std::vector<std::string>args;for(auto&x:c->args)args.push_back(emitExpr(x.get()));
   if(auto n=dynamic_cast<Name*>(c->callee.get())){
     if(n->v=="print"){if(args.size()!=1)throw std::runtime_error("print takes one argument");body<<"  call void @rt_print(ptr "<<args[0]<<")\n";auto r=tmp();body<<"  "<<r<<" = call ptr @rt_none()\n";return r;}
     if(n->v=="input"){if(args.size()>1)throw std::runtime_error("input takes zero or one argument");auto r=tmp();if(args.empty()){auto z=tmp();body<<"  "<<z<<" = call ptr @rt_str_const_empty()\n";body<<"  "<<r<<" = call ptr @rt_input(ptr "<<z<<")\n";}else body<<"  "<<r<<" = call ptr @rt_input(ptr "<<args[0]<<")\n";return r;}
     std::string bn=n->v;
     if(importedNames.count(bn)) bn=importedNames[bn];
     std::unordered_map<std::string,std::string> built{{"int","to_int"},{"str","to_str"},{"bool","to_bool"},{"float","to_float"},{"len","len"},{"alloc","alloc"},{"free","free"},{"ptr_add","ptr_add"},{"load_int","ptr_load_int"},{"load_byte","ptr_load_byte"},{"store_int","ptr_store_int"},{"store_byte","ptr_store_byte"}};
     if(built.count(bn)){
       if(bn=="free"){if(args.size()!=1)throw std::runtime_error("free takes one argument");body<<"  call void @rt_free(ptr "<<args[0]<<")\n";auto r=tmp();body<<"  "<<r<<" = call ptr @rt_none()\n";return r;}
       if(bn=="store_int"||bn=="store_byte"){if(args.size()!=2)throw std::runtime_error(bn+" takes two arguments");body<<"  call void @rt_"<<built[bn]<<"(ptr "<<args[0]<<", ptr "<<args[1]<<")\n";auto r=tmp();body<<"  "<<r<<" = call ptr @rt_none()\n";return r;}
       auto r=tmp(); if(bn=="alloc"||bn=="ptr_add"||bn=="load_int"||bn=="load_byte") { body<<"  "<<r<<" = call ptr @rt_"<<built[bn]<<"(ptr "<<args.at(0); if(bn=="ptr_add"&&args.size()>1) body<<", ptr "<<args[1]; body<<")\n"; return r; }
       body<<"  "<<r<<" = call ptr @rt_"<<built[bn]<<"(ptr "<<args.at(0)<<")\n";return r;
     }
     if(bn=="range"){auto r=tmp();body<<"  "<<r<<" = call ptr (i32, ...) @rt_range(i32 "<<args.size();for(auto&x:args)body<<", ptr "<<x;body<<")\n";return r;}
     if(bn.find('.')!=std::string::npos){
       auto dot=bn.find('.'); std::string mod=bn.substr(0,dot),name=bn.substr(dot+1);
       if(isStdModule(mod)){
         static int sid=50000;std::string gm="@.stdm"+std::to_string(sid++),gn="@.stdn"+std::to_string(sid++);
         globals<<gm<<" = private unnamed_addr constant ["<<mod.size()+1<<" x i8] c\""<<q(mod)<<"\\00\"\n";
         globals<<gn<<" = private unnamed_addr constant ["<<name.size()+1<<" x i8] c\""<<q(name)<<"\\00\"\n";
         auto pm=tmp(),pn=tmp();body<<"  "<<pm<<" = getelementptr inbounds ["<<mod.size()+1<<" x i8], ptr "<<gm<<", i64 0, i64 0\n";
         body<<"  "<<pn<<" = getelementptr inbounds ["<<name.size()+1<<" x i8], ptr "<<gn<<", i64 0, i64 0\n";
         auto r=tmp();body<<"  "<<r<<" = call ptr (ptr,ptr,i32,...) @rt_std_call(ptr "<<pm<<", ptr "<<pn<<", i32 "<<args.size();
         for(auto&x:args)body<<", ptr "<<x;body<<")\n";return r;
       }
     }
     auto it=functions.find(bn);if(it!=functions.end()){
       Function*f=it->second.f; if(args.size()>f->params.size())throw std::runtime_error("too many arguments to "+bn);
       while(args.size()<f->params.size()){size_t k=args.size();if(!f->defaults[k])throw std::runtime_error("missing argument to "+bn);args.push_back(emitExpr(f->defaults[k].get()));}
       auto r=tmp();body<<"  "<<r<<" = call ptr @"<<it->second.symbol<<"(";for(size_t k=0;k<args.size();++k){if(k)body<<", ";body<<"ptr "<<args[k];}body<<")\n";
       return r;
     }
     auto ci=classes.find(bn);if(ci!=classes.end()){auto r=tmp();static int sid=20000;std::string g="@.c"+std::to_string(sid++);globals<<g<<" = private unnamed_addr constant ["<<bn.size()+1<<" x i8] c\""<<q(bn)<<"\\00\"\n";auto p=tmp();body<<"  "<<p<<" = getelementptr inbounds ["<<bn.size()+1<<" x i8], ptr "<<g<<", i64 0, i64 0\n";body<<"  "<<r<<" = call ptr @rt_new_object(ptr "<<p<<")\n";auto init=ci->second;for(auto&st:init->body)if(auto fn=dynamic_cast<Function*>(st.get()))if(fn->name=="__init__"){std::vector<std::string>aa{r};for(auto&x:args)aa.push_back(x);body<<"  call ptr @"<<functions[bn+"::__init__"].symbol<<"(";for(size_t k=0;k<aa.size();++k){if(k)body<<", ";body<<"ptr "<<aa[k];}body<<")\n";break;}return r;}
     auto callable=loadVar(bn);auto cr=tmp();body<<"  "<<cr<<" = call ptr (ptr,i32,...) @rt_call_callable(ptr "<<callable<<", i32 "<<args.size();for(auto&x:args)body<<", ptr "<<x;body<<")\n";return cr;
   }
   if(!dynamic_cast<Name*>(c->callee.get())&&!dynamic_cast<Attr*>(c->callee.get())){
     auto fnv=emitExpr(c->callee.get());auto r=tmp();body<<"  "<<r<<" = call ptr (ptr,i32,...) @rt_call_callable(ptr "<<fnv<<", i32 "<<args.size();for(auto&x:args)body<<", ptr "<<x;body<<")\n";return r;
   }
   if(auto at=dynamic_cast<Attr*>(c->callee.get())){
     if(auto mod=dynamic_cast<Name*>(at->a.get()); mod && importedModules.count(mod->v)){
       std::string key=at->n;
       std::string module=importedModules[mod->v];
       if(isStdModule(module)){
         static int sid=51000;std::string gm="@.stdm"+std::to_string(sid++),gn="@.stdn"+std::to_string(sid++);
         globals<<gm<<" = private unnamed_addr constant ["<<module.size()+1<<" x i8] c\""<<q(module)<<"\\00\"\n";
         globals<<gn<<" = private unnamed_addr constant ["<<key.size()+1<<" x i8] c\""<<q(key)<<"\\00\"\n";
         auto pm=tmp(),pn=tmp();body<<"  "<<pm<<" = getelementptr inbounds ["<<module.size()+1<<" x i8], ptr "<<gm<<", i64 0, i64 0\n";
         body<<"  "<<pn<<" = getelementptr inbounds ["<<key.size()+1<<" x i8], ptr "<<gn<<", i64 0, i64 0\n";
         auto r=tmp();body<<"  "<<r<<" = call ptr (ptr,ptr,i32,...) @rt_std_call(ptr "<<pm<<", ptr "<<pn<<", i32 "<<args.size();
         for(auto&x:args)body<<", ptr "<<x;body<<")\n";return r;
       }
       auto fi=functions.find(key);
       if(fi!=functions.end()){
         Function*f=fi->second.f;
         if(args.size()>f->params.size())throw std::runtime_error("too many arguments to "+at->n);
         while(args.size()<f->params.size()){size_t k=args.size();if(!f->defaults[k])throw std::runtime_error("missing argument to "+at->n);args.push_back(emitExpr(f->defaults[k].get()));}
         auto r=tmp(); body<<"  "<<r<<" = call ptr @"<<fi->second.symbol<<"(";for(size_t k=0;k<args.size();++k){if(k)body<<", ";body<<"ptr "<<args[k];}body<<")\n"; return r;
       }
       auto ci=classes.find(key);
       if(ci!=classes.end()){auto r=tmp();static int sid=25000;std::string g="@.mc"+std::to_string(sid++);globals<<g<<" = private unnamed_addr constant ["<<key.size()+1<<" x i8] c\""<<q(key)<<"\\00\"\n";auto p=tmp();body<<"  "<<p<<" = getelementptr inbounds ["<<key.size()+1<<" x i8], ptr "<<g<<", i64 0, i64 0\n";body<<"  "<<r<<" = call ptr @rt_new_object(ptr "<<p<<")\n";return r;}
     }
     auto obj=emitExpr(at->a.get());auto r=tmp();std::string owner;
     if(auto nn=dynamic_cast<Name*>(at->a.get())) owner=types[nn->v]; else if(currentOwner.size())owner=currentOwner;
     std::string sym=owner+"::"+at->n;
     auto fi=functions.find(sym);
     if(fi!=functions.end()){Function*f=fi->second.f;bool selfParam=!f->params.empty()&&f->params[0]=="self";size_t expected=f->params.size()-(selfParam?1:0);if(args.size()>expected)throw std::runtime_error("too many arguments to method "+at->n);while(args.size()<expected){size_t k=args.size()+(selfParam?1:0);if(k>=f->params.size()||!f->defaults[k])throw std::runtime_error("missing argument to method "+at->n);args.push_back(emitExpr(f->defaults[k].get()));}body<<"  "<<r<<" = call ptr @"<<fi->second.symbol<<"(ptr "<<obj;for(auto&x:args)body<<", ptr "<<x;body<<")\n";return r;}
     static int sid=30000;std::string g="@.m"+std::to_string(sid++);globals<<g<<" = private unnamed_addr constant ["<<at->n.size()+1<<" x i8] c\""<<q(at->n)<<"\\00\"\n";auto p=tmp();body<<"  "<<p<<" = getelementptr inbounds ["<<at->n.size()+1<<" x i8], ptr "<<g<<", i64 0, i64 0\n";body<<"  "<<r<<" = call ptr @rt_call_method(ptr "<<obj<<", ptr "<<p;for(auto&x:args)body<<", ptr "<<x;body<<", ptr null)\n";return r;
   }
 }
 throw std::runtime_error("unsupported expression");
}

void Codegen::emitStmt(Stmt*s){
 if(dynamic_cast<Import*>(s) || dynamic_cast<FromImport*>(s)) return;
 if(auto p=dynamic_cast<Print*>(s)){std::vector<std::string> args;for(auto&arg:p->args)args.push_back(emitExpr(arg.get()));body<<"  call void (i32, ...) @rt_print_many(i32 "<<args.size();for(auto&x:args)body<<", ptr "<<x;body<<")\n";return;}
 if(auto e=dynamic_cast<ExprStmt*>(s)){emitExpr(e->e.get());return;}
 if(auto a=dynamic_cast<Assign*>(s)){
   std::string v=emitExpr(a->value.get());if(a->op!="="){auto old=emitExpr(a->target.get());std::string fn=a->op=="+="? "add":a->op=="-="? "sub":a->op=="*="? "mul":"div";auto r=tmp();body<<"  "<<r<<" = call ptr @rt_"<<fn<<"(ptr "<<old<<", ptr "<<v<<")\n";v=r;}
   if(auto u=dynamic_cast<Unary*>(a->target.get()); u && u->op=="*"){
     auto ptr=emitExpr(u->x.get()); body<<"  call void @rt_store(ptr "<<ptr<<", ptr "<<v<<")\n";
   }
   else if(auto l=dynamic_cast<List*>(a->target.get())){for(size_t i=0;i<l->xs.size();++i){if(auto n=dynamic_cast<Name*>(l->xs[i].get())){auto iv=tmp();auto ix=tmp();body<<"  "<<ix<<" = call ptr @rt_int(i64 "<<i<<")\n  "<<iv<<" = call ptr @rt_index(ptr "<<v<<", ptr "<<ix<<")\n";storeVar(n->v,iv);}}}
   else if(auto t=dynamic_cast<Tuple*>(a->target.get())){for(size_t i=0;i<t->xs.size();++i){if(auto n=dynamic_cast<Name*>(t->xs[i].get())){auto iv=tmp();auto ix=tmp();body<<"  "<<ix<<" = call ptr @rt_int(i64 "<<i<<")\n  "<<iv<<" = call ptr @rt_index(ptr "<<v<<", ptr "<<ix<<")\n";storeVar(n->v,iv);}}}
   else if(auto n=dynamic_cast<Name*>(a->target.get())){std::string ty; if(auto c=dynamic_cast<Call*>(a->value.get()))if(auto nn=dynamic_cast<Name*>(c->callee.get()))if(classes.count(nn->v))ty=nn->v;storeVar(n->v,v,ty);}
   else if(auto at=dynamic_cast<Attr*>(a->target.get())){auto obj=emitExpr(at->a.get());static int sid=40000;std::string g="@.sa"+std::to_string(sid++);globals<<g<<" = private unnamed_addr constant ["<<at->n.size()+1<<" x i8] c\""<<q(at->n)<<"\\00\"\n";auto pp=tmp();body<<"  "<<pp<<" = getelementptr inbounds ["<<at->n.size()+1<<" x i8], ptr "<<g<<", i64 0, i64 0\n";body<<"  call void @rt_set_attr(ptr "<<obj<<", ptr "<<pp<<", ptr "<<v<<")\n";}
   else if(auto ix=dynamic_cast<Index*>(a->target.get())){auto obj=emitExpr(ix->a.get()),ind=emitExpr(ix->i.get());body<<"  call void @rt_set_index(ptr "<<obj<<", ptr "<<ind<<", ptr "<<v<<")\n";}
   return;
 }
 if(auto d=dynamic_cast<Defer*>(s)){defers.push_back(d->e.get());return;}
 if(auto r=dynamic_cast<Return*>(s)){std::vector<Expr*> ds=defers;for(auto it=ds.rbegin();it!=ds.rend();++it)emitExpr(*it);if(r->e){auto x=emitExpr(r->e.get());body<<"  ret ptr "<<x<<"\n";}else{auto x=tmp();body<<"  "<<x<<" = call ptr @rt_none()\n  ret ptr "<<x<<"\n";}terminated=true;return;}
 if(auto b=dynamic_cast<Break*>(s)){if(breakTargets.empty())throw std::runtime_error("break outside loop");body<<"  br label %"<<breakTargets.back()<<"\n";terminated=true;controlKind=1;return;}
 if(auto c=dynamic_cast<Continue*>(s)){if(continueTargets.empty())throw std::runtime_error("continue outside loop");body<<"  br label %"<<continueTargets.back()<<"\n";terminated=true;controlKind=2;return;}
 if(auto m=dynamic_cast<Match*>(s)){
   auto val=emitExpr(m->value.get());
   auto end=label("matchend");
   for(size_t i=0;i<m->cases.size();++i){
     std::string eq;
     if(auto pn=dynamic_cast<Name*>(m->cases[i].first.get());pn&&pn->v=="_"){
       eq=tmp();
       body<<"  "<<eq<<" = call ptr @rt_bool(i1 true)\n";
     }else{
       auto pat=emitExpr(m->cases[i].first.get());
       eq=tmp();
       body<<"  "<<eq<<" = call ptr @rt_eq(ptr "<<val<<", ptr "<<pat<<")\n";
     }
     auto tr=tmp();
     body<<"  "<<tr<<" = call i1 @rt_truth(ptr "<<eq<<")\n";
     auto yes=label("caseyes"),no=label("caseno");
     body<<"  br i1 "<<tr<<", label %"<<yes<<", label %"<<no<<"\n"<<yes<<":\n";
     terminated=false;
     emitBlock(m->cases[i].second);
     if(!terminated)body<<"  br label %"<<end<<"\n";
     terminated=false;
     body<<no<<":\n";
   }
   if(!m->els.empty()){
     terminated=false;
     emitBlock(m->els);
   }
   if(!terminated)body<<"  br label %"<<end<<"\n";
   body<<end<<":\n";
   terminated=false;
   return;
 }
 if(auto z=dynamic_cast<If*>(s)){
   std::string end=label("ifend");std::vector<std::string>ends;
   for(size_t i=0;i<z->branches.size();++i){auto c=emitExpr(z->branches[i].first.get());auto yes=label("ifyes"),no=label("ifno");auto tr=tmp();body<<"  "<<tr<<" = call i1 @rt_truth(ptr "<<c<<")\n  br i1 "<<tr<<", label %"<<yes<<", label %"<<no<<"\n"<<yes<<":\n";emitBlock(z->branches[i].second);if(!terminated)body<<"  br label %"<<end<<"\n";terminated=false;body<<no<<":\n";if(i+1==z->branches.size()&&!z->els.empty())emitBlock(z->els);if(i+1==z->branches.size()&&!terminated)body<<"  br label %"<<end<<"\n";}body<<end<<":\n";return;
 }
 if(auto w=dynamic_cast<While*>(s)){auto head=label("while"),done=label("wend"),inside=label("wbody");body<<"  br label %"<<head<<"\n"<<head<<":\n";auto c=emitExpr(w->cond.get());auto tr=tmp();body<<"  "<<tr<<" = call i1 @rt_truth(ptr "<<c<<")\n  br i1 "<<tr<<", label %"<<inside<<", label %"<<done<<"\n"<<inside<<":\n";breakTargets.push_back(done);continueTargets.push_back(head);terminated=false;emitBlock(w->body);breakTargets.pop_back();continueTargets.pop_back();if(!terminated)body<<"  br label %"<<head<<"\n";terminated=false;body<<done<<":\n";return;}
 if(auto f=dynamic_cast<For*>(s)){auto it=emitExpr(f->iterable.get());std::string idxslot="%foridx"+std::to_string(++nextId);allocas<<"  "<<idxslot<<" = alloca ptr\n";auto zero=tmp();body<<"  "<<zero<<" = call ptr @rt_int(i64 0)\n  store ptr "<<zero<<", ptr "<<idxslot<<"\n";std::string head=label("for"),done=label("forend"),inside=label("forbody"),inc=label("forinc");body<<"  br label %"<<head<<"\n"<<head<<":\n";auto idx=tmp();body<<"  "<<idx<<" = load ptr, ptr "<<idxslot<<"\n";auto n=tmp();body<<"  "<<n<<" = call ptr @rt_len(ptr "<<it<<")\n";auto cond=tmp();body<<"  "<<cond<<" = call ptr @rt_lt(ptr "<<idx<<", ptr "<<n<<")\n";auto tr=tmp();body<<"  "<<tr<<" = call i1 @rt_truth(ptr "<<cond<<")\n  br i1 "<<tr<<", label %"<<inside<<", label %"<<done<<"\n"<<inside<<":\n";auto v=tmp();body<<"  "<<v<<" = call ptr @rt_index(ptr "<<it<<", ptr "<<idx<<")\n";storeVar(f->var,v);breakTargets.push_back(done);continueTargets.push_back(inc);terminated=false;emitBlock(f->body);breakTargets.pop_back();continueTargets.pop_back();if(terminated&&controlKind==1){terminated=false;body<<done<<":\n";return;}if(!terminated)body<<"  br label %"<<inc<<"\n";terminated=false;body<<inc<<":\n";auto one=tmp();body<<"  "<<one<<" = call ptr @rt_int(i64 1)\n";auto ni=tmp();body<<"  "<<ni<<" = call ptr @rt_add(ptr "<<idx<<", ptr "<<one<<")\n  store ptr "<<ni<<", ptr "<<idxslot<<"\n";body<<"  br label %"<<head<<"\n"<<done<<":\n";return;}
 if(dynamic_cast<Try*>(s)){throw std::runtime_error("try/catch lowering is not yet enabled in the LLVM backend");}
 if(auto fn=dynamic_cast<Function*>(s)){return;} if(auto c=dynamic_cast<Class*>(s)){return;}
 throw std::runtime_error("unsupported statement");
}
void Codegen::emitBlock(const std::vector<S>&v){for(auto&s:v){if(terminated)break;emitStmt(s.get());}}

void Codegen::emitFunction(Function*f){
 slots.clear();types.clear();defers.clear();nextId=0;nextBlock=0;terminated=false;controlKind=0;body.str("");body.clear();allocas.str("");allocas.clear();currentFn=f->name;currentOwner=f->owner;
 std::string sym=f->method?f->owner+"__"+f->name:f->name;
 bool explicitSelf=f->method && !f->params.empty() && f->params[0]=="self";
 ir<<"define ptr @"<<sym<<"(";size_t n=f->params.size()+(f->method&&!explicitSelf?1:0);for(size_t i=0;i<n;++i){if(i)ir<<", ";ir<<"ptr %a"<<i;}ir<<") {\nentry:\n";
 size_t off=0;
 if(f->method){ensureSlot("self");storeVar("self","%a0",f->owner);off=explicitSelf?1:1;}
 for(size_t i=0;i<f->params.size();++i){if(explicitSelf && i==0)continue;ensureSlot(f->params[i]);storeVar(f->params[i],"%a"+std::to_string(i+((f->method&&!explicitSelf)?1:0)));}
 emitBlock(f->body);
 for(auto it=defers.rbegin();it!=defers.rend();++it)emitExpr(*it);
 auto r=tmp();body<<"  "<<r<<" = call ptr @rt_none()\n  ret ptr "<<r<<"\n";
 ir<<allocas.str()<<body.str()<<"}\n";
}
std::string Codegen::generate(const Program&p){
 ir<<"; Tekst LLVM IR\nsource_filename = \"Tekst\"\n\n";
 ir<<"declare ptr @rt_none()\ndeclare ptr @rt_int(i64)\ndeclare ptr @rt_float(double)\ndeclare ptr @rt_str(ptr)\ndeclare ptr @rt_str_const_empty()\ndeclare ptr @rt_bool(i1)\ndeclare ptr @rt_callable(ptr)\ndeclare ptr @rt_call_callable(ptr,i32,...)\ndeclare ptr @rt_list_empty()\ndeclare void @rt_list_push(ptr,ptr)\ndeclare ptr @rt_optional_attr(ptr,ptr)\ndeclare ptr @rt_add(ptr,ptr)\ndeclare ptr @rt_sub(ptr,ptr)\ndeclare ptr @rt_mul(ptr,ptr)\ndeclare ptr @rt_div(ptr,ptr)\ndeclare ptr @rt_mod(ptr,ptr)\ndeclare ptr @rt_eq(ptr,ptr)\ndeclare ptr @rt_ne(ptr,ptr)\ndeclare ptr @rt_lt(ptr,ptr)\ndeclare ptr @rt_le(ptr,ptr)\ndeclare ptr @rt_gt(ptr,ptr)\ndeclare ptr @rt_ge(ptr,ptr)\ndeclare ptr @rt_neg(ptr)\ndeclare ptr @rt_not(ptr)\ndeclare ptr @rt_ref(ptr)\ndeclare ptr @rt_deref(ptr)\ndeclare void @rt_store(ptr,ptr)\ndeclare ptr @rt_alloc(ptr)\ndeclare void @rt_free(ptr)\ndeclare ptr @rt_ptr_add(ptr,ptr)\ndeclare ptr @rt_ptr_load_int(ptr)\ndeclare void @rt_ptr_store_int(ptr,ptr)\ndeclare ptr @rt_ptr_load_byte(ptr)\ndeclare void @rt_ptr_store_byte(ptr,ptr)\ndeclare ptr @rt_and(ptr,ptr)\ndeclare ptr @rt_or(ptr,ptr)\ndeclare i1 @rt_truth(ptr)\ndeclare void @rt_print(ptr)\ndeclare void @rt_print_many(i32,...)\ndeclare ptr @rt_input(ptr)\ndeclare ptr @rt_to_int(ptr)\ndeclare ptr @rt_to_str(ptr)\ndeclare ptr @rt_to_bool(ptr)\ndeclare ptr @rt_to_float(ptr)\ndeclare ptr @rt_len(ptr)\ndeclare ptr @rt_index(ptr,ptr)\ndeclare void @rt_set_index(ptr,ptr,ptr)\ndeclare ptr @rt_list(i32,...)\ndeclare ptr @rt_dict(i32,...)\ndeclare ptr @rt_range(i32,...)\ndeclare ptr @rt_new_object(ptr)\ndeclare ptr @rt_get_attr(ptr,ptr)\ndeclare void @rt_set_attr(ptr,ptr,ptr)\ndeclare ptr @rt_call_method(ptr,ptr,...)\ndeclare ptr @rt_format(ptr,i32,...)\ndeclare ptr @rt_std_call(ptr,ptr,i32,...)\ndeclare ptr @rt_std_get(ptr,ptr)\ndeclare i32 @rt_try_begin()\ndeclare void @rt_try_end()\ndeclare void @rt_throw(ptr)\ndeclare ptr @rt_last_error()\n\n";
 for(auto&s:p.body){
   if(auto im=dynamic_cast<Import*>(s.get())) importedModules[im->alias.empty()?im->module:im->alias]=im->module;
   if(auto fi=dynamic_cast<FromImport*>(s.get())) importedNames[fi->alias.empty()?fi->name:fi->alias]=isStdModule(fi->module)?fi->module+"."+fi->name:fi->name;
 }
 for(auto&s:p.body){
   if(auto f=dynamic_cast<Function*>(s.get()))functions[f->name]={f,f->name,""};
   if(auto c=dynamic_cast<Class*>(s.get())){classes[c->name]=c;for(auto&x:c->body)if(auto f=dynamic_cast<Function*>(x.get()))functions[c->name+"::"+f->name]={f,c->name+"__"+f->name,c->name};}
 }
 for(auto&s:p.body)if(auto f=dynamic_cast<Function*>(s.get()))emitFunction(f);
 for(auto&s:p.body)if(auto c=dynamic_cast<Class*>(s.get()))for(auto&x:c->body)if(auto f=dynamic_cast<Function*>(x.get()))emitFunction(f);
 slots.clear();types.clear();defers.clear();terminated=false;nextId=0;nextBlock=0;body.str("");body.clear();allocas.str("");allocas.clear();currentFn="main";currentOwner="";
 emitBlock(p.body);
 for(auto it=defers.rbegin();it!=defers.rend();++it)emitExpr(*it);
 ir<<globals.str();
 ir<<lambdaFunctions.str();
 ir<<"define i32 @main() {\nentry:\n"<<allocas.str()<<body.str()<<"  ret i32 0\n}\n";return ir.str();
}
