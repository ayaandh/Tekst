#include "parser.h"
#include <iostream>
#include <stdexcept>
#include <sstream>

std::string NumberLiteral::toString() const { return std::to_string(value); }
std::string BooleanLiteral::toString() const { return value ? "True" : "False"; }
std::string StringLiteral::toString() const { return "\"" + value + "\""; }
std::string Identifier::toString() const { return name; }
std::string BinaryOp::toString() const { return "(" + left->toString() + " " + op + " " + right->toString() + ")"; }
std::string CallExpression::toString() const {
    std::string s = callee + "(";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) s += ", ";
        s += args[i]->toString();
    }
    s += ")";
    return s;
}
std::string AttributeAccess::toString() const { return object->toString() + "." + attribute; }
std::string IndexAccess::toString() const { return object->toString() + "[" + index->toString() + "]"; }
std::string Assignment::toString() const { return var + " = " + expr->toString(); }
std::string ListLiteral::toString() const {
    std::string s = "[";
    for (size_t i = 0; i < elements.size(); ++i) {
        if (i > 0) s += ", ";
        s += elements[i]->toString();
    }
    return s + "]";
}
std::string DictLiteral::toString() const {
    std::string s = "{";
    for (size_t i = 0; i < entries.size(); ++i) {
        if (i > 0) s += ", ";
        s += entries[i].first->toString() + ": " + entries[i].second->toString();
    }
    return s + "}";
}
std::string PrintStatement::toString() const { return "print(" + expr->toString() + ")"; }
std::string ReturnStatement::toString() const { return expr ? ("return " + expr->toString()) : "return"; }
std::string FunctionDef::toString() const {
    std::string s = "def " + name + "(";
    for (size_t i = 0; i < parameters.size(); ++i) { if (i > 0) s += ", "; s += parameters[i]; }
    s += "):\n";
    for (const auto& stmt : body) s += "  " + stmt->toString() + "\n";
    return s;
}
std::string ClassDef::toString() const {
    std::string s = "class " + name;
    if (!baseClass.empty()) s += " extends " + baseClass;
    s += ":\n";
    for (const auto& stmt : body) s += "  " + stmt->toString() + "\n";
    return s;
}
std::string ForStatement::toString() const {
    std::string s = "for " + var + " in " + iterable->toString() + ":\n";
    for (const auto& stmt : body) s += "  " + stmt->toString() + "\n";
    return s;
}
std::string TryStatement::toString() const {
    std::string s = "try:\n";
    for (const auto& stmt : tryBody) s += "  " + stmt->toString() + "\n";
    s += "catch " + exceptionName + ":\n";
    for (const auto& stmt : catchBody) s += "  " + stmt->toString() + "\n";
    return s;
}
std::string IfStatement::toString() const {
    std::string s = "if " + condition->toString() + " :\n";
    for (const auto& stmt : body) s += "  " + stmt->toString() + "\n";
    if (!elseBody.empty()) {
        s += "else:\n";
        for (const auto& stmt : elseBody) s += "  " + stmt->toString() + "\n";
    }
    return s;
}
std::string WhileStatement::toString() const {
    std::string s = "while " + condition->toString() + ":\n";
    for (const auto& stmt : body) s += "  " + stmt->toString() + "\n";
    return s;
}
std::string Program::toString() const {
    std::string result = "Program:\n";
    for (const auto& stmt : statements) result += "  " + stmt->toString() + "\n";
    return result;
}

static std::string valueToString(const RuntimeValue& value) {
    if (const auto* v = std::get_if<int>(&value)) return std::to_string(*v);
    if (const auto* v = std::get_if<bool>(&value)) return *v ? "True" : "False";
    if (const auto* v = std::get_if<std::string>(&value)) return *v;
    if (const auto* v = std::get_if<double>(&value)) return std::to_string(*v);
    if (const auto* v = std::get_if<std::shared_ptr<RuntimeFunction>>(&value)) {
        return "<function " + (*v)->name + ">";
    }
    if (const auto* v = std::get_if<std::shared_ptr<RuntimeObject>>(&value)) {
        if ((*v)->className == "__list__") {
            std::string s = "[";
            bool first = true;
            for (size_t i = 0; i < 1000; ++i) {
                auto key = std::to_string(i);
                auto it = (*v)->fields.find(key);
                if (it == (*v)->fields.end()) break;
                if (!first) s += ", ";
                s += valueToString(it->second);
                first = false;
            }
            s += "]";
            return s;
        }
        if ((*v)->className == "__dict__") {
            std::string s = "{";
            bool first = true;
            for (const auto& [k, val] : (*v)->fields) {
                if (k != "__size__" && k != "__type__") {
                    if (!first) s += ", ";
                    s += k + ": " + valueToString(val);
                    first = false;
                }
            }
            s += "}";
            return s;
        }
        return "<" + (*v)->className + " object>";
    }
    return "<unknown>";
}

