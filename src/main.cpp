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
#include <array>
#include <string>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <iomanip>
#include <unordered_set>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#endif

static std::string cleanPathArg(std::string p){
    while(!p.empty() && (p.back()=='\n'||p.back()=='\r'||p.back()==' '||p.back()=='\t')) p.pop_back();
    if(p.size()>=2&&p[p.size()-2]=='\\'&&p.back()=='n') p.resize(p.size()-2);
    return p;
}

static std::string readFile(const std::filesystem::path&p){
    std::ifstream f(p,std::ios::binary);
    if(!f) throw std::runtime_error("cannot open "+p.string());
    std::ostringstream s;s<<f.rdbuf();return s.str();
}

static std::filesystem::path findProjectRoot(std::filesystem::path start){
    start=std::filesystem::absolute(start);
    if(std::filesystem::is_regular_file(start)) start=start.parent_path();
    while(true){
        if(std::filesystem::exists(start/"tekst.toml")||std::filesystem::exists(start/"packages")) return start;
        if(start==start.root_path()) return start;
        start=start.parent_path();
    }
}

static std::vector<std::filesystem::path> packageFiles(const std::filesystem::path&dir){
    std::vector<std::filesystem::path> found;
    if(!std::filesystem::is_directory(dir)) return found;
    for(auto const&e:std::filesystem::recursive_directory_iterator(dir)) if(e.is_regular_file()&&e.path().extension()==".tk") found.push_back(std::filesystem::absolute(e.path()));
    std::sort(found.begin(),found.end(),[](const auto&a,const auto&b){
        bool ad=a.filename()=="dec.tk",bd=b.filename()=="dec.tk";
        if(ad!=bd) return ad;
        return a.generic_string()<b.generic_string();
    });
    return found;
}

static bool isStdModule(const std::string&name){return name=="math"||name=="random"||name=="fs"||name=="time"||name=="os"||name=="http"||name=="socket"||name=="smtp"||name=="stmp"||name=="json";}

static std::vector<std::filesystem::path> resolveModule(const std::string&name,const std::filesystem::path&current,const std::filesystem::path&project){
    std::filesystem::path requested(name);std::vector<std::filesystem::path> files;
    auto addFile=[&](const std::filesystem::path&p){if(std::filesystem::is_regular_file(p)) files.push_back(std::filesystem::absolute(p));};
    auto addDir=[&](const std::filesystem::path&p){if(files.empty()&&std::filesystem::is_directory(p)) files=packageFiles(p);};
    if(requested.extension()==".tk"||requested.has_parent_path()){
        addFile(current/requested);addFile(project/requested);if(files.empty()) addDir(current/requested);addDir(project/requested);
    }else{
        const char*la=std::getenv("LOCALAPPDATA");
        if(la) addDir(std::filesystem::path(la)/"Tekst"/"packages"/requested);
        addDir(project/"packages"/requested);addDir(current/requested);addFile(current/(name+".tk"));addFile(current/"lib"/(name+".tk"));addFile(project/(name+".tk"));
    }
    return files;
}

static void expandImports(Program&prog,const std::filesystem::path&sourceFile,const std::filesystem::path&project,std::set<std::string>&loaded,std::vector<std::filesystem::path>&deps){
    std::vector<S> expanded;
    for(auto&st:prog.body){
        std::string module;
        if(auto im=dynamic_cast<Import*>(st.get())) module=im->module;
        else if(auto fi=dynamic_cast<FromImport*>(st.get())) module=fi->module;
        if(module.empty()){expanded.push_back(std::move(st));continue;}
        if(isStdModule(module)){expanded.push_back(std::move(st));continue;}
        auto files=resolveModule(module,sourceFile.parent_path(),project);
        if(files.empty()) throw std::runtime_error("Module not found: "+module);
        for(auto const&f:files){
            auto key=f.lexically_normal().string();
            if(loaded.count(key)) continue;
            loaded.insert(key);deps.push_back(f);
            auto childSource=readFile(f);auto toks=lex(childSource,f.string());auto child=parse(toks,childSource,f.string());
            expandImports(child,f,project,loaded,deps);
            for(auto&x:child.body) expanded.push_back(std::move(x));
        }
        expanded.push_back(std::move(st));
    }
    prog.body=std::move(expanded);
}

