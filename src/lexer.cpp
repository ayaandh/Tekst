
#include "token.h"
#include <cctype>
#include <stdexcept>
#include <sstream>
#include <iomanip>

static bool isid0(char c){ return std::isalpha((unsigned char)c)||c=='_'; }
static bool isid(char c){ return std::isalnum((unsigned char)c)||c=='_'; }

std::vector<Token> lex(const std::string& s, const std::string& filename) {
    std::vector<std::string> sourceLines;
    {
        std::istringstream in(s);
        std::string line;
        while(std::getline(in,line)) sourceLines.push_back(line);
        if(s.empty() || (!s.empty() && s.back()=='\n')) sourceLines.push_back({});
    }
    auto fail = [&](const std::string& message, int l, int c, const std::string& help = {}) -> void {
        std::string text = (l >= 1 && static_cast<size_t>(l) <= sourceLines.size()) ? sourceLines[static_cast<size_t>(l)-1] : std::string{};
        std::ostringstream out;
        out << "error: " << message << "\n";
        out << " --> " << (filename.empty() ? "<source>" : filename) << ":" << l << ":" << c << "\n";
        out << "  |\n";
        out << std::setw(3) << l << " | " << text << "\n";
        out << "    | ";
        for(int n=1;n<c;++n) out << ((n-1 < static_cast<int>(text.size()) && text[static_cast<size_t>(n-1)]=='\t') ? '\t' : ' ');
        out << "^\n";
        if(!help.empty()) out << "  = help: " << help << "\n";
        throw std::runtime_error(out.str());
    };
    std::vector<Token> out;
    std::vector<int> ind{0};
    size_t i=0; int line=1,col=1; bool bol=true;
    auto add=[&](TokenKind k,std::string t,int l,int c){out.push_back({k,std::move(t),l,c});};
    while(i<s.size()){
        if(bol){
            int n=0;
            while(i<s.size() && (s[i]==' '||s[i]=='\t')){
                n += s[i]=='\t' ? 4 : 1; ++i; ++col;
            }
            if(i<s.size() && s[i]!='\n' && s[i]!='\r' && s[i]!='#'){
                if(n>ind.back()){
                    ind.push_back(n);
                    add(TokenKind::Indent,"",line,col);
                } else if(n<ind.back()){
                    while(n<ind.back()){
                        ind.pop_back();
                        add(TokenKind::Dedent,"",line,col);
                    }
                    if(n!=ind.back())
                        throw std::runtime_error("inconsistent indentation at line "+std::to_string(line));
                }
            }
            bol=false;
        }
        if(i>=s.size()) break;
        char c=s[i];
        if(c=='\r'){++i; continue;}
        if(c=='\n'){add(TokenKind::Newline,"",line,col);++i;++line;col=1;bol=true;continue;}
        if(c=='#'){while(i<s.size()&&s[i]!='\n'){++i;++col;}continue;}
        if(std::isspace((unsigned char)c)){++i;++col;continue;}
        int l=line,cc=col;
        if(isid0(c)){
            size_t b=i++; ++col; while(i<s.size()&&isid(s[i])){++i;++col;}
            add(TokenKind::Identifier,s.substr(b,i-b),l,cc); continue;
        }
        if(std::isdigit((unsigned char)c)){
            size_t b=i++; ++col; bool dot=false;
            while(i<s.size()&&(std::isdigit((unsigned char)s[i])||(!dot&&s[i]=='.'))){dot|=s[i]=='.';++i;++col;}
            add(TokenKind::Number,s.substr(b,i-b),l,cc); continue;
        }
        if(c=='"'||c=='\''){
            char q=c; ++i;++col; std::string v;
            while(i<s.size()&&s[i]!=q){
                if(s[i]=='\\'&&i+1<s.size()){
                    char e=s[i+1]; v += e=='n'?'\n':e=='t'?'\t':e=='r'?'\r':e; i+=2;col+=2;
                } else {v+=s[i++];++col;}
            }
            if(i>=s.size()) fail("unterminated string literal", l, cc, "close the string with " + std::string(1,q) + " before the end of the file");
            ++i;++col; add(TokenKind::String,v,l,cc); continue;
        }
        auto two=[&](char a,char b,TokenKind k){if(i+1<s.size()&&s[i]==a&&s[i+1]==b){add(k,s.substr(i,2),l,cc);i+=2;col+=2;return true;}return false;};
        if(two('+','=',TokenKind::PlusEq)||two('-','=',TokenKind::MinusEq)||two('*','=',TokenKind::StarEq)||two('/','=',TokenKind::SlashEq)||
           two('=','=',TokenKind::EqEq)||two('!','=',TokenKind::NotEq)||two('<','=',TokenKind::Le)||two('>','=',TokenKind::Ge)) continue;
        if(i+1<s.size()&&s[i]=='?'&&s[i+1]=='.'){add(TokenKind::QDot,"?.",l,cc);i+=2;col+=2;continue;}
        if(i+1<s.size()&&s[i]=='-'&&s[i+1]=='>'){add(TokenKind::Arrow,"->",l,cc);i+=2;col+=2;continue;}
        TokenKind k;
        switch(c){
            case '+':k=TokenKind::Plus;break;case '-':k=TokenKind::Minus;break;case '*':k=TokenKind::Star;break;
            case '/':k=TokenKind::Slash;break;case '%':k=TokenKind::Percent;break;case '=':k=TokenKind::Eq;break;
            case '<':k=TokenKind::Lt;break;case '>':k=TokenKind::Gt;break;case '(':k=TokenKind::LParen;break;case ')':k=TokenKind::RParen;break;
            case '[':k=TokenKind::LBracket;break;case ']':k=TokenKind::RBracket;break;case '{':k=TokenKind::LBrace;break;case '}':k=TokenKind::RBrace;break;
            case ',':k=TokenKind::Comma;break;case ':':k=TokenKind::Colon;break;case '.':k=TokenKind::Dot;break;case '&':k=TokenKind::Ampersand;break;
            default: fail("unexpected character '"+std::string(1,c)+"'", line, col, "remove this character or use a valid Tekst operator or delimiter");
        }
        add(k,std::string(1,c),l,cc);++i;++col;
    }
    while(ind.size()>1){ind.pop_back();add(TokenKind::Dedent,"",line,col);}
    add(TokenKind::End,"",line,col);
    return out;
}
