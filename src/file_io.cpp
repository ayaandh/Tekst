#include "file_io.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
namespace tekst::io {
std::string readText(const std::string& path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("cannot read file: "+path);std::ostringstream s;s<<f.rdbuf();return s.str();}
void writeText(const std::string& path,const std::string& data){std::ofstream f(path,std::ios::binary|std::ios::trunc);if(!f)throw std::runtime_error("cannot write file: "+path);f<<data;}
void appendText(const std::string& path,const std::string& data){std::ofstream f(path,std::ios::binary|std::ios::app);if(!f)throw std::runtime_error("cannot write file: "+path);f<<data;}
bool exists(const std::string& path){return std::filesystem::exists(path);}
}
