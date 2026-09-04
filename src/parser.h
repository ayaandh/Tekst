#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <optional>
#include <variant>
#include <filesystem>
#include <functional>

#include "lexer.h"

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::string toString() const = 0;
};

class Expression : public ASTNode {
public:
    virtual ~Expression() = default;
};

struct RuntimeObject;
struct RuntimeFunction;
struct NativeFunction;

using RuntimeValue = std::variant<
    int,
    bool,
    std::string,
    double,
    std::shared_ptr<RuntimeObject>,
    std::shared_ptr<RuntimeFunction>,
    std::shared_ptr<NativeFunction>
>;

struct NativeFunction {
    std::string name;
    std::function<RuntimeValue(const std::vector<RuntimeValue>&)> call;
};

struct RuntimeObject {
    std::string className;
    std::map<std::string, RuntimeValue> fields;
};

struct RuntimeFunction {
    std::string name;
    std::vector<std::string> parameters;
    std::vector<std::shared_ptr<Expression>> defaultArgs;
    std::string variadicParameter;
    std::vector<std::shared_ptr<ASTNode>> body;
    std::map<std::string, RuntimeValue> closure;
    std::shared_ptr<RuntimeObject> self;
};

class NullLiteral : public Expression {
public:
    std::string toString() const override;
};

class NumberLiteral : public Expression {
public:
    int value;

    NumberLiteral(int v) : value(v) {}

    std::string toString() const override;
};

class FloatLiteral : public Expression {
public:
    double value;

    FloatLiteral(double v) : value(v) {}

    std::string toString() const override;
};

class BooleanLiteral : public Expression {
public:
    bool value;

    BooleanLiteral(bool v) : value(v) {}

    std::string toString() const override;
};

class StringLiteral : public Expression {
public:
    std::string value;

    StringLiteral(const std::string& v) : value(v) {}

    std::string toString() const override;
};

class Identifier : public Expression {
public:
    std::string name;

    Identifier(const std::string& n) : name(n) {}

    std::string toString() const override;
};

class UnaryOp : public Expression {
public:
    std::string op;
    std::shared_ptr<Expression> operand;

    UnaryOp(const std::string& o, std::shared_ptr<Expression> e)
        : op(o), operand(e) {}

    std::string toString() const override;
};

class BinaryOp : public Expression {
public:
    std::shared_ptr<Expression> left;
    std::string op;
    std::shared_ptr<Expression> right;

    BinaryOp(
        std::shared_ptr<Expression> l,
        const std::string& o,
        std::shared_ptr<Expression> r
    ) : left(l), op(o), right(r) {}

    std::string toString() const override;
};

class CallExpression : public Expression {
public:
    std::string callee;
    std::vector<std::shared_ptr<Expression>> args;

    CallExpression(
        const std::string& n,
        const std::vector<std::shared_ptr<Expression>>& a
    ) : callee(n), args(a) {}

    std::string toString() const override;
};

class AttributeAccess : public Expression {
public:
    std::shared_ptr<Expression> object;
    std::string attribute;

    AttributeAccess(
        std::shared_ptr<Expression> obj,
        const std::string& attr
    ) : object(obj), attribute(attr) {}

    std::string toString() const override;
};

class SliceAccess : public Expression {
public:
    std::shared_ptr<Expression> object;
    std::shared_ptr<Expression> start;
    std::shared_ptr<Expression> end;

    SliceAccess(std::shared_ptr<Expression> obj, std::shared_ptr<Expression> s, std::shared_ptr<Expression> e)
        : object(obj), start(s), end(e) {}

    std::string toString() const override;
};

class IndexAccess : public Expression {
public:
    std::shared_ptr<Expression> object;
    std::shared_ptr<Expression> index;

    IndexAccess(
        std::shared_ptr<Expression> obj,
        std::shared_ptr<Expression> idx
    ) : object(obj), index(idx) {}

    std::string toString() const override;
};

class ListLiteral : public Expression {
public:
    std::vector<std::shared_ptr<Expression>> elements;

    ListLiteral(
        const std::vector<std::shared_ptr<Expression>>& e
    ) : elements(e) {}

