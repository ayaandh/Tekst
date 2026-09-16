
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <set>
#include <algorithm>

static std::string cleanPathArg(std::string p){
    while(!p.empty() && (p.back()=='\n' || p.back()=='\r' || p.back()==' ' || p.back()=='\t')) p.pop_back();
    if(p.size()>=2 && p[p.size()-2]=='\\' && p.back()=='n') p.resize(p.size()-2);
    return p;
}

static std::string readFile(const std::string&p){std::ifstream f(p);if(!f)throw std::runtime_error("cannot open "+p);std::ostringstream s;s<<f.rdbuf();return s.str();}
static std::string readFile(const std::filesystem::path&p){std::ifstream f(p);if(!f)throw std::runtime_error("cannot open "+p.string());std::ostringstream s;s<<f.rdbuf();return s.str();}

static std::filesystem::path findProjectRoot(std::filesystem::path start){
    start=std::filesystem::absolute(start);
    if(std::filesystem::is_regular_file(start)) start=start.parent_path();
    while(true){
        if(std::filesystem::exists(start/"tekst.toml") || std::filesystem::exists(start/"packages")) return start;
        if(start==start.root_path()) return start;
        start=start.parent_path();
    }
}

static std::vector<std::filesystem::path> packageFiles(const std::filesystem::path& dir){
    std::vector<std::filesystem::path> found;
    if(!std::filesystem::is_directory(dir)) return found;
    for(auto const& e: std::filesystem::recursive_directory_iterator(dir))
        if(e.is_regular_file() && e.path().extension()==".tk") found.push_back(std::filesystem::absolute(e.path()));
    std::sort(found.begin(),found.end(),[](const auto&a,const auto&b){
        bool ad=a.filename()=="dec.tk", bd=b.filename()=="dec.tk";
        if(ad!=bd) return ad;
        return a.generic_string()<b.generic_string();
    });
    return found;
}

static std::vector<std::filesystem::path> resolveModule(const std::string& name,const std::filesystem::path& current,const std::filesystem::path& project){
    std::filesystem::path requested(name), normalized=name;
    std::vector<std::filesystem::path> files;
    auto addFile=[&](const std::filesystem::path&p){if(std::filesystem::is_regular_file(p)) files.push_back(std::filesystem::absolute(p));};
    auto addDir=[&](const std::filesystem::path&p){if(files.empty() && std::filesystem::is_directory(p)) files=packageFiles(p);};

    if(requested.extension()==".tk" || requested.has_parent_path()){
        addFile(current/requested); addFile(project/requested);
        if(files.empty()) addDir(current/requested); addDir(project/requested);
    } else {
        const char* la=std::getenv("LOCALAPPDATA");
        if(la) addDir(std::filesystem::path(la)/"Tekst"/"packages"/requested);
        addDir(project/"packages"/requested);
        addDir(current/requested);
        addFile(current/(name+".tk"));
        addFile(current/"lib"/(name+".tk"));
        addFile(project/(name+".tk"));
    }
    return files;
}

static void expandImports(Program& prog,const std::filesystem::path& sourceFile,const std::filesystem::path& project,std::set<std::string>& loaded){
    std::vector<S> expanded;
    for(auto& st:prog.body){
        std::string module;
        if(auto im=dynamic_cast<Import*>(st.get())) module=im->module;
        else if(auto fi=dynamic_cast<FromImport*>(st.get())) module=fi->module;
        if(module.empty()){ expanded.push_back(std::move(st)); continue; }
        auto files=resolveModule(module,sourceFile.parent_path(),project);
        if(files.empty()) throw std::runtime_error("Module not found: "+module);
        for(auto const& f:files){
            auto key=f.lexically_normal().string();
            if(loaded.count(key)) continue;
            loaded.insert(key);
            auto toks=lex(readFile(f));
            auto child=parse(toks);
            expandImports(child,f,project,loaded);
            for(auto& x:child.body) expanded.push_back(std::move(x));
        }
        // Keep the import statement so LLVM codegen knows module/alias metadata.
        expanded.push_back(std::move(st));
    }
    prog.body=std::move(expanded);
}

