#pragma once
#include <memory>
#include <string>
#include <vector>
#include <utility>

struct Expr { virtual ~Expr()=default; };
using E=std::unique_ptr<Expr>;

struct Number:Expr{std::string v;explicit Number(std::string x):v(std::move(x)){}};
struct String:Expr{std::string v;explicit String(std::string x):v(std::move(x)){}};
struct Name:Expr{std::string v;explicit Name(std::string x):v(std::move(x)){}};
struct Bool:Expr{bool v;explicit Bool(bool x):v(x){}};
struct NoneExpr:Expr{};
struct List:Expr{std::vector<E> xs;};
struct Tuple:Expr{std::vector<E> xs;};
struct Dict:Expr{std::vector<std::pair<E,E>> xs;};
struct ListComp:Expr{E value;std::string var;E iterable;ListComp(E v,std::string n,E i):value(std::move(v)),var(std::move(n)),iterable(std::move(i)){}};
struct Lambda:Expr{std::vector<std::string> params;E body;Lambda(std::vector<std::string> p,E b):params(std::move(p)),body(std::move(b)){}};
struct Unary:Expr{std::string op;E x;Unary(std::string o,E a):op(std::move(o)),x(std::move(a)){}};
struct Binary:Expr{std::string op;E a,b;Binary(std::string o,E x,E y):op(std::move(o)),a(std::move(x)),b(std::move(y)){}};
struct Call:Expr{E callee;std::vector<E> args;Call(E c,std::vector<E> a):callee(std::move(c)),args(std::move(a)){}};
struct Index:Expr{E a,i;Index(E x,E y):a(std::move(x)),i(std::move(y)){}};
struct Attr:Expr{E a;std::string n;bool optional=false;Attr(E x,std::string s,bool o=false):a(std::move(x)),n(std::move(s)),optional(o){}};

struct Stmt{virtual~Stmt()=default;}; using S=std::unique_ptr<Stmt>;
struct ExprStmt:Stmt{E e;explicit ExprStmt(E x):e(std::move(x)){}};
struct Assign:Stmt{E target,value;std::string op;Assign(E t,E v,std::string o="="):target(std::move(t)),value(std::move(v)),op(std::move(o)){}};
struct Print:Stmt{std::vector<E> args;explicit Print(std::vector<E> x):args(std::move(x)){}};
struct Return:Stmt{E e;explicit Return(E x):e(std::move(x)){}};
struct Break:Stmt{};
struct Continue:Stmt{};
struct Defer:Stmt{E e;explicit Defer(E x):e(std::move(x)){}};
struct Throw:Stmt{E e;explicit Throw(E x):e(std::move(x)){}};
struct Import:Stmt{std::string module,alias;Import(std::string m,std::string a=""):module(std::move(m)),alias(std::move(a)){}};
struct FromImport:Stmt{std::string module,name,alias;FromImport(std::string m,std::string n,std::string a=""):module(std::move(m)),name(std::move(n)),alias(std::move(a)){}};
struct If:Stmt{std::vector<std::pair<E,std::vector<S>>> branches;std::vector<S> els;};
struct While:Stmt{E cond;std::vector<S> body;};
struct For:Stmt{std::string var;E iterable;std::vector<S> body;};
struct Try:Stmt{std::vector<S> body;std::string error;std::vector<S> handler;};
struct Match:Stmt{E value;std::vector<std::pair<E,std::vector<S>>> cases;std::vector<S> els;};
struct Function:Stmt{std::string name;std::vector<std::string> params;std::vector<std::string> paramTypes;std::string returnType;std::vector<E> defaults;std::vector<S> body;bool method=false;std::string owner;};
struct Class:Stmt{std::string name,base;std::vector<S> body;bool isStruct=false;};
struct Program{std::vector<S> body;};
std::size_t astNodeCount(const Program& program);