    std::string toString() const override;
};

class DictLiteral : public Expression {
public:
    std::vector<
        std::pair<
            std::shared_ptr<Expression>,
            std::shared_ptr<Expression>
        >
    > entries;

    DictLiteral(
        const std::vector<
            std::pair<
                std::shared_ptr<Expression>,
                std::shared_ptr<Expression>
            >
        >& e
    ) : entries(e) {}

    std::string toString() const override;
};

class IndexAssignment : public ASTNode {
public:
    std::shared_ptr<Expression> object;
    std::shared_ptr<Expression> index;
    std::shared_ptr<Expression> expr;
    std::string op;

    IndexAssignment(std::shared_ptr<Expression> obj, std::shared_ptr<Expression> idx, std::shared_ptr<Expression> e, const std::string& o = "=")
        : object(obj), index(idx), expr(e), op(o) {}

    std::string toString() const override;
};

class Assignment : public ASTNode {
public:
    std::string var;
    std::shared_ptr<Expression> expr;

    Assignment(
        const std::string& v,
        std::shared_ptr<Expression> e
    ) : var(v), expr(e) {}

    std::string toString() const override;
};

class PrintStatement : public ASTNode {
public:
    std::shared_ptr<Expression> expr;

    PrintStatement(std::shared_ptr<Expression> e)
        : expr(e) {}

    std::string toString() const override;
};

class ReturnStatement : public ASTNode {
public:
    std::shared_ptr<Expression> expr;

    ReturnStatement(std::shared_ptr<Expression> e)
        : expr(e) {}

    std::string toString() const override;
};

class FunctionDef : public ASTNode {
public:
    std::string name;
    std::vector<std::string> parameters;
    std::vector<std::shared_ptr<Expression>> defaultArgs;
    std::string variadicParameter;
    std::vector<std::shared_ptr<ASTNode>> body;

    FunctionDef(
        const std::string& n,
        const std::vector<std::string>& p,
        const std::vector<std::shared_ptr<Expression>>& defs,
        const std::vector<std::shared_ptr<ASTNode>>& b,
        const std::string& variadic = ""
    ) : name(n),
        parameters(p),
        defaultArgs(defs),
        variadicParameter(variadic),
        body(b) {}

    std::string toString() const override;
};

class ClassDef : public ASTNode {
public:
    std::string name;
    std::string baseClass;
    std::vector<std::shared_ptr<ASTNode>> body;

    ClassDef(
        const std::string& n,
        const std::string& base,
        const std::vector<std::shared_ptr<ASTNode>>& b
    ) : name(n),
        baseClass(base),
        body(b) {}

    std::string toString() const override;
};

class ForStatement : public ASTNode {
public:
    std::string var;
    std::shared_ptr<Expression> iterable;
    std::vector<std::shared_ptr<ASTNode>> body;

    ForStatement(
        const std::string& v,
        std::shared_ptr<Expression> it,
        const std::vector<std::shared_ptr<ASTNode>>& b
    ) : var(v),
        iterable(it),
        body(b) {}

    std::string toString() const override;
};

class BreakStatement : public ASTNode {
public:
    std::string toString() const override;
};

class ContinueStatement : public ASTNode {
public:
    std::string toString() const override;
};

class ThrowStatement : public ASTNode {
public:
    std::shared_ptr<Expression> expr;

    ThrowStatement(std::shared_ptr<Expression> e)
        : expr(e) {}

    std::string toString() const override;
};

class TryStatement : public ASTNode {
public:
    std::vector<std::shared_ptr<ASTNode>> tryBody;
    std::string exceptionName;
    std::vector<std::shared_ptr<ASTNode>> catchBody;

    TryStatement(
        const std::vector<std::shared_ptr<ASTNode>>& t,
        const std::string& ex,
        const std::vector<std::shared_ptr<ASTNode>>& c
    ) : tryBody(t),
        exceptionName(ex),
        catchBody(c) {}

    std::string toString() const override;
};

class IfStatement : public ASTNode {
public:
    std::shared_ptr<Expression> condition;
    std::vector<std::shared_ptr<ASTNode>> body;
    std::vector<std::shared_ptr<ASTNode>> elseBody;

