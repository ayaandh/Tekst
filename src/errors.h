#pragma once
#include <stdexcept>
#include <string>
namespace tekst {
class Error : public std::runtime_error { public: explicit Error(const std::string& message):std::runtime_error(message){} };
std::string formatError(const std::string& message,const std::string& file,int line,int column,const std::string& source);
}
