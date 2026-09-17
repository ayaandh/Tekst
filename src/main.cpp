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
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#endif

static std::string cleanPathArg(std::string p){
    while(!p.empty() && (p.back()=='\n' || p.back()=='\r' || p.back()==' ' || p.back()=='\t')) p.pop_back();
    if(p.size()>=2 && p[p.size()-2]=='\\' && p.back()=='n') p.resize(p.size()-2);
    return p;
}

static std::string readFile(const std::filesystem::path&p){
    std::ifstream f(p);
    if(!f) throw std::runtime_error("cannot open "+p.string());
    std::ostringstream s;
    s<<f.rdbuf();
    return s.str();
}

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
    for(auto const& e:std::filesystem::recursive_directory_iterator(dir))
        if(e.is_regular_file() && e.path().extension()==".tk") found.push_back(std::filesystem::absolute(e.path()));
    std::sort(found.begin(),found.end(),[](const auto&a,const auto&b){
        bool ad=a.filename()=="dec.tk", bd=b.filename()=="dec.tk";
        if(ad!=bd) return ad;
        return a.generic_string()<b.generic_string();
    });
    return found;
}

static std::vector<std::filesystem::path> resolveModule(const std::string& name,const std::filesystem::path& current,const std::filesystem::path& project){
    std::filesystem::path requested(name);
    std::vector<std::filesystem::path> files;
    auto addFile=[&](const std::filesystem::path&p){if(std::filesystem::is_regular_file(p)) files.push_back(std::filesystem::absolute(p));};
    auto addDir=[&](const std::filesystem::path&p){if(files.empty() && std::filesystem::is_directory(p)) files=packageFiles(p);};

    if(requested.extension()==".tk" || requested.has_parent_path()){
        addFile(current/requested);
        addFile(project/requested);
        if(files.empty()) addDir(current/requested);
        addDir(project/requested);
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
        if(module.empty()){
            expanded.push_back(std::move(st));
            continue;
        }
        auto files=resolveModule(module,sourceFile.parent_path(),project);
        if(files.empty()) throw std::runtime_error("Module not found: "+module);
        for(auto const& f:files){
            auto key=f.lexically_normal().string();
            if(loaded.count(key)) continue;
            loaded.insert(key);
            auto childSource=readFile(f);
            auto toks=lex(childSource,f.string());
            auto child=parse(toks,childSource,f.string());
            expandImports(child,f,project,loaded);
            for(auto& x:child.body) expanded.push_back(std::move(x));
        }
        expanded.push_back(std::move(st));
    }
    prog.body=std::move(expanded);
}

static std::string quoteArg(const std::string&s){
    std::string r="\"";
    for(char c:s){
        if(c=='"') r+="\\\"";
        else if(c=='\\') r+="\\\\";
        else r+=c;
    }
    return r+"\"";
}

static std::string quoteShell(const std::string&s){
#ifdef _WIN32
    return quoteArg(s);
#else
    std::string r="'";
    for(char c:s){
        if(c=='\'') r+="'\\''";
        else r+=c;
    }
    return r+"'";
#endif
}

static std::string findOnPath(const std::string&name){
    const char* pathEnv=std::getenv("PATH");
    if(!pathEnv) return {};
#ifdef _WIN32
    const char sep=';';
#else
    const char sep=':';
#endif
    std::string paths(pathEnv);
    size_t start=0;
    while(start<=paths.size()){
        size_t end=paths.find(sep,start);
        std::string part=paths.substr(start,end==std::string::npos?std::string::npos:end-start);
        if(!part.empty()){
            std::filesystem::path p=std::filesystem::path(part)/name;
            if(std::filesystem::is_regular_file(p)) return std::filesystem::absolute(p).string();
#ifdef _WIN32
            if(p.extension().empty()){
                p += ".exe";
                if(std::filesystem::is_regular_file(p)) return std::filesystem::absolute(p).string();
            }
#endif
        }
        if(end==std::string::npos) break;
        start=end+1;
    }
    return {};
}

static std::string compilerFromEnv(){
    const char* v=std::getenv("TEKST_CXX");
    if(v && *v) return v;
    v=std::getenv("CXX");
    if(v && *v) return v;
    return findOnPath(
#ifdef _WIN32
        "clang++.exe"
#else
        "clang++"
#endif
    );
}

#ifdef _WIN32
static std::string findMinGWRoot(){
    const char* v=std::getenv("MINGW_ROOT");
    if(v && *v && std::filesystem::is_directory(v)) return std::filesystem::absolute(v).string();

    std::vector<std::string> candidates={
        "C:/msys64/ucrt64/bin/g++.exe",
        "C:/msys64/mingw64/bin/g++.exe",
        "C:/mingw64/bin/g++.exe",
        "C:/mingw/bin/g++.exe"
    };

    std::string gpp=findOnPath("g++.exe");
    if(!gpp.empty()) candidates.insert(candidates.begin(),gpp);

    for(const auto& c:candidates){
        std::filesystem::path p(c);
        if(!std::filesystem::is_regular_file(p)) continue;
        auto bin=p.parent_path();
        auto root=bin.parent_path();
        if(bin.filename()=="bin") return root.string();
    }
    return {};
}
#endif

static std::vector<std::string> makeCompileArgs(const std::string&compiler,const std::filesystem::path&ir,const std::filesystem::path&runtime,const std::filesystem::path&exe){
    std::vector<std::string> args;
#ifdef _WIN32
    args.push_back(compiler);
    args.push_back("--target=x86_64-w64-windows-gnu");
#else
    args.push_back(compiler);
#endif
    args.push_back("-std=c++17");
    args.push_back("-O2");
    args.push_back("-Wno-override-module");
    args.push_back(ir.string());
    args.push_back(runtime.string());
    args.push_back("-o");
    args.push_back(exe.string());
    return args;
}