static int asInt(const RuntimeValue& value) {
    if (const auto* v = std::get_if<int>(&value)) return *v;
    if (const auto* v = std::get_if<bool>(&value)) return *v ? 1 : 0;
    throw std::runtime_error("Expected numeric value");
}

static std::vector<RuntimeValue> asList(const RuntimeValue& value) {
    if (const auto* v = std::get_if<std::shared_ptr<RuntimeObject>>(&value)) {
        if ((*v)->className == "__list__") {
            std::vector<RuntimeValue> result;
            for (size_t i = 0; i < 1000; ++i) {
                auto key = std::to_string(i);
                auto it = (*v)->fields.find(key);
                if (it == (*v)->fields.end()) break;
                result.push_back(it->second);
            }
            return result;
        }
    }
    throw std::runtime_error("Expected list value");
}

static std::map<std::string, RuntimeValue> asDict(const RuntimeValue& value) {
    if (const auto* v = std::get_if<std::shared_ptr<RuntimeObject>>(&value)) {
        if ((*v)->className == "__dict__") {
            std::map<std::string, RuntimeValue> result;
            for (const auto& [k, val] : (*v)->fields) {
                if (k != "__size__" && k != "__type__") {
                    result[k] = val;
                }
            }
            return result;
        }
    }
    throw std::runtime_error("Expected dictionary value");
}

static bool asBool(const RuntimeValue& value) {
    if (const auto* v = std::get_if<int>(&value)) return *v != 0;
    if (const auto* v = std::get_if<bool>(&value)) return *v;
    if (const auto* v = std::get_if<std::string>(&value)) return !v->empty();
    return true;
}

Parser::Parser(const std::vector<Token>& t) : tokens(t), pos(0) {}

Token Parser::current() const {
    if (pos < tokens.size()) return tokens[pos];
    return Token{TokenType::EOF_TOKEN, ""};
}

Token Parser::peek(int offset) const {
    if (pos + offset < tokens.size()) return tokens[pos + offset];
    return Token{TokenType::EOF_TOKEN, ""};
}

void Parser::advance() { if (pos < tokens.size()) pos++; }

bool Parser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

bool Parser::check(TokenType type) const { return current().type == type; }

bool Parser::checkKeyword(const std::string& name) const {
    return current().type == TokenType::KEYWORD && current().value == name;
}

void Parser::consumeNewlines() {
    while (check(TokenType::NEWLINE)) advance();
}

std::shared_ptr<Program> Parser::parse() {
    auto program = std::make_shared<Program>();
    consumeNewlines();
    while (!check(TokenType::EOF_TOKEN)) {
        if (check(TokenType::NEWLINE)) { advance(); continue; }
        auto stmt = parseStatement();
        if (stmt) program->statements.push_back(stmt);
        consumeNewlines();
    }
    return program;
}

std::shared_ptr<ASTNode> Parser::parseStatement() {
    if (checkKeyword("print")) return parsePrintStatement();
    if (checkKeyword("if")) return parseIfStatement();
    if (checkKeyword("while")) return parseWhileStatement();
    if (checkKeyword("for")) return parseForStatement();
    if (checkKeyword("try")) return parseTryStatement();
    if (checkKeyword("import") || checkKeyword("from")) return parseImportStatement();
    if (checkKeyword("def") || checkKeyword("fn")) return parseFunctionDef();
    if (checkKeyword("class")) return parseClassDef();
    if (checkKeyword("return")) return parseReturnStatement();
    if (checkKeyword("let")) return parseDeclaration();

    if (!check(TokenType::NAME)) return parseAssignment();

    if (peek().type == TokenType::LPAREN) return parseCallStatement();
    if (peek().type == TokenType::DOT && peek(2).type == TokenType::NAME && peek(3).type == TokenType::LPAREN) return parseCallStatement();
    if (peek().type == TokenType::NAME && peek(2).type != TokenType::ASSIGN) return parseDeclaration();
    if (peek().type == TokenType::ASSIGN) return parseAssignment();
    if (peek().type == TokenType::DOT) return parseAssignment();
    return parseAssignment();
}

std::vector<std::shared_ptr<ASTNode>> Parser::parseBlock() {
    std::vector<std::shared_ptr<ASTNode>> block;
    consumeNewlines();
    if (match(TokenType::INDENT)) {
        while (!check(TokenType::EOF_TOKEN) && !check(TokenType::DEDENT)) {
            if (check(TokenType::NEWLINE)) { advance(); continue; }
            block.push_back(parseStatement());
            consumeNewlines();
        }
        if (check(TokenType::DEDENT)) {
            advance();
        }
        return block;
    }

    while (!check(TokenType::EOF_TOKEN) && !checkKeyword("else") && !check(TokenType::DEDENT)) {
        if (check(TokenType::NEWLINE)) { advance(); continue; }
        block.push_back(parseStatement());
        consumeNewlines();
    }
    return block;
}

