#include "builtins.h"
#include <algorithm>
namespace tekst {
std::vector<std::string> builtinNames(){return {"print","input","int","str","bool","float","len","range","alloc","free","ptr_add","load_int","load_byte","store_int","store_byte"};}
bool isBuiltin(const std::string& name){auto names=builtinNames();return std::find(names.begin(),names.end(),name)!=names.end();}
}