static int runProcess(const std::vector<std::string>&args){
    if(args.empty()) return 1;
#ifdef _WIN32
    std::string command;
    for(size_t i=0;i<args.size();++i){
        if(i) command.push_back(' ');
        command+=quoteArg(args[i]);
    }
    std::vector<char> buffer(command.begin(),command.end());
    buffer.push_back('\0');
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb=sizeof(si);
    BOOL ok=CreateProcessA(nullptr,buffer.data(),nullptr,nullptr,TRUE,0,nullptr,nullptr,&si,&pi);
    if(!ok){
        std::cerr<<"error: could not execute compiler '"<<args[0]<<"': "<<GetLastError()<<"\n";
        return 1;
    }
    WaitForSingleObject(pi.hProcess,INFINITE);
    DWORD code=1;
    GetExitCodeProcess(pi.hProcess,&code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return static_cast<int>(code);
#else
    std::vector<char*> argv;
    argv.reserve(args.size()+1);
    std::vector<std::string> mutableArgs=args;
    for(auto& a:mutableArgs) argv.push_back(a.data());
    argv.push_back(nullptr);
    pid_t pid=fork();
    if(pid<0){
        std::cerr<<"error: could not start compiler: "<<std::strerror(errno)<<"\n";
        return 1;
    }
    if(pid==0){
        execvp(argv[0],argv.data());
        std::fprintf(stderr,"error: could not execute compiler '%s': %s\n",argv[0],std::strerror(errno));
        _exit(127);
    }
    int status=0;
    if(waitpid(pid,&status,0)<0){
        std::cerr<<"error: could not wait for compiler: "<<std::strerror(errno)<<"\n";
        return 1;
    }
    if(WIFEXITED(status)) return WEXITSTATUS(status);
    if(WIFSIGNALED(status)) return 128+WTERMSIG(status);
    return 1;
#endif
}

static int runExecutable(const std::filesystem::path&exe){
#ifdef _WIN32
    std::vector<std::string> args={exe.string()};
    return runProcess(args);
#else
    std::vector<std::string> args={exe.string()};
    return runProcess(args);
#endif
}

int main(int argc,char**argv){
    try{
        if(argc<2){
            std::cerr<<"Tekst compiler\nUsage: tekst <file.tekst|file.tk> [-o output] [--emit-ir] [--run]\n";
            return 1;
        }

        std::string input,output;
        bool emit=false,run=false;
        for(int i=1;i<argc;i++){
            std::string a=cleanPathArg(argv[i]);
            if(a=="--emit-ir") emit=true;
            else if(a=="--run") run=true;
            else if(a=="-o" && i+1<argc) output=cleanPathArg(argv[++i]);
            else if(a=="--version"){
                std::cout<<"Tekst 2.0.0 (LLVM backend)\n";
                return 0;
            } else if(input.empty()) input=a;
            else throw std::runtime_error("unknown argument "+a);
        }

        if(input.empty()) throw std::runtime_error("no input file");
        if(output.empty()){
            std::filesystem::path p=input;
            output=(p.parent_path()/p.stem()).string();
        }

        input=cleanPathArg(input);
        std::filesystem::path inputPath=std::filesystem::absolute(std::filesystem::path(input));
        auto source=readFile(inputPath);
        auto toks=lex(source,inputPath.string());
        auto prog=parse(toks,source,inputPath.string());
        std::set<std::string> loaded;
        loaded.insert(inputPath.lexically_normal().string());
        expandImports(prog,inputPath,findProjectRoot(inputPath),loaded);

        auto ll=Codegen().generate(prog);
        std::filesystem::path ir=std::filesystem::absolute(std::filesystem::path(output));
        ir.replace_extension(".ll");
        {
            std::ofstream of(ir);
            if(!of) throw std::runtime_error("cannot write "+ir.string());
            of<<ll;
        }

        if(emit){
            std::cout<<ll;
            return 0;
        }

        std::filesystem::path exe=std::filesystem::absolute(std::filesystem::path(output));
#ifdef _WIN32
        if(exe.extension()!=".exe") exe+=".exe";
#endif

        std::filesystem::path here=std::filesystem::absolute(argv[0]).parent_path();
        std::filesystem::path runtime=here/"runtime.cpp";
        if(!std::filesystem::is_regular_file(runtime)) runtime=std::filesystem::absolute("runtime.cpp");
        if(!std::filesystem::is_regular_file(runtime)) throw std::runtime_error("runtime.cpp not found");

        std::string compiler=compilerFromEnv();
        if(compiler.empty()) throw std::runtime_error("clang++ not found; install LLVM/Clang and MinGW-w64, or set TEKST_CXX");

#ifdef _WIN32
        if(findMinGWRoot().empty()) throw std::runtime_error("MinGW-w64 not found; install MinGW-w64 and make g++.exe available on PATH, or set MINGW_ROOT");
#endif

        int rc=runProcess(makeCompileArgs(compiler,ir,runtime,exe));
        if(rc!=0){
            std::error_code ec;
            std::filesystem::remove(ir,ec);
            return rc;
        }

        std::error_code ec;
        std::filesystem::remove(ir,ec);

        if(run) return runExecutable(exe);
        return 0;
    }catch(const std::exception&e){
        std::string message=e.what();
        if(message.rfind("error:",0)==0) std::cerr<<message<<"\n";
        else std::cerr<<"error: "<<message<<"\n";
        return 1;
    }
}