std::shared_ptr<ASTNode> Parser::parseIfStatement() {
    advance();
    auto cond = parseExpression();
    if (!match(TokenType::COLON)) throw std::runtime_error("Expected ':' after if condition");
    auto body = parseBlock();
    std::vector<std::shared_ptr<ASTNode>> elseBody;
    while (checkKeyword("elif")) {
        advance();
        auto elifCond = parseExpression();
        if (!match(TokenType::COLON)) throw std::runtime_error("Expected ':' after elif condition");
        auto elifBody = parseBlock();
        auto elifStmt = std::make_shared<IfStatement>(elifCond, elifBody, std::vector<std::shared_ptr<ASTNode>>{});
        elseBody.insert(elseBody.end(), elifStmt);
    }
    if (checkKeyword("else")) {
        advance();
        if (!match(TokenType::COLON)) throw std::runtime_error("Expected ':' after else");
        elseBody = parseBlock();
    }
    return std::make_shared<IfStatement>(cond, body, elseBody);
}

std::shared_ptr<ASTNode> Parser::parseWhileStatement() {
    advance();
    auto cond = parseExpression();
    if (!match(TokenType::COLON)) throw std::runtime_error("Expected ':' after while condition");
    auto body = parseBlock();
    return std::make_shared<WhileStatement>(cond, body);
}

std::shared_ptr<ASTNode> Parser::parsePrintStatement() {
    advance();
    if (!match(TokenType::LPAREN)) throw std::runtime_error("Expected '(' after print");
    auto expr = parseExpression();
    if (!match(TokenType::RPAREN)) throw std::runtime_error("Expected ')' after print expression");
    return std::make_shared<PrintStatement>(expr);
}

std::shared_ptr<ASTNode> Parser::parseFunctionDef() {
    advance();
    if (!check(TokenType::NAME)) throw std::runtime_error("Expected function name");
    std::string name = current().value;
    advance();
    if (!match(TokenType::LPAREN)) throw std::runtime_error("Expected '(' after function name");
    std::vector<std::string> params;
    std::vector<std::shared_ptr<Expression>> defaultArgs;
    if (!check(TokenType::RPAREN)) {
        do {
            if (!check(TokenType::NAME)) throw std::runtime_error("Expected parameter name");
            std::string paramName = current().value;
            advance();
            params.push_back(paramName);
            if (match(TokenType::ASSIGN)) {
                defaultArgs.push_back(parseExpression());
            }
        } while (match(TokenType::COMMA));
    }
    if (!match(TokenType::RPAREN)) throw std::runtime_error("Expected ')' after parameter list");
    if (!match(TokenType::COLON)) throw std::runtime_error("Expected ':' after function signature");
    auto body = parseBlock();
    return std::make_shared<FunctionDef>(name, params, defaultArgs, body);
}

std::shared_ptr<ASTNode> Parser::parseClassDef() {
    advance();
    if (!check(TokenType::NAME)) throw std::runtime_error("Expected class name");
    std::string name = current().value;
    advance();
    std::string baseClass = "";
    if (checkKeyword("extends")) {
        advance();
        if (!check(TokenType::NAME)) throw std::runtime_error("Expected base class name after extends");
        baseClass = current().value;
        advance();
    }
    if (!match(TokenType::COLON)) throw std::runtime_error("Expected ':' after class name");
    auto body = parseBlock();
    return std::make_shared<ClassDef>(name, baseClass, body);
}

std::shared_ptr<ASTNode> Parser::parseReturnStatement() {
    advance();
    if (check(TokenType::NEWLINE) || check(TokenType::EOF_TOKEN)) {
        return std::make_shared<ReturnStatement>(nullptr);
    }
    return std::make_shared<ReturnStatement>(parseExpression());
}

std::shared_ptr<ASTNode> Parser::parseAssignment() {
    std::string varName = current().value;
    advance();
    if (match(TokenType::DOT)) {
        if (!check(TokenType::NAME)) throw std::runtime_error("Expected attribute name after '.'");
        varName += "." + current().value;
        advance();
    }
    if (!match(TokenType::ASSIGN)) throw std::runtime_error("Expected '=' after variable name");
    auto expr = parseExpression();
    return std::make_shared<Assignment>(varName, expr);
}

std::shared_ptr<ASTNode> Parser::parseDeclaration() {
    if (checkKeyword("let")) advance();
    std::string typeName = current().value;
    advance();
    if (!check(TokenType::NAME)) {
        throw std::runtime_error("Expected variable name after type name");
    }

    std::string varName = current().value;
    advance();
    
    // Try to call the type as a constructor with possible arguments
    std::vector<std::shared_ptr<Expression>> args;
    if (match(TokenType::LPAREN)) {
        if (!check(TokenType::RPAREN)) {
            do {
                args.push_back(parseExpression());
            } while (match(TokenType::COMMA));
        }
        if (!match(TokenType::RPAREN)) throw std::runtime_error("Expected ')' after constructor arguments");
    }
    
    auto callExpr = std::make_shared<CallExpression>(typeName, args);
    
    std::shared_ptr<Expression> init = callExpr;
    if (match(TokenType::ASSIGN)) {
        init = parseExpression();
    }
    return std::make_shared<Assignment>(varName, init);
}

