#include "function.h"
#include <cctype>
namespace tekst {
bool validFunctionName(const std::string& name){if(name.empty()||!(std::isalpha((unsigned char)name[0])||name[0]=='_'))return false;for(char c:name)if(!(std::isalnum((unsigned char)c)||c=='_'))return false;return true;}
}
