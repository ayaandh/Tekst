#pragma once
#include <string>
namespace tekst::io {
std::string readText(const std::string& path);
void writeText(const std::string& path,const std::string& data);
void appendText(const std::string& path,const std::string& data);
bool exists(const std::string& path);
}
