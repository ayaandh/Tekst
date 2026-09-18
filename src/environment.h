#pragma once
#include <map>
#include <string>
#include <optional>
namespace tekst {
class Environment { std::map<std::string,std::string> values; public: void set(const std::string& name,const std::string& value); std::optional<std::string> get(const std::string& name) const; bool contains(const std::string& name) const; void clear(); };
}