    IfStatement(
        std::shared_ptr<Expression> cond,
        const std::vector<std::shared_ptr<ASTNode>>& b,
        const std::vector<std::shared_ptr<ASTNode>>& e
    ) : condition(cond),
        body(b),
        elseBody(e) {}

    std::string toString() const override;
};

class WhileStatement : public ASTNode {
public:
    std::shared_ptr<Expression> condition;
    std::vector<std::shared_ptr<ASTNode>> body;

    WhileStatement(
        std::shared_ptr<Expression> cond,
        const std::vector<std::shared_ptr<ASTNode>>& b
    ) : condition(cond),
        body(b) {}

    std::string toString() const override;
};

class Program : public ASTNode {
public:
    std::vector<std::shared_ptr<ASTNode>> statements;

    std::string toString() const override;
};

class Parser {
private:
    std::vector<Token> tokens;
    size_t pos;

    Token current() const;
    Token peek(int offset = 1) const;
    void advance();

    bool match(TokenType type);
    bool check(TokenType type) const;
    bool checkKeyword(const std::string& name) const;

    void consumeNewlines();

    std::shared_ptr<Expression> parsePrimary();
    std::shared_ptr<Expression> parseUnary();
    std::shared_ptr<Expression> parseMultiplicative();
    std::shared_ptr<Expression> parseAdditive();
    std::shared_ptr<Expression> parseComparison();
    std::shared_ptr<Expression> parseExpression();
    std::shared_ptr<Expression> parseLogical();
    std::shared_ptr<Expression> parsePostfix();

    std::shared_ptr<ASTNode> parseStatement();
    std::vector<std::shared_ptr<ASTNode>> parseBlock();

    std::shared_ptr<ASTNode> parseIfStatement();
    std::shared_ptr<ASTNode> parseWhileStatement();
    std::shared_ptr<ASTNode> parsePrintStatement();
    std::shared_ptr<ASTNode> parseFunctionDef();
    std::shared_ptr<ASTNode> parseClassDef();
    std::shared_ptr<ASTNode> parseReturnStatement();
    std::shared_ptr<ASTNode> parseAssignment();
    std::shared_ptr<ASTNode> parseDeclaration();
    std::shared_ptr<ASTNode> parseForStatement();

    std::shared_ptr<ASTNode> parseBreakStatement();
    std::shared_ptr<ASTNode> parseContinueStatement();
    std::shared_ptr<ASTNode> parseThrowStatement();

    std::shared_ptr<ASTNode> parseTryStatement();
    std::shared_ptr<ASTNode> parseImportStatement();
    std::shared_ptr<ASTNode> parseCallStatement();

    std::shared_ptr<Expression> parseListLiteral();
    std::shared_ptr<Expression> parseDictLiteral();

public:
    Parser(const std::vector<Token>& t);

    std::shared_ptr<Program> parse();
};

class Interpreter {
private:
    std::map<std::string, RuntimeValue> variables;
    std::map<std::string, std::shared_ptr<RuntimeObject>> modules;

    std::filesystem::path currentDirectory;
    std::filesystem::path projectDirectory;

    RuntimeValue evaluate(
        const std::shared_ptr<Expression>& expr,
        std::map<std::string, RuntimeValue>* scope = nullptr
    );

    std::optional<RuntimeValue> executeStatement(
        const std::shared_ptr<ASTNode>& stmt,
        std::map<std::string, RuntimeValue>& scope
    );

    std::optional<RuntimeValue> executeBlock(
        const std::vector<std::shared_ptr<ASTNode>>& block,
        std::map<std::string, RuntimeValue>& scope
    );

    RuntimeValue callFunction(
        const std::shared_ptr<RuntimeFunction>& function,
        const std::vector<RuntimeValue>& args
    );

    std::shared_ptr<RuntimeObject> loadModule(
        const std::string& moduleName
    );

public:
    void execute(
        const std::shared_ptr<Program>& program,
        const std::filesystem::path& sourceDirectory = "."
    );

    void printVariables() const;
};