std::shared_ptr<ASTNode> Parser::parseForStatement() {
    advance();
    if (!check(TokenType::NAME)) throw std::runtime_error("Expected loop variable name");
    std::string varName = current().value;
    advance();
    if (!checkKeyword("in")) throw std::runtime_error("Expected 'in' in for loop");
    advance();
    auto iterable = parseExpression();
    if (!match(TokenType::COLON)) throw std::runtime_error("Expected ':' after for loop header");
    auto body = parseBlock();
    return std::make_shared<ForStatement>(varName, iterable, body);
}

std::shared_ptr<ASTNode> Parser::parseTryStatement() {
    advance();
    if (!match(TokenType::COLON)) throw std::runtime_error("Expected ':' after try");
    auto tryBody = parseBlock();
    if (!checkKeyword("catch")) throw std::runtime_error("Expected 'catch' in try statement");
    advance();
    if (!check(TokenType::NAME)) throw std::runtime_error("Expected exception name");
    std::string excName = current().value;
    advance();
    if (!match(TokenType::COLON)) throw std::runtime_error("Expected ':' after catch name");
    auto catchBody = parseBlock();
    return std::make_shared<TryStatement>(tryBody, excName, catchBody);
}

std::shared_ptr<ASTNode> Parser::parseImportStatement() {
    bool isFrom = checkKeyword("from");
    advance();
    if (isFrom) {
        std::string module = current().value;
        advance();
        if (!checkKeyword("import")) throw std::runtime_error("Expected 'import' after module name");
        advance();
        std::string item = current().value;
        advance();
        return std::make_shared<Assignment>(item, std::make_shared<CallExpression>("import", std::vector<std::shared_ptr<Expression>>{ std::make_shared<StringLiteral>(module) }));
    }
    std::string module = current().value;
    advance();
    return std::make_shared<Assignment>(module, std::make_shared<CallExpression>("import", std::vector<std::shared_ptr<Expression>>{}));
}

std::shared_ptr<Expression> Parser::parseListLiteral() {
    if (!match(TokenType::LBRACKET)) throw std::runtime_error("Expected '[' for list literal");
    std::vector<std::shared_ptr<Expression>> elems;
    if (!check(TokenType::RBRACKET)) {
        do {
            elems.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }
    if (!match(TokenType::RBRACKET)) throw std::runtime_error("Expected ']' after list literal");
    return std::make_shared<ListLiteral>(elems);
}

std::shared_ptr<Expression> Parser::parseDictLiteral() {
    if (!match(TokenType::LBRACE)) throw std::runtime_error("Expected '{' for dict literal");
    std::vector<std::pair<std::shared_ptr<Expression>, std::shared_ptr<Expression>>> entries;
    if (!check(TokenType::RBRACE)) {
        do {
            auto key = parseExpression();
            if (!match(TokenType::COLON)) throw std::runtime_error("Expected ':' in dictionary entry");
            auto value = parseExpression();
            entries.push_back({key, value});
        } while (match(TokenType::COMMA));
    }
    if (!match(TokenType::RBRACE)) throw std::runtime_error("Expected '}' after dict literal");
    return std::make_shared<DictLiteral>(entries);
}

std::shared_ptr<ASTNode> Parser::parseCallStatement() {
    std::string callee = current().value;
    advance();
    if (match(TokenType::DOT)) {
        if (!check(TokenType::NAME)) throw std::runtime_error("Expected method name after '.'");
        callee += "." + current().value;
        advance();
    }
    if (!match(TokenType::LPAREN)) throw std::runtime_error("Expected '(' after function name");
    std::vector<std::shared_ptr<Expression>> args;
    if (!check(TokenType::RPAREN)) {
        do {
            args.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }
    if (!match(TokenType::RPAREN)) throw std::runtime_error("Expected ')' after call arguments");
    return std::make_shared<CallExpression>(callee, args);
}

std::shared_ptr<Expression> Parser::parseExpression() {
    return parseComparison();
}

std::shared_ptr<Expression> Parser::parseComparison() {
    auto left = parseAdditive();
    while (check(TokenType::EQUAL) || check(TokenType::NOT_EQUAL) || check(TokenType::LESS) ||
           check(TokenType::LESS_EQUAL) || check(TokenType::GREATER) || check(TokenType::GREATER_EQUAL)) {
        std::string op = current().value;
        advance();
        auto right = parseAdditive();
        left = std::make_shared<BinaryOp>(left, op, right);
    }
    return left;
}

std::shared_ptr<Expression> Parser::parseAdditive() {
    auto left = parseMultiplicative();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        std::string op = current().value;
        advance();
        auto right = parseMultiplicative();
        left = std::make_shared<BinaryOp>(left, op, right);
    }
    return left;
}

std::shared_ptr<Expression> Parser::parseMultiplicative() {
    auto left = parseUnary();
    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::MOD)) {
        std::string op = current().value;
        advance();
        auto right = parseUnary();
        left = std::make_shared<BinaryOp>(left, op, right);
    }
    return left;
}