static std::string quote(const std::string&s){
#ifdef _WIN32
   std::string r="\"";
   for(char c:s){if(c=='\"')r+="\\\"";else r+=c;}
   return r+"\"";
#else
   std::string r="'";for(char c:s){if(c=='\'')r+="'\\''";else r+=c;}return r+"'";
#endif
}
int main(int argc,char**argv){
 try{
   if(argc<2){std::cerr<<"Tekst compiler\nUsage: tekst <file.tekst|file.tk> [-o output] [--emit-ir] [--run]\n";return 1;}
   std::string input,output;bool emit=false,run=false;
   for(int i=1;i<argc;i++){std::string a=cleanPathArg(argv[i]);if(a=="--emit-ir")emit=true;else if(a=="--run")run=true;else if(a=="-o"&&i+1<argc)output=argv[++i];else if(a=="--version"){std::cout<<"Tekst 2.0.0 (LLVM backend)\n";return 0;}else if(input.empty())input=a;else throw std::runtime_error("unknown argument "+a);}
   if(input.empty())throw std::runtime_error("no input file");
   if(output.empty()){std::filesystem::path p=input;output=(p.parent_path()/(p.stem().string())).string();}
   input=cleanPathArg(input);
   std::filesystem::path inputPath=std::filesystem::absolute(std::filesystem::path(input));
   auto toks=lex(readFile(inputPath));auto prog=parse(toks);
   std::set<std::string> loaded; loaded.insert(inputPath.lexically_normal().string());
   expandImports(prog,inputPath,findProjectRoot(inputPath),loaded);
   auto ll=Codegen().generate(prog);
   std::filesystem::path ir=std::filesystem::path(output).replace_extension(".ll");
   std::ofstream of(ir);of<<ll;of.close();
   if(emit){std::cout<<ll;return 0;}
   std::filesystem::path exe=output;
#ifdef _WIN32
   if(exe.extension()!=".exe")exe+=".exe";
#endif
   std::filesystem::path here=std::filesystem::absolute(argv[0]).parent_path();
   std::filesystem::path runtime=here/"runtime.cpp";
   std::string clang="clang++";
#ifdef _WIN32
   if(std::filesystem::exists("C:/Program Files/LLVM/bin/clang++.exe"))
      clang="C:/Program Files/LLVM/bin/clang++.exe";
#endif
#ifdef _WIN32
   std::string cmd="\""+clang+"\" -std=c++17 -O2 "+quote(ir.string())+" "+quote(runtime.string())+" -o "+quote(exe.string());
   // clang++ targets MSVC on Windows, so it needs the Visual Studio
   // developer environment to locate the MSVC C++ standard library headers.
   std::string vsdev;
   const char* pf86=std::getenv("ProgramFiles(x86)");
   if(pf86) {
      std::filesystem::path p=std::filesystem::path(pf86)/"Microsoft Visual Studio/2019/BuildTools/Common7/Tools/VsDevCmd.bat";
      if(std::filesystem::exists(p)) vsdev=p.string();
   }
   if(vsdev.empty()) {
      const char* pf=std::getenv("ProgramFiles");
      if(pf) {
         std::filesystem::path p=std::filesystem::path(pf)/"Microsoft Visual Studio/2019/BuildTools/Common7/Tools/VsDevCmd.bat";
         if(std::filesystem::exists(p)) vsdev=p.string();
      }
   }
   if(!vsdev.empty())
      cmd="call \""+vsdev+"\" -arch=x64 && "+cmd;
#else
   std::string cmd=quote(clang)+" -std=c++17 -O2 "+quote(ir.string())+" "+quote(runtime.string())+" -o "+quote(exe.string());
#endif
   int rc=std::system(cmd.c_str());if(rc!=0){std::cerr<<"Tekst: clang++ failed. Check the compiler output above.\n";return rc;}
   if(run){std::string rcmd=quote(exe.string());return std::system(rcmd.c_str());}
   std::cout<<"compiled "<<input<<" -> "<<exe.string()<<"\n";
   return 0;
 }catch(const std::exception&e){std::cerr<<"Tekst error: "<<e.what()<<"\n";return 1;}
}
