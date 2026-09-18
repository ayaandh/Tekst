#pragma once
#include <string>
#include <vector>
namespace tekst {
bool isBuiltin(const std::string& name);
std::vector<std::string> builtinNames();
}
