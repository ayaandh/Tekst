
#include "parser.h"
#include <stdexcept>
#include <sstream>
class Parser{
 const std::vector<Token>&t;size_t p=0;
 Token&cur(){return const_cast<Token&>(t[p]);}
 bool is(TokenKind k,const std::string&s=""){return cur().kind==k&&(s.empty()||cur().text==s);}
 bool word(const std::string&s){return is(TokenKind::Identifier,s);}
 void take(TokenKind k,const std::string&s=""){if(!is(k,s))err("expected "+(s.empty()?std::to_string((int)k):s));++p;}
 void nl(){if(is(TokenKind::Newline))++p;}
 [[noreturn]]void err(const std::string&m){throw std::runtime_error("line "+std::to_string(cur().line)+": "+m);}
 std::vector<S> block(){
   take(TokenKind::Newline);
   // Blank lines are valid inside an indented block.
   while(is(TokenKind::Newline)) ++p;
   take(TokenKind::Indent); std::vector<S> b;
   while(!is(TokenKind::Dedent)&&!is(TokenKind::End)){if(is(TokenKind::Newline)){++p;continue;}b.push_back(stmt());}
   take(TokenKind::Dedent);return b;
 }
 E expr(){return logicalOr();}
 E logicalOr(){auto x=logicalAnd();while(word("or")){++p;x=std::make_unique<Binary>("or",std::move(x),logicalAnd());}return x;}
 E logicalAnd(){auto x=equality();while(word("and")){++p;x=std::make_unique<Binary>("and",std::move(x),equality());}return x;}
 E equality(){
   auto x=compare();while(is(TokenKind::EqEq)||is(TokenKind::NotEq)){auto o=cur().text;++p;x=std::make_unique<Binary>(o,std::move(x),compare());}return x;
 }
 E compare(){
   auto x=term();while(is(TokenKind::Lt)||is(TokenKind::Le)||is(TokenKind::Gt)||is(TokenKind::Ge)){auto o=cur().text;++p;x=std::make_unique<Binary>(o,std::move(x),term());}return x;
 }
 E term(){auto x=factor();while(is(TokenKind::Plus)||is(TokenKind::Minus)){auto o=cur().text;++p;x=std::make_unique<Binary>(o,std::move(x),factor());}return x;}
 E factor(){auto x=unary();while(is(TokenKind::Star)||is(TokenKind::Slash)||is(TokenKind::Percent)){auto o=cur().text;++p;x=std::make_unique<Binary>(o,std::move(x),unary());}return x;}
 E unary(){if(is(TokenKind::Minus)||is(TokenKind::Star)||is(TokenKind::Ampersand)||word("not")){auto o=cur().text;++p;return std::make_unique<Unary>(o,unary());}return postfix();}
 E postfix(){
   auto x=primary();
   while(true){
     if(is(TokenKind::LParen)){++p;std::vector<E>a;if(!is(TokenKind::RParen)){do{a.push_back(expr());if(!is(TokenKind::Comma))break;++p;}while(!is(TokenKind::RParen));}take(TokenKind::RParen);x=std::make_unique<Call>(std::move(x),std::move(a));}
     else if(is(TokenKind::LBracket)){++p;auto i=expr();take(TokenKind::RBracket);x=std::make_unique<Index>(std::move(x),std::move(i));}
     else if(is(TokenKind::Dot)){++p;if(!is(TokenKind::Identifier))err("expected attribute");auto n=cur().text;++p;x=std::make_unique<Attr>(std::move(x),n);}
     else break;
   }return x;
 }
 E primary(){
   if(is(TokenKind::Number)){auto x=std::make_unique<Number>(cur().text);++p;return x;}
   if(is(TokenKind::String)){auto x=std::make_unique<String>(cur().text);++p;return x;}
   if(word("True")||word("true")||word("False")||word("false")){bool b=word("True")||word("true");++p;return std::make_unique<Bool>(b);}
   if(word("None")||word("null")){++p;return std::make_unique<NoneExpr>();}
   if(is(TokenKind::Identifier)){auto x=std::make_unique<Name>(cur().text);++p;return x;}
   if(is(TokenKind::LParen)){++p;auto x=expr();take(TokenKind::RParen);return x;}
   if(is(TokenKind::LBracket)){++p;auto x=std::make_unique<List>();if(!is(TokenKind::RBracket)){do{x->xs.push_back(expr());if(!is(TokenKind::Comma))break;++p;}while(!is(TokenKind::RBracket));}take(TokenKind::RBracket);return x;}
   if(is(TokenKind::LBrace)){++p;auto x=std::make_unique<Dict>();if(!is(TokenKind::RBrace)){do{auto k=expr();take(TokenKind::Colon);x->xs.emplace_back(std::move(k),expr());if(!is(TokenKind::Comma))break;++p;}while(!is(TokenKind::RBrace));}take(TokenKind::RBrace);return x;}
   err("expected expression");return {};
 }
 S stmt(){
   if(word("import")){
     ++p;
     if(!is(TokenKind::Identifier)) err("expected module name");
     std::string m=cur().text; ++p;
     // Preserve Tekst's original explicit .tk/path import syntax, e.g.
     //   import hello.tk
     //   import lib.tools.tk
     while(is(TokenKind::Dot)){
       ++p;
       if(!is(TokenKind::Identifier)) err("expected module path component");
       m += "." + cur().text; ++p;
     }
     std::string a;
     if(word("as")){++p;if(!is(TokenKind::Identifier))err("expected import alias");a=cur().text;++p;}
     nl();
     return std::make_unique<Import>(m,a);
   }
   if(word("from")){
     ++p;
     if(!is(TokenKind::Identifier)) err("expected module name");
     std::string m=cur().text; ++p;
     while(is(TokenKind::Dot)){
       ++p;
       if(!is(TokenKind::Identifier)) err("expected module path component");
       m += "." + cur().text; ++p;
     }
     if(!word("import"))err("expected 'import'");
     ++p;
     if(!is(TokenKind::Identifier))err("expected imported name");
     std::string n=cur().text;++p;
     std::string a;
     if(word("as")){++p;if(!is(TokenKind::Identifier))err("expected import alias");a=cur().text;++p;}
     nl();
     return std::make_unique<FromImport>(m,n,a);
   }
   if(word("if")){++p;auto z=std::make_unique<If>();auto c=expr();take(TokenKind::Colon);z->branches.push_back({std::move(c),block()});
     while(word("elif")){++p;auto cc=expr();take(TokenKind::Colon);z->branches.push_back({std::move(cc),block()});}
     if(word("else")){++p;take(TokenKind::Colon);z->els=block();}return z;}
   if(word("while")){++p;auto z=std::make_unique<While>();z->cond=expr();take(TokenKind::Colon);z->body=block();return z;}
   if(word("for")){++p;if(!is(TokenKind::Identifier))err("expected loop variable");auto n=cur().text;++p;if(!word("in"))err("expected 'in'");++p;auto z=std::make_unique<For>();z->var=n;z->iterable=expr();take(TokenKind::Colon);z->body=block();return z;}
   if(word("try")){++p;auto z=std::make_unique<Try>();take(TokenKind::Colon);z->body=block();if(!word("catch"))err("try must be followed by catch");++p;if(is(TokenKind::Identifier)){z->error=cur().text;++p;}take(TokenKind::Colon);z->handler=block();return z;}
   if(word("break")){++p;nl();return std::make_unique<Break>();}
   if(word("continue")){++p;nl();return std::make_unique<Continue>();}
   if(word("throw")){++p;auto e=expr();nl();return std::make_unique<Throw>(std::move(e));}
   if(word("return")){++p;E e;if(!is(TokenKind::Newline)&&!is(TokenKind::Dedent)&&!is(TokenKind::End))e=expr();nl();return std::make_unique<Return>(std::move(e));}
   if(word("print")){++p;take(TokenKind::LParen);auto e=expr();take(TokenKind::RParen);nl();return std::make_unique<Print>(std::move(e));}
   if(word("fn")||word("def"))return function(false,"");
   if(word("class")){++p;if(!is(TokenKind::Identifier))err("expected class name");auto z=std::make_unique<Class>();z->name=cur().text;++p;if(word("extends")){++p;if(!is(TokenKind::Identifier))err("expected base class");z->base=cur().text;++p;}take(TokenKind::Colon);take(TokenKind::Newline);take(TokenKind::Indent);
     while(!is(TokenKind::Dedent)&&!is(TokenKind::End)){if(is(TokenKind::Newline)){++p;continue;}if(word("fn")||word("def"))z->body.push_back(function(true,z->name));else err("class body currently accepts methods declared with fn/def");}
     take(TokenKind::Dedent);return z;}
   // Explicit class declaration: ClassName variable
   if(is(TokenKind::Identifier)&&p+1<t.size()&&t[p+1].kind==TokenKind::Identifier){
      auto type=cur().text; ++p; auto name=cur().text;++p; nl();
      return std::make_unique<Assign>(std::make_unique<Name>(name),std::make_unique<Call>(std::make_unique<Name>(type),std::vector<E>{}));
   }
   if(word("let")){++p;}
   auto a=expr();
   if(is(TokenKind::Eq)||is(TokenKind::PlusEq)||is(TokenKind::MinusEq)||is(TokenKind::StarEq)||is(TokenKind::SlashEq)){
      auto o=cur().text;++p;auto v=expr();nl();return std::make_unique<Assign>(std::move(a),std::move(v),o);
   }
   nl();return std::make_unique<ExprStmt>(std::move(a));
 }
 S function(bool method,std::string owner){
   ++p;if(!is(TokenKind::Identifier))err("expected function name");auto z=std::make_unique<Function>();z->name=cur().text;z->method=method;z->owner=owner;++p;take(TokenKind::LParen);
   if(!is(TokenKind::RParen)){do{if(!is(TokenKind::Identifier))err("expected parameter");std::string pn=cur().text;++p;E d;if(is(TokenKind::Eq)){++p;d=expr();}z->params.push_back(pn);z->defaults.push_back(std::move(d));if(!is(TokenKind::Comma))break;++p;}while(!is(TokenKind::RParen));}
   take(TokenKind::RParen);take(TokenKind::Colon);z->body=block();return z;
 }
public:Parser(const std::vector<Token>&x):t(x){}
 Program run(){Program z;while(!is(TokenKind::End)){if(is(TokenKind::Newline)){++p;continue;}z.body.push_back(stmt());}return z;}
};
Program parse(const std::vector<Token>&t){return Parser(t).run();}
