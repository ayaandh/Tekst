#include "ast.h"
#include <cstddef>
namespace {
std::size_t countExpr(const Expr* e){
 if(!e)return 0;
 if(auto x=dynamic_cast<const List*>(e)){std::size_t n=1;for(auto&v:x->xs)n+=countExpr(v.get());return n;}
 if(auto x=dynamic_cast<const Tuple*>(e)){std::size_t n=1;for(auto&v:x->xs)n+=countExpr(v.get());return n;}
 if(auto x=dynamic_cast<const Dict*>(e)){std::size_t n=1;for(auto&v:x->xs)n+=countExpr(v.first.get())+countExpr(v.second.get());return n;}
 if(auto x=dynamic_cast<const ListComp*>(e))return 1+countExpr(x->value.get())+countExpr(x->iterable.get());
 if(auto x=dynamic_cast<const Lambda*>(e))return 1+countExpr(x->body.get());
 if(auto x=dynamic_cast<const Unary*>(e))return 1+countExpr(x->x.get());
 if(auto x=dynamic_cast<const Binary*>(e))return 1+countExpr(x->a.get())+countExpr(x->b.get());
 if(auto x=dynamic_cast<const Call*>(e)){std::size_t n=1+countExpr(x->callee.get());for(auto&v:x->args)n+=countExpr(v.get());return n;}
 if(auto x=dynamic_cast<const Index*>(e))return 1+countExpr(x->a.get())+countExpr(x->i.get());
 if(auto x=dynamic_cast<const Attr*>(e))return 1+countExpr(x->a.get());
 return 1;
}
std::size_t countStmt(const Stmt* s){
 if(!s)return 0;
 if(auto x=dynamic_cast<const ExprStmt*>(s))return 1+countExpr(x->e.get());
 if(auto x=dynamic_cast<const Assign*>(s))return 1+countExpr(x->target.get())+countExpr(x->value.get());
 if(auto x=dynamic_cast<const Print*>(s)){std::size_t n=1;for(auto&v:x->args)n+=countExpr(v.get());return n;}
 if(auto x=dynamic_cast<const Return*>(s))return 1+countExpr(x->e.get());
 if(auto x=dynamic_cast<const Defer*>(s))return 1+countExpr(x->e.get());
 if(auto x=dynamic_cast<const Throw*>(s))return 1+countExpr(x->e.get());
 if(auto x=dynamic_cast<const If*>(s)){std::size_t n=1;for(auto&b:x->branches){n+=countExpr(b.first.get());for(auto&v:b.second)n+=countStmt(v.get());}for(auto&v:x->els)n+=countStmt(v.get());return n;}
 if(auto x=dynamic_cast<const While*>(s)){std::size_t n=1+countExpr(x->cond.get());for(auto&v:x->body)n+=countStmt(v.get());return n;}
 if(auto x=dynamic_cast<const For*>(s)){std::size_t n=1+countExpr(x->iterable.get());for(auto&v:x->body)n+=countStmt(v.get());return n;}
 if(auto x=dynamic_cast<const Try*>(s)){std::size_t n=1;for(auto&v:x->body)n+=countStmt(v.get());for(auto&v:x->handler)n+=countStmt(v.get());return n;}
 if(auto x=dynamic_cast<const Match*>(s)){std::size_t n=1+countExpr(x->value.get());for(auto&b:x->cases){n+=countExpr(b.first.get());for(auto&v:b.second)n+=countStmt(v.get());}for(auto&v:x->els)n+=countStmt(v.get());return n;}
 if(auto x=dynamic_cast<const Function*>(s)){std::size_t n=1;for(auto&v:x->defaults)n+=countExpr(v.get());for(auto&v:x->body)n+=countStmt(v.get());return n;}
 if(auto x=dynamic_cast<const Class*>(s)){std::size_t n=1;for(auto&v:x->body)n+=countStmt(v.get());return n;}
 return 1;
}
}
std::size_t astNodeCount(const Program& program){std::size_t n=1;for(auto&v:program.body)n+=countStmt(v.get());return n;}