static std::string quoteArg(const std::string&s){
    std::string r="\"";for(char c:s){if(c=='"')r+="\\\"";else if(c=='\\')r+="\\\\";else r+=c;}return r+"\"";
}

static std::string findOnPath(const std::string&name){
    const char*pathEnv=std::getenv("PATH");if(!pathEnv)return{};
#ifdef _WIN32
    const char sep=';';
#else
    const char sep=':';
#endif
    std::string paths(pathEnv);size_t start=0;
    while(start<=paths.size()){
        size_t end=paths.find(sep,start);std::string part=paths.substr(start,end==std::string::npos?std::string::npos:end-start);
        if(!part.empty()){
            std::filesystem::path p=std::filesystem::path(part)/name;
            if(std::filesystem::is_regular_file(p))return std::filesystem::absolute(p).string();
#ifdef _WIN32
            if(p.extension().empty()){p+=".exe";if(std::filesystem::is_regular_file(p))return std::filesystem::absolute(p).string();}
#endif
        }
        if(end==std::string::npos)break;start=end+1;
    }
    return{};
}

static std::string compilerFromEnv(){
    const char*v=std::getenv("TEKST_CXX");if(v&&*v)return v;
    v=std::getenv("CXX");if(v&&*v)return v;
#ifdef _WIN32
    const std::array<std::filesystem::path,4> bundled={
        std::filesystem::path("C:/msys64/ucrt64/bin/clang++.exe"),
        std::filesystem::path("C:/msys64/clang64/bin/clang++.exe"),
        std::filesystem::path("C:/msys64/mingw64/bin/clang++.exe"),
        std::filesystem::path("C:/Program Files/Tekst/toolchain/clang++.exe")
    };
    for(const auto&p:bundled)if(std::filesystem::is_regular_file(p))return std::filesystem::absolute(p).string();
    return findOnPath("clang++.exe");
#else
    return findOnPath("clang++");
#endif
}

static std::string shellEscape(const std::string&s){
#ifdef _WIN32
    return quoteArg(s);
#else
    std::string r="'";for(char c:s){if(c=='\'')r+="'\\''";else r+=c;}return r+"'";
#endif
}

static int runProcess(const std::vector<std::string>&args){
    if(args.empty())return 1;
#ifdef _WIN32
    std::string command;for(size_t i=0;i<args.size();++i){if(i)command.push_back(' ');command+=quoteArg(args[i]);}
    std::vector<char>buffer(command.begin(),command.end());buffer.push_back('\0');STARTUPINFOA si{};PROCESS_INFORMATION pi{};si.cb=sizeof(si);
    BOOL ok=CreateProcessA(nullptr,buffer.data(),nullptr,nullptr,TRUE,0,nullptr,nullptr,&si,&pi);if(!ok){std::cerr<<"error: could not execute '"<<args[0]<<"': "<<GetLastError()<<"\n";return 1;}
    WaitForSingleObject(pi.hProcess,INFINITE);DWORD code=1;GetExitCodeProcess(pi.hProcess,&code);CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return static_cast<int>(code);
#else
    std::vector<char*>argv;std::vector<std::string>mutableArgs=args;for(auto&a:mutableArgs)argv.push_back(a.data());argv.push_back(nullptr);
    pid_t pid=fork();if(pid<0){std::cerr<<"error: could not start process: "<<std::strerror(errno)<<"\n";return 1;}
    if(pid==0){execvp(argv[0],argv.data());std::fprintf(stderr,"error: could not execute '%s': %s\n",argv[0],std::strerror(errno));_exit(127);}
    int status=0;if(waitpid(pid,&status,0)<0)return 1;if(WIFEXITED(status))return WEXITSTATUS(status);if(WIFSIGNALED(status))return 128+WTERMSIG(status);return 1;
#endif
}

