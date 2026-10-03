#include "errors.h"
#include <sstream>
namespace tekst {
std::string formatError(const std::string& message,const std::string& file,int line,int column,const std::string& source){std::ostringstream out;out<<"error: "<<message<<"\n --> "<<(file.empty()?"<source>":file)<<":"<<line<<":"<<column<<"\n  |\n"<<line<<" | "<<source<<"\n    | ";for(int i=1;i<column;i++)out<<' ';out<<"^\n";return out.str();}
}
