#pragma once
#include <string>
namespace tekst {
struct ClassInfo { std::string name; std::string base; bool isStruct=false; };
bool validTypeName(const std::string& name);
}