static int runProcessWithInput(const std::vector<std::string>&args,const std::string&input){
    if(args.empty())return 1;
#ifdef _WIN32
    SECURITY_ATTRIBUTES sa{};sa.nLength=sizeof(sa);sa.bInheritHandle=TRUE;
    HANDLE readPipe=nullptr,writePipe=nullptr;
    if(!CreatePipe(&readPipe,&writePipe,&sa,0))return 1;
    SetHandleInformation(writePipe,HANDLE_FLAG_INHERIT,0);
    std::string command;for(size_t i=0;i<args.size();++i){if(i)command.push_back(' ');command+=quoteArg(args[i]);}
    std::vector<char>buffer(command.begin(),command.end());buffer.push_back('\0');STARTUPINFOA si{};PROCESS_INFORMATION pi{};si.cb=sizeof(si);si.dwFlags|=STARTF_USESTDHANDLES;si.hStdInput=readPipe;si.hStdOutput=GetStdHandle(STD_OUTPUT_HANDLE);si.hStdError=GetStdHandle(STD_ERROR_HANDLE);
    BOOL ok=CreateProcessA(nullptr,buffer.data(),nullptr,nullptr,TRUE,0,nullptr,nullptr,&si,&pi);CloseHandle(readPipe);if(!ok){CloseHandle(writePipe);return 1;}
    DWORD written=0;size_t off=0;while(off<input.size()){DWORD chunk=0;BOOL w=WriteFile(writePipe,input.data()+off,(DWORD)std::min<size_t>(input.size()-off,1<<20),&chunk,nullptr);if(!w)break;off+=chunk;}CloseHandle(writePipe);
    WaitForSingleObject(pi.hProcess,INFINITE);DWORD code=1;GetExitCodeProcess(pi.hProcess,&code);CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return static_cast<int>(code);
#else
    int pipefd[2];if(pipe(pipefd)!=0)return 1;pid_t pid=fork();if(pid<0){close(pipefd[0]);close(pipefd[1]);return 1;}
    if(pid==0){dup2(pipefd[0],STDIN_FILENO);close(pipefd[0]);close(pipefd[1]);std::vector<char*>av;std::vector<std::string>ma=args;for(auto&a:ma)av.push_back(a.data());av.push_back(nullptr);execvp(av[0],av.data());_exit(127);}
    close(pipefd[0]);size_t off=0;while(off<input.size()){ssize_t n=write(pipefd[1],input.data()+off,input.size()-off);if(n<=0)break;off+=static_cast<size_t>(n);}close(pipefd[1]);int status=0;waitpid(pid,&status,0);if(WIFEXITED(status))return WEXITSTATUS(status);if(WIFSIGNALED(status))return 128+WTERMSIG(status);return 1;
#endif
}

static uint64_t fnv1a(const std::string&s,uint64_t h=1469598103934665603ULL){for(unsigned char c:s){h^=c;h*=1099511628211ULL;}return h;}
static std::string hex64(uint64_t v){std::ostringstream o;o<<std::hex<<std::setw(16)<<std::setfill('0')<<v;return o.str();}

static std::string fileHash(const std::filesystem::path&p,uint64_t h=1469598103934665603ULL){return hex64(fnv1a(readFile(p),h));}

static std::filesystem::path cacheDir(const std::filesystem::path&root){auto p=root/".tekst-cache";std::filesystem::create_directories(p);return p;}