std::shared_ptr<Expression> Parser::parseUnary() {
    if (check(TokenType::PLUS)) { advance(); return parseUnary(); }
    if (check(TokenType::MINUS)) { advance(); auto v = parseUnary(); return std::make_shared<BinaryOp>(std::make_shared<NumberLiteral>(0), "-", v); }
    return parsePrimary();
}

std::shared_ptr<Expression> Parser::parsePrimary() {
    if (check(TokenType::LPAREN)) {
        advance();
        auto expr = parseExpression();
        if (!match(TokenType::RPAREN)) throw std::runtime_error("Expected ')' after expression");
        return expr;
    }

    if (check(TokenType::NUMBER)) {
        int value = std::stoi(current().value);
        advance();
        return std::make_shared<NumberLiteral>(value);
    }

    if (check(TokenType::STR)) {
        std::string value = current().value;
        advance();
        return std::make_shared<StringLiteral>(value);
    }

    if (checkKeyword("true") || checkKeyword("false")) {
        bool value = checkKeyword("true");
        advance();
        return std::make_shared<BooleanLiteral>(value);
    }

    if (check(TokenType::LBRACKET)) {
        return parseListLiteral();
    }

    if (check(TokenType::LBRACE)) {
        return parseDictLiteral();
    }

    if (check(TokenType::NAME)) {
        std::string name = current().value;
        advance();

        std::shared_ptr<Expression> expr = std::make_shared<Identifier>(name);

        while (true) {
            if (match(TokenType::DOT)) {
                if (!check(TokenType::NAME)) throw std::runtime_error("Expected attribute name after '.'");
                std::string attribute = current().value;
                advance();
                expr = std::make_shared<AttributeAccess>(expr, attribute);
                continue;
            }
            if (match(TokenType::LBRACKET)) {
                auto indexExpr = parseExpression();
                if (!match(TokenType::RBRACKET)) throw std::runtime_error("Expected ']' after index");
                expr = std::make_shared<IndexAccess>(expr, indexExpr);
                continue;
            }
            if (match(TokenType::LPAREN)) {
                std::vector<std::shared_ptr<Expression>> args;
                if (!check(TokenType::RPAREN)) {
                    do {
                        args.push_back(parseExpression());
                    } while (match(TokenType::COMMA));
                }
                if (!match(TokenType::RPAREN)) throw std::runtime_error("Expected ')' after call arguments");
                expr = std::make_shared<CallExpression>(name, args);
                break;
            }
            break;
        }

        return expr;
    }

    throw std::runtime_error("Unexpected token in expression");
}

RuntimeValue Interpreter::callFunction(const std::shared_ptr<RuntimeFunction>& function, const std::vector<RuntimeValue>& args) {
    if (args.size() != function->parameters.size()) {
        throw std::runtime_error("Function " + function->name + " expected " + std::to_string(function->parameters.size()) + " arguments, got " + std::to_string(args.size()));
    }

    std::map<std::string, RuntimeValue> localScope = function->closure;
    if (function->self) localScope["self"] = function->self;
    for (size_t i = 0; i < function->parameters.size(); ++i) {
        localScope[function->parameters[i]] = args[i];
    }

    std::optional<RuntimeValue> result;
    for (const auto& stmt : function->body) {
        auto value = executeStatement(stmt, localScope);
        if (value.has_value()) {
            result = value; 
            break;
        }
    }

    return result.value_or(0);
}

