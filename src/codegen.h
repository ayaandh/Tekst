
#pragma once
#include "ast.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>

class Codegen {
    struct FnInfo { Function* f; std::string symbol; std::string owner; };
    std::unordered_map<std::string,FnInfo> functions;
    std::unordered_map<std::string,Class*> classes;
    std::unordered_map<std::string,std::string> importedNames;
    std::unordered_map<std::string,bool> importedModules;
    std::ostringstream ir, body, allocas, globals;
    int nextId=0, nextBlock=0;
    std::string currentFn, currentOwner;
    std::unordered_map<std::string,std::string> slots;
    std::unordered_map<std::string,std::string> types;
    std::vector<std::string> breakTargets, continueTargets;
    std::vector<Expr*> defers;
    std::ostringstream lambdaFunctions;
    int lambdaId=0;
    bool terminated=false;
    int controlKind=0;
    std::string tmp(){return "%v"+std::to_string(++nextId);}
    std::string label(const std::string&s){return s+std::to_string(++nextBlock);}
    std::string emitExpr(Expr*);
    std::string emitLambda(Lambda*);
    void emitStmt(Stmt*);
    void emitBlock(const std::vector<S>&);
    std::string loadVar(const std::string&);
    void storeVar(const std::string&,const std::string&,const std::string& type="");
    void ensureSlot(const std::string&);
    void emitFunction(Function*);
    std::string escape(const std::string&);
public:
    std::string generate(const Program&);
};