static std::filesystem::path runtimeObject(const std::filesystem::path&runtime,const std::filesystem::path&root,const std::string&compiler,int opt){
    auto text=readFile(runtime);uint64_t h=fnv1a(text);h=fnv1a(compiler,h);h=fnv1a(std::to_string(opt),h);
#ifdef _WIN32
    auto out=cacheDir(root)/("runtime-"+hex64(h)+".obj");
#else
    auto out=cacheDir(root)/("runtime-"+hex64(h)+".o");
#endif
    if(std::filesystem::is_regular_file(out))return out;
    std::filesystem::path tmp=out;tmp+=".tmp";
    std::vector<std::string>a={compiler,"-std=c++17","-O"+std::to_string(opt),"-c",runtime.string(),"-o",tmp.string()};
    int rc=runProcess(a);if(rc!=0){std::error_code ec;std::filesystem::remove(tmp,ec);throw std::runtime_error("failed to build Tekst runtime");}
    std::error_code ec;std::filesystem::rename(tmp,out,ec);if(ec){std::filesystem::remove(out,ec);std::filesystem::rename(tmp,out,ec);}return out;
}

static std::filesystem::path compileIR(const std::string&ir,const std::filesystem::path&root,const std::string&compiler,int opt,bool debug,const std::vector<std::filesystem::path>&deps){
    uint64_t h=fnv1a(ir);h=fnv1a(compiler,h);h=fnv1a(std::to_string(opt),h);h=fnv1a(debug?"debug":"release",h);
    for(auto const&d:deps){std::error_code ec;auto s=readFile(d);h=fnv1a(d.lexically_normal().string(),h);h=fnv1a(s,h);}
#ifdef _WIN32
    auto out=cacheDir(root)/("code-"+hex64(h)+".obj");
#else
    auto out=cacheDir(root)/("code-"+hex64(h)+".o");
#endif
    if(std::filesystem::is_regular_file(out))return out;
    std::filesystem::path tmp=out;tmp+=".tmp";
    std::vector<std::string>a={compiler,"-x","ir","-std=c++17","-O"+std::to_string(opt),"-Wno-override-module"};if(debug)a.push_back("-g");a.push_back("-c");a.push_back("-");a.push_back("-o");a.push_back(tmp.string());
    int rc=runProcessWithInput(a,ir);if(rc!=0){std::error_code ec;std::filesystem::remove(tmp,ec);throw std::runtime_error("LLVM code generation failed");}
    std::error_code ec;std::filesystem::rename(tmp,out,ec);if(ec){std::filesystem::remove(out,ec);std::filesystem::rename(tmp,out,ec);}return out;
}

static int linkExecutable(const std::string&compiler,const std::filesystem::path&code,const std::filesystem::path&runtime,const std::filesystem::path&exe,bool debug){
    std::vector<std::string>a={compiler,"-std=c++17"};if(debug)a.push_back("-g");a.push_back(code.string());a.push_back(runtime.string());
#ifdef _WIN32
    a.push_back("-lws2_32");
#endif
    a.push_back("-o");a.push_back(exe.string());return runProcess(a);
}

static std::filesystem::path locateRuntime(const std::filesystem::path&here){
    std::vector<std::filesystem::path>p={here/"runtime.cpp",here.parent_path()/"runtime.cpp",std::filesystem::current_path()/"runtime.cpp"};
    for(auto const&x:p)if(std::filesystem::is_regular_file(x))return std::filesystem::absolute(x);
    throw std::runtime_error("runtime.cpp not found");
}

static int compileFile(const std::filesystem::path&inputPath,std::filesystem::path output,const std::string&compiler,int opt,bool debug,bool run,const std::filesystem::path&here){
    auto source=readFile(inputPath);auto toks=lex(source,inputPath.string());auto prog=parse(toks,source,inputPath.string());
    std::set<std::string>loaded;loaded.insert(inputPath.lexically_normal().string());std::vector<std::filesystem::path>deps{inputPath};
    expandImports(prog,inputPath,findProjectRoot(inputPath),loaded,deps);
    auto ir=Codegen().generate(prog);
    auto root=findProjectRoot(inputPath);auto runtime=locateRuntime(here);auto rtObj=runtimeObject(runtime,root,compiler,opt);auto codeObj=compileIR(ir,root,compiler,opt,debug,deps);
    if(output.empty())output=inputPath.parent_path()/inputPath.stem();output=std::filesystem::absolute(output);
#ifdef _WIN32
    if(output.extension()!=".exe")output+=".exe";
#endif
    int rc=linkExecutable(compiler,codeObj,rtObj,output,debug);if(rc!=0)return rc;
    if(run){
#ifdef _WIN32
        return runProcess({output.string()});
#else
        return runProcess({output.string()});
#endif
    }
    return 0;
}