RuntimeValue Interpreter::evaluate(const std::shared_ptr<Expression>& expr, std::map<std::string, RuntimeValue>* scope) {
    if (auto num = std::dynamic_pointer_cast<NumberLiteral>(expr)) return num->value;
    if (auto b = std::dynamic_pointer_cast<BooleanLiteral>(expr)) return b->value;
    if (auto s = std::dynamic_pointer_cast<StringLiteral>(expr)) return s->value;
    if (auto id = std::dynamic_pointer_cast<Identifier>(expr)) {
        if (scope) {
            auto it = scope->find(id->name);
            if (it != scope->end()) return it->second;
        }
        auto it = variables.find(id->name);
        if (it == variables.end()) throw std::runtime_error("Undefined variable: " + id->name);
        return it->second;
    }
    if (auto attr = std::dynamic_pointer_cast<AttributeAccess>(expr)) {
        auto objectValue = evaluate(attr->object, scope);
        if (const auto* obj = std::get_if<std::shared_ptr<RuntimeObject>>(&objectValue)) {
            auto it = (*obj)->fields.find(attr->attribute);
            if (it == (*obj)->fields.end()) throw std::runtime_error("Undefined attribute: " + attr->attribute);
            return it->second;
        }
        throw std::runtime_error("Attribute access requires an object");
    }
    if (auto index = std::dynamic_pointer_cast<IndexAccess>(expr)) {
        auto container = evaluate(index->object, scope);
        auto idx = evaluate(index->index, scope);
        if (const auto* objPtr = std::get_if<std::shared_ptr<RuntimeObject>>(&container)) {
            if ((*objPtr)->className == "__list__") {
                int i = asInt(idx);
                auto key = std::to_string(i);
                auto it = (*objPtr)->fields.find(key);
                if (it == (*objPtr)->fields.end()) throw std::runtime_error("List index out of range");
                return it->second;
            }
            if ((*objPtr)->className == "__dict__") {
                if (const auto* key = std::get_if<std::string>(&idx)) {
                    auto it = (*objPtr)->fields.find(*key);
                    if (it == (*objPtr)->fields.end()) throw std::runtime_error("Key not found: " + *key);
                    return it->second;
                }
                throw std::runtime_error("Dictionary keys must be strings");
            }
        }
        throw std::runtime_error("Indexing requires a list or dictionary");
    }
    if (auto list = std::dynamic_pointer_cast<ListLiteral>(expr)) {
        auto listObj = std::make_shared<RuntimeObject>();
        listObj->className = "__list__";
        for (size_t i = 0; i < list->elements.size(); ++i) {
            listObj->fields[std::to_string(i)] = evaluate(list->elements[i], scope);
        }
        listObj->fields["__size__"] = static_cast<int>(list->elements.size());
        return listObj;
    }
    if (auto dict = std::dynamic_pointer_cast<DictLiteral>(expr)) {
        auto dictObj = std::make_shared<RuntimeObject>();
        dictObj->className = "__dict__";
        for (const auto& [keyExpr, valueExpr] : dict->entries) {
            auto key = evaluate(keyExpr, scope);
            if (const auto* k = std::get_if<std::string>(&key)) {
                dictObj->fields[*k] = evaluate(valueExpr, scope);
            } else {
                throw std::runtime_error("Dictionary keys must be strings");
            }
        }
        return dictObj;
    }
    if (auto call = std::dynamic_pointer_cast<CallExpression>(expr)) {
        std::vector<RuntimeValue> arguments;
        for (const auto& arg : call->args) arguments.push_back(evaluate(arg, scope));

        if (call->callee == "input") {
            std::string prompt;
            if (!arguments.empty()) {
                prompt = valueToString(arguments[0]);
                if (!prompt.empty()) {
                    std::cout << prompt;
                }
            }
            std::string value;
            std::getline(std::cin, value);
            return value;
        }

        if (call->callee == "int") {
            if (arguments.empty()) throw std::runtime_error("int() requires 1 argument");
            if (const auto* v = std::get_if<int>(&arguments[0])) return *v;
            if (const auto* v = std::get_if<bool>(&arguments[0])) return *v ? 1 : 0;
            if (const auto* v = std::get_if<std::string>(&arguments[0])) return std::stoi(*v);
            throw std::runtime_error("int() expected a number or string");
        }

        if (call->callee == "str") {
            if (arguments.empty()) return std::string{};
            return valueToString(arguments[0]);
        }

        if (call->callee == "bool") {
            if (arguments.empty()) return false;
            return asBool(arguments[0]);
        }

        if (call->callee == "float") {
            if (arguments.empty()) throw std::runtime_error("float() requires 1 argument");
            if (const auto* v = std::get_if<int>(&arguments[0])) return static_cast<double>(*v);
            if (const auto* v = std::get_if<bool>(&arguments[0])) return *v ? 1.0 : 0.0;
            if (const auto* v = std::get_if<double>(&arguments[0])) return *v;
            if (const auto* v = std::get_if<std::string>(&arguments[0])) return std::stod(*v);
            throw std::runtime_error("float() expected a number or string");
        }

        if (call->callee == "len") {
            if (arguments.empty()) throw std::runtime_error("len() requires 1 argument");
            if (const auto* v = std::get_if<std::string>(&arguments[0])) return static_cast<int>(v->size());
            if (const auto* obj = std::get_if<std::shared_ptr<RuntimeObject>>(&arguments[0])) {
                if ((*obj)->className == "__list__") {
                    int size = 0;
                    for (size_t i = 0; i < 1000; ++i) {
                        if ((*obj)->fields.find(std::to_string(i)) == (*obj)->fields.end()) break;
                        ++size;
                    }
                    return size;
                }
                if ((*obj)->className == "__dict__") {
                    int size = 0;
                    for (const auto& [k, val] : (*obj)->fields) {
                        if (k != "__size__" && k != "__type__") ++size;
                    }
                    return size;
                }
            }
            throw std::runtime_error("len() expected a string, list, or dictionary");
        }

        if (call->callee == "import") {
            auto dictObj = std::make_shared<RuntimeObject>();
            dictObj->className = "__dict__";
            if (!arguments.empty()) {
                std::string moduleName = valueToString(arguments[0]);
                if (moduleName == "math") {
                    auto sqrtFn = std::make_shared<RuntimeFunction>();
                    sqrtFn->name = "sqrt";
                    dictObj->fields["sqrt"] = sqrtFn;
                }
            }
            return dictObj;
        }

        if (call->callee.find('.') != std::string::npos) {
            std::string objectName = call->callee.substr(0, call->callee.find('.'));
            std::string methodName = call->callee.substr(call->callee.find('.') + 1);
            auto objIt = scope ? scope->find(objectName) : variables.find(objectName);
            if (objIt == (scope ? scope->end() : variables.end())) {
                auto globalIt = variables.find(objectName);
                if (globalIt == variables.end()) throw std::runtime_error("Undefined object: " + objectName);
                objIt = globalIt;
            }
            if (const auto* obj = std::get_if<std::shared_ptr<RuntimeObject>>(&objIt->second)) {
                auto methodIt = (*obj)->fields.find(methodName);
                if (methodIt == (*obj)->fields.end()) throw std::runtime_error("Undefined method: " + methodName + " on " + objectName);
                if (const auto* fn = std::get_if<std::shared_ptr<RuntimeFunction>>(&methodIt->second)) {
                    auto bound = *fn;
                    bound->self = *obj;
                    return callFunction(bound, arguments);
                }
            }
            throw std::runtime_error("Method call target is not an object: " + objectName);
        }

        auto it = scope ? scope->find(call->callee) : variables.find(call->callee);
        if (it == (scope ? scope->end() : variables.end())) {
            auto globalIt = variables.find(call->callee);
            if (globalIt == variables.end()) throw std::runtime_error("Undefined function or class: " + call->callee);
            it = globalIt;
        }
        if (const auto* fn = std::get_if<std::shared_ptr<RuntimeFunction>>(&it->second)) {
            return callFunction(*fn, arguments);
        }
        if (const auto* obj = std::get_if<std::shared_ptr<RuntimeObject>>(&it->second)) {
            auto instance = std::make_shared<RuntimeObject>();
            instance->className = (*obj)->className;

            auto initIt = (*obj)->fields.find("__init__");
            if (initIt != (*obj)->fields.end()) {
                if (const auto* initFn = std::get_if<std::shared_ptr<RuntimeFunction>>(&initIt->second)) {
                    auto init = *initFn;
                    init->self = instance;
                    std::vector<std::string> paramsWithoutSelf(init->parameters.begin() + 1, init->parameters.end());

                    if (arguments.size() < paramsWithoutSelf.size()) {
                        arguments.resize(paramsWithoutSelf.size(), RuntimeValue{std::string{}});
                    }
                    if (arguments.size() != paramsWithoutSelf.size()) {
                        throw std::runtime_error("Function __init__ expected " + std::to_string(paramsWithoutSelf.size()) + " arguments, got " + std::to_string(arguments.size()));
                    }

                    std::map<std::string, RuntimeValue> localScope = init->closure;
                    localScope["self"] = instance;
                    for (size_t i = 0; i < paramsWithoutSelf.size(); ++i) {
                        localScope[paramsWithoutSelf[i]] = arguments[i];
                    }

                    for (const auto& stmt : init->body) {
                        auto value = executeStatement(stmt, localScope);
                        if (value.has_value()) break;
                    }
                }
            }

            for (const auto& [name, value] : (*obj)->fields) {
                if (name != "__init__") {
                    instance->fields[name] = value;
                }
            }
            return instance;
        }
        throw std::runtime_error("Undefined function or class: " + call->callee);
    }
    if (auto binop = std::dynamic_pointer_cast<BinaryOp>(expr)) {
        RuntimeValue left = evaluate(binop->left, scope);
        RuntimeValue right = evaluate(binop->right, scope);

        if (binop->op == "+") {
            if (const auto* lv = std::get_if<int>(&left); lv && std::get_if<int>(&right)) return *lv + asInt(right);
            if (const auto* lv = std::get_if<std::string>(&left); lv && std::get_if<std::string>(&right)) return *lv + *std::get_if<std::string>(&right);
            if (std::get_if<std::string>(&left) || std::get_if<std::string>(&right)) {
                return valueToString(left) + valueToString(right);
            }
            return asInt(left) + asInt(right);
        }
        if (binop->op == "-") return asInt(left) - asInt(right);
        if (binop->op == "*") return asInt(left) * asInt(right);
        if (binop->op == "/") {
            int r = asInt(right);
            if (r == 0) throw std::runtime_error("Division by zero");
            return asInt(left) / r;
        }
        if (binop->op == "%") return asInt(left) % asInt(right);
        if (binop->op == "==") return asInt(left) == asInt(right);
        if (binop->op == "!=") return asInt(left) != asInt(right);
        if (binop->op == "<") return asInt(left) < asInt(right);
        if (binop->op == "<=") return asInt(left) <= asInt(right);
        if (binop->op == ">") return asInt(left) > asInt(right);
        if (binop->op == ">=") return asInt(left) >= asInt(right);
        throw std::runtime_error("Unknown operator: " + binop->op);
    }

    throw std::runtime_error("Cannot evaluate expression");
}

