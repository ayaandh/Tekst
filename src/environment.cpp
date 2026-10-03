#include "environment.h"
namespace tekst {
void Environment::set(const std::string& name,const std::string& value){values[name]=value;}
std::optional<std::string> Environment::get(const std::string& name) const{auto it=values.find(name);return it==values.end()?std::nullopt:std::optional<std::string>(it->second);}
bool Environment::contains(const std::string& name) const{return values.find(name)!=values.end();}
void Environment::clear(){values.clear();}
}
