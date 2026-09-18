#include "repl.h"
#include <iostream>
#include <string>
namespace tekst {
int repl(){std::string line;while(std::cout<<">> "&&std::getline(std::cin,line)){if(line=="exit"||line=="quit")break;if(!line.empty())std::cout<<line<<'\n';}return 0;}
}