std::optional<RuntimeValue> Interpreter::executeStatement(const std::shared_ptr<ASTNode>& stmt, std::map<std::string, RuntimeValue>& scope) {
    if (auto assign = std::dynamic_pointer_cast<Assignment>(stmt)) {
        auto value = evaluate(assign->expr, &scope);
        if (assign->var.find('.') != std::string::npos) {
            auto dot = assign->var.find('.');
            std::string objName = assign->var.substr(0, dot);
            std::string attr = assign->var.substr(dot + 1);
            auto it = scope.find(objName);
            if (it == scope.end()) throw std::runtime_error("Undefined object: " + objName);
            auto obj = std::get_if<std::shared_ptr<RuntimeObject>>(&it->second);
            if (!obj) throw std::runtime_error("Attribute assignment requires an object");
            (*obj)->fields[attr] = value;
            return std::nullopt;
        }
        scope[assign->var] = value;
        return std::nullopt;
    }

    if (auto printStmt = std::dynamic_pointer_cast<PrintStatement>(stmt)) {
        std::cout << valueToString(evaluate(printStmt->expr, &scope)) << '\n';
        return std::nullopt;
    }

    if (auto callExpr = std::dynamic_pointer_cast<CallExpression>(stmt)) {
        evaluate(callExpr, &scope);
        return std::nullopt;
    }

    if (auto ret = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
        return ret->expr ? evaluate(ret->expr, &scope) : RuntimeValue{0};
    }

    if (auto func = std::dynamic_pointer_cast<FunctionDef>(stmt)) {
        auto fn = std::make_shared<RuntimeFunction>();
        fn->name = func->name;
        fn->parameters = func->parameters;
        fn->body = func->body;
        fn->closure = scope;
        scope[func->name] = fn;
        return std::nullopt;
    }

    if (auto cls = std::dynamic_pointer_cast<ClassDef>(stmt)) {
        auto classObject = std::make_shared<RuntimeObject>();
        classObject->className = cls->name;
        for (const auto& child : cls->body) {
            if (auto innerFunc = std::dynamic_pointer_cast<FunctionDef>(child)) {
                auto fn = std::make_shared<RuntimeFunction>();
                fn->name = innerFunc->name;
                fn->parameters = innerFunc->parameters;
                fn->body = innerFunc->body;
                fn->closure = scope;
                classObject->fields[innerFunc->name] = fn;
            } else if (auto innerAssign = std::dynamic_pointer_cast<Assignment>(child)) {
                classObject->fields[innerAssign->var] = evaluate(innerAssign->expr);
            }
        }
        scope[cls->name] = classObject;
        return std::nullopt;
    }

    if (auto ifStmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
        RuntimeValue cond = evaluate(ifStmt->condition, &scope);
        if (asBool(cond)) {
            return executeBlock(ifStmt->body, scope);
        }
        if (!ifStmt->elseBody.empty()) {
            return executeBlock(ifStmt->elseBody, scope);
        }
        return std::nullopt;
    }

    if (auto whileStmt = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
        while (asBool(evaluate(whileStmt->condition, &scope))) {
            auto result = executeBlock(whileStmt->body, scope);
            if (result.has_value()) return result;
        }
        return std::nullopt;
    }

    if (auto forStmt = std::dynamic_pointer_cast<ForStatement>(stmt)) {
        auto iterableValue = evaluate(forStmt->iterable, &scope);
        if (const auto* objPtr = std::get_if<std::shared_ptr<RuntimeObject>>(&iterableValue)) {
            if ((*objPtr)->className == "__list__") {
                for (size_t i = 0; i < 1000; ++i) {
                    auto key = std::to_string(i);
                    auto it = (*objPtr)->fields.find(key);
                    if (it == (*objPtr)->fields.end()) break;
                    scope[forStmt->var] = it->second;
                    auto result = executeBlock(forStmt->body, scope);
                    if (result.has_value()) return result;
                }
                return std::nullopt;
            }
        }
        throw std::runtime_error("For loops require a list value");
    }

    if (auto tryStmt = std::dynamic_pointer_cast<TryStatement>(stmt)) {
        try {
            auto result = executeBlock(tryStmt->tryBody, scope);
            if (result.has_value()) return result;
        } catch (const std::exception&) {
            return executeBlock(tryStmt->catchBody, scope);
        }
        return std::nullopt;
    }

    throw std::runtime_error("Unsupported statement type");
}

std::optional<RuntimeValue> Interpreter::executeBlock(const std::vector<std::shared_ptr<ASTNode>>& block, std::map<std::string, RuntimeValue>& scope) {
    for (const auto& stmt : block) {
        auto result = executeStatement(stmt, scope);
        if (result.has_value()) return result;
    }
    return std::nullopt;
}

void Interpreter::execute(const std::shared_ptr<Program>& program) {
    executeBlock(program->statements, variables);
}

void Interpreter::printVariables() const {
    std::cout << "\nVariables:\n";
    for (const auto& pair : variables) std::cout << "  " << pair.first << " = " << valueToString(pair.second) << "\n";
}