static std::vector<std::filesystem::path> discoverTests(const std::filesystem::path&root){
    std::vector<std::filesystem::path>r;auto d=root/"tests";if(!std::filesystem::is_directory(d))return r;
    for(auto const&e:std::filesystem::recursive_directory_iterator(d))if(e.is_regular_file()&&(e.path().extension()==".tk"||e.path().extension()==".tekst"))r.push_back(e.path());
    std::sort(r.begin(),r.end());return r;
}

int main(int argc,char**argv){
    try{
        if(argc<2){std::cerr<<"Tekst compiler\nUsage: tekst <file.tk> [options]\n       tekst build <file.tk> [options]\n       tekst run <file.tk> [options]\n       tekst test [path]\n";return 1;}
        std::string command;int pos=1;std::string first=cleanPathArg(argv[pos]);
        if(first=="build"||first=="run"||first=="test"){command=first;++pos;}
        if(first=="--version"){std::cout<<"Tekst 2.3.0 LLVM backend\n";return 0;}
        if(command=="test"){
            std::filesystem::path root=pos<argc?std::filesystem::absolute(cleanPathArg(argv[pos])):std::filesystem::current_path();
            if(std::filesystem::is_regular_file(root))root=root.parent_path();
            if(root.filename()=="tests")root=root.parent_path();
            auto compiler=compilerFromEnv();if(compiler.empty())throw std::runtime_error("clang++ not found");
            auto here=std::filesystem::absolute(argv[0]).parent_path();auto tests=discoverTests(root);if(tests.empty())throw std::runtime_error("no tests found");
            int failed=0;for(auto const&t:tests){std::cout<<"TEST "<<t.lexically_relative(root).string()<<"\n";std::filesystem::path out=cacheDir(root)/("test-"+t.stem().string());int rc=compileFile(t,out,compiler,0,true,true,here);if(rc!=0)++failed;}
            std::cout<<(tests.size()-failed)<<" passed, "<<failed<<" failed\n";return failed?1:0;
        }
        std::string input,output;bool run=command=="run";bool debug=false;int opt=2;
        for(int i=pos;i<argc;++i){std::string a=cleanPathArg(argv[i]);if(a=="--run")run=true;else if(a=="--debug"){debug=true;opt=0;}else if(a=="--release"){debug=false;opt=3;}else if(a=="-O0")opt=0;else if(a=="-O1")opt=1;else if(a=="-O2")opt=2;else if(a=="-O3")opt=3;else if(a=="-o"&&i+1<argc)output=cleanPathArg(argv[++i]);else if(a=="--emit-ir"){if(input.empty())throw std::runtime_error("--emit-ir requires an input file");auto source=readFile(std::filesystem::absolute(input));auto toks=lex(source,input);auto prog=parse(toks,source,input);std::cout<<Codegen().generate(prog);return 0;}else if(a=="--version"){std::cout<<"Tekst 2.3.0 LLVM backend\n";return 0;}else if(input.empty())input=a;else throw std::runtime_error("unknown argument "+a);}
        if(input.empty())throw std::runtime_error("no input file");
        auto inputPath=std::filesystem::absolute(std::filesystem::path(input));
        auto compiler=compilerFromEnv();if(compiler.empty())throw std::runtime_error("clang++ not found; install LLVM/Clang or set TEKST_CXX");
        auto here=std::filesystem::absolute(argv[0]).parent_path();
        return compileFile(inputPath,output,compiler,opt,debug,run,here);
    }catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}
}
