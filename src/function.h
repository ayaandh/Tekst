#pragma once
#include <string>
#include <vector>
namespace tekst {
struct FunctionSignature { std::string name; std::vector<std::string> parameters; std::string returnType; bool variadic=false; };
bool validFunctionName(const std::string& name);
}
