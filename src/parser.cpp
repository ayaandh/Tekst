#include "parser.h"
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <functional>
#include <cctype>
#include <cmath>
#include <random>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <algorithm>

std::string NumberLiteral::toString() const {
    return std::to_string(value);
}

std::string FloatLiteral::toString() const {
    return std::to_string(value);
}

std::string BreakStatement::toString() const {
    return "break";
}

std::string ContinueStatement::toString() const {
    return "continue";
}

std::string BooleanLiteral::toString() const {
    return value ? "True" : "False";
}

std::string StringLiteral::toString() const {
    return "\"" + value + "\"";
}

std::string Identifier::toString() const {
    return name;
}

std::string UnaryOp::toString() const {
    return "(" + op + " " + operand->toString() + ")";
}

std::string BinaryOp::toString() const {
    return "(" + left->toString() + " " + op + " " + right->toString() + ")";
}

std::string CallExpression::toString() const {
    std::string s = callee + "(";

    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) s += ", ";
        s += args[i]->toString();
    }

    s += ")";
    return s;
}

std::string AttributeAccess::toString() const {
    return object->toString() + "." + attribute;
}

std::string IndexAccess::toString() const {
    return object->toString() + "[" + index->toString() + "]";
}

std::string Assignment::toString() const {
    return var + " = " + expr->toString();
}

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

        s += entries[i].first->toString();
        s += ": ";
        s += entries[i].second->toString();
    }

    return s + "}";
}

std::string PrintStatement::toString() const {
    return "print(" + expr->toString() + ")";
}

std::string ReturnStatement::toString() const {
    return expr ? "return " + expr->toString() : "return";
}

std::string FunctionDef::toString() const {
    std::string s = "def " + name + "(";

    for (size_t i = 0; i < parameters.size(); ++i) {
        if (i > 0) s += ", ";
        s += parameters[i];
    }

    if (!variadicParameter.empty()) {
        if (!parameters.empty()) s += ", ";
        s += "*" + variadicParameter;
    }

    s += "):\n";

    for (const auto& stmt : body) {
        s += "  " + stmt->toString() + "\n";
    }

    return s;
}

std::string ClassDef::toString() const {
    std::string s = "class " + name;

    if (!baseClass.empty()) {
        s += " extends " + baseClass;
    }

    s += ":\n";

    for (const auto& stmt : body) {
        s += "  " + stmt->toString() + "\n";
    }

    return s;
}

std::string ForStatement::toString() const {
    std::string s = "for " + var + " in " + iterable->toString() + ":\n";

    for (const auto& stmt : body) {
        s += "  " + stmt->toString() + "\n";
    }

    return s;
}

std::string ThrowStatement::toString() const {
    return "throw " + (expr ? expr->toString() : "");
}

std::string TryStatement::toString() const {
    std::string s = "try:\n";

    for (const auto& stmt : tryBody) {
        s += "  " + stmt->toString() + "\n";
    }

    s += "catch " + exceptionName + ":\n";

    for (const auto& stmt : catchBody) {
        s += "  " + stmt->toString() + "\n";
    }

    return s;
}

std::string IfStatement::toString() const {
    std::string s = "if " + condition->toString() + ":\n";

    for (const auto& stmt : body) {
        s += "  " + stmt->toString() + "\n";
    }

    if (!elseBody.empty()) {
        s += "else:\n";

        for (const auto& stmt : elseBody) {
            s += "  " + stmt->toString() + "\n";
        }
    }

    return s;
}

std::string WhileStatement::toString() const {
    std::string s = "while " + condition->toString() + ":\n";

    for (const auto& stmt : body) {
        s += "  " + stmt->toString() + "\n";
    }

    return s;
}

std::string Program::toString() const {
    std::string result = "Program:\n";

    for (const auto& stmt : statements) {
        result += "  " + stmt->toString() + "\n";
    }

    return result;
}

static std::string valueToString(const RuntimeValue& value) {
    if (const auto* v = std::get_if<int>(&value)) {
        return std::to_string(*v);
    }

    if (const auto* v = std::get_if<bool>(&value)) {
        return *v ? "True" : "False";
    }

    if (const auto* v = std::get_if<std::string>(&value)) {
        return *v;
    }

    if (const auto* v = std::get_if<double>(&value)) {
        std::string s = std::to_string(*v);
        while (!s.empty() && s.back() == '0') s.pop_back();
        if (!s.empty() && s.back() == '.') s.pop_back();
        return s;
    }

    if (const auto* v = std::get_if<std::shared_ptr<RuntimeFunction>>(&value)) {
        return "<function " + (*v)->name + ">";
    }

    if (const auto* v = std::get_if<std::shared_ptr<RuntimeObject>>(&value)) {
        if ((*v)->className == "__list__") {
            std::string s = "[";
            bool first = true;

            for (size_t i = 0; i < 1000; ++i) {
                auto it = (*v)->fields.find(std::to_string(i));

                if (it == (*v)->fields.end()) {
                    break;
                }

                if (!first) {
                    s += ", ";
                }

                s += valueToString(it->second);
                first = false;
            }

            return s + "]";
        }

        if ((*v)->className == "__dict__") {
            std::string s = "{";
            bool first = true;

            for (const auto& [k, val] : (*v)->fields) {
                if (k == "__size__" || k == "__type__") {
                    continue;
                }

                if (!first) {
                    s += ", ";
                }

                s += k + ": " + valueToString(val);
                first = false;
            }

            return s + "}";
        }

        return "<" + (*v)->className + " object>";
    }

    return "<unknown>";
}

static int asInt(const RuntimeValue& value) {
    if (const auto* v = std::get_if<int>(&value)) {
        return *v;
    }

    if (const auto* v = std::get_if<bool>(&value)) {
        return *v ? 1 : 0;
    }

    throw std::runtime_error("Expected numeric value");
}

static bool asBool(const RuntimeValue& value) {
    if (const auto* v = std::get_if<int>(&value)) {
        return *v != 0;
    }

    if (const auto* v = std::get_if<bool>(&value)) {
        return *v;
    }

    if (const auto* v = std::get_if<std::string>(&value)) {
        return !v->empty();
    }

    return true;
}

Parser::Parser(const std::vector<Token>& t)
    : tokens(t), pos(0) {}

Token Parser::current() const {
    if (pos < tokens.size()) {
        return tokens[pos];
    }

    return {TokenType::EOF_TOKEN, ""};
}

Token Parser::peek(int offset) const {
    if (pos + offset < tokens.size()) {
        return tokens[pos + offset];
    }

    return {TokenType::EOF_TOKEN, ""};
}

void Parser::advance() {
    if (pos < tokens.size()) {
        ++pos;
    }
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }

    return false;
}

bool Parser::check(TokenType type) const {
    return current().type == type;
}

bool Parser::checkKeyword(const std::string& name) const {
    return current().type == TokenType::KEYWORD &&
           current().value == name;
}

void Parser::consumeNewlines() {
    while (check(TokenType::NEWLINE)) {
        advance();
    }
}

std::shared_ptr<Program> Parser::parse() {
    auto program = std::make_shared<Program>();

    consumeNewlines();

    while (!check(TokenType::EOF_TOKEN)) {
        if (check(TokenType::NEWLINE)) {
            advance();
            continue;
        }

        auto stmt = parseStatement();

        if (stmt) {
            program->statements.push_back(stmt);
        }

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
    if (checkKeyword("throw")) return parseThrowStatement();
    if (checkKeyword("break")) return parseBreakStatement();
    if (checkKeyword("continue")) return parseContinueStatement();
    if (checkKeyword("import") || checkKeyword("from")) return parseImportStatement();
    if (checkKeyword("def") || checkKeyword("fn")) return parseFunctionDef();
    if (checkKeyword("class")) return parseClassDef();
    if (checkKeyword("return")) return parseReturnStatement();
    if (checkKeyword("let")) return parseDeclaration();

    if (!check(TokenType::NAME)) {
        return parseAssignment();
    }

    if (peek().type == TokenType::LPAREN) {
        return parseCallStatement();
    }

    if (peek().type == TokenType::DOT &&
        peek(2).type == TokenType::NAME &&
        peek(3).type == TokenType::LPAREN) {
        return parseCallStatement();
    }

    if (peek().type == TokenType::NAME &&
        peek(2).type != TokenType::ASSIGN) {
        return parseDeclaration();
    }

    if (peek().type == TokenType::ASSIGN) {
        return parseAssignment();
    }

    if (peek().type == TokenType::DOT) {
        return parseAssignment();
    }

    return parseAssignment();
}

std::vector<std::shared_ptr<ASTNode>> Parser::parseBlock() {
    std::vector<std::shared_ptr<ASTNode>> block;

    consumeNewlines();

    if (match(TokenType::INDENT)) {
        while (!check(TokenType::EOF_TOKEN) &&
               !check(TokenType::DEDENT)) {
            if (check(TokenType::NEWLINE)) {
                advance();
                continue;
            }

            block.push_back(parseStatement());
            consumeNewlines();
        }

        if (check(TokenType::DEDENT)) {
            advance();
        }

        return block;
    }

    while (!check(TokenType::EOF_TOKEN) &&
           !checkKeyword("else") &&
           !check(TokenType::DEDENT)) {
        if (check(TokenType::NEWLINE)) {
            advance();
            continue;
        }

        block.push_back(parseStatement());
        consumeNewlines();
    }

    return block;
}

std::shared_ptr<ASTNode> Parser::parseIfStatement() {
    advance();

    auto cond = parseExpression();

    if (!match(TokenType::COLON)) {
        throw std::runtime_error("Expected ':' after if condition");
    }

    auto body = parseBlock();
    std::vector<std::shared_ptr<ASTNode>> elseBody;

    while (checkKeyword("elif")) {
        advance();

        auto elifCond = parseExpression();

        if (!match(TokenType::COLON)) {
            throw std::runtime_error("Expected ':' after elif condition");
        }

        auto elifBody = parseBlock();

        auto elifStmt = std::make_shared<IfStatement>(
            elifCond,
            elifBody,
            std::vector<std::shared_ptr<ASTNode>>{}
        );

        elseBody.push_back(elifStmt);
    }

    if (checkKeyword("else")) {
        advance();

        if (!match(TokenType::COLON)) {
            throw std::runtime_error("Expected ':' after else");
        }

        elseBody = parseBlock();
    }

    return std::make_shared<IfStatement>(
        cond,
        body,
        elseBody
    );
}

std::shared_ptr<ASTNode> Parser::parseWhileStatement() {
    advance();

    auto cond = parseExpression();

    if (!match(TokenType::COLON)) {
        throw std::runtime_error("Expected ':' after while condition");
    }

    return std::make_shared<WhileStatement>(
        cond,
        parseBlock()
    );
}

std::shared_ptr<ASTNode> Parser::parsePrintStatement() {
    advance();

    if (!match(TokenType::LPAREN)) {
        throw std::runtime_error("Expected '(' after print");
    }

    auto expr = parseExpression();

    if (!match(TokenType::RPAREN)) {
        throw std::runtime_error("Expected ')' after print expression");
    }

    return std::make_shared<PrintStatement>(expr);
}

std::shared_ptr<ASTNode> Parser::parseFunctionDef() {
    advance();

    if (!check(TokenType::NAME)) {
        throw std::runtime_error("Expected function name");
    }

    std::string name = current().value;
    advance();

    if (!match(TokenType::LPAREN)) {
        throw std::runtime_error("Expected '(' after function name");
    }

    std::vector<std::string> params;
    std::vector<std::shared_ptr<Expression>> defaults;
    std::string variadicParameter;

    if (!check(TokenType::RPAREN)) {
        do {
            bool variadic = match(TokenType::STAR);

            if (!check(TokenType::NAME)) {
                throw std::runtime_error(variadic ? "Expected parameter name after '*'" : "Expected parameter name");
            }

            std::string param = current().value;
            advance();

            if (variadic) {
                if (!variadicParameter.empty()) {
                    throw std::runtime_error("Only one variadic parameter is allowed");
                }

                variadicParameter = param;

                if (match(TokenType::COMMA) && !check(TokenType::RPAREN)) {
                    throw std::runtime_error("Variadic parameter must be the last parameter");
                }
                break;
            }

            params.push_back(param);

            if (match(TokenType::ASSIGN)) {
                defaults.push_back(parseExpression());
            } else {
                defaults.push_back(nullptr);
            }

            if (!match(TokenType::COMMA)) break;
        } while (!check(TokenType::RPAREN));
    }

    if (!match(TokenType::RPAREN)) {
        throw std::runtime_error("Expected ')' after parameter list");
    }

    if (!match(TokenType::COLON)) {
        throw std::runtime_error("Expected ':' after function signature");
    }

    return std::make_shared<FunctionDef>(
        name,
        params,
        defaults,
        parseBlock(),
        variadicParameter
    );
}

std::shared_ptr<ASTNode> Parser::parseClassDef() {
    advance();

    if (!check(TokenType::NAME)) {
        throw std::runtime_error("Expected class name");
    }

    std::string name = current().value;
    advance();

    std::string baseClass;

    if (checkKeyword("extends")) {
        advance();

        if (!check(TokenType::NAME)) {
            throw std::runtime_error("Expected base class name");
        }

        baseClass = current().value;
        advance();
    }

    if (!match(TokenType::COLON)) {
        throw std::runtime_error("Expected ':' after class name");
    }

    return std::make_shared<ClassDef>(
        name,
        baseClass,
        parseBlock()
    );
}

std::shared_ptr<ASTNode> Parser::parseReturnStatement() {
    advance();

    if (check(TokenType::NEWLINE) ||
        check(TokenType::EOF_TOKEN)) {
        return std::make_shared<ReturnStatement>(nullptr);
    }

    return std::make_shared<ReturnStatement>(
        parseExpression()
    );
}

std::shared_ptr<ASTNode> Parser::parseAssignment() {
    std::string varName = current().value;
    advance();

    if (match(TokenType::DOT)) {
        if (!check(TokenType::NAME)) {
            throw std::runtime_error("Expected attribute name after '.'");
        }

        varName += "." + current().value;
        advance();
    }

    if (!match(TokenType::ASSIGN)) {
        throw std::runtime_error("Expected '=' after variable name");
    }

    return std::make_shared<Assignment>(
        varName,
        parseExpression()
    );
}



std::shared_ptr<ASTNode> Parser::parseDeclaration() {
    if (checkKeyword("let")) {
        advance();
    }

    std::string typeName = current().value;
    advance();

    if (!check(TokenType::NAME)) {
        throw std::runtime_error("Expected variable name after type name");
    }

    std::string varName = current().value;
    advance();

    std::vector<std::shared_ptr<Expression>> args;

    if (match(TokenType::LPAREN)) {
        if (!check(TokenType::RPAREN)) {
            do {
                args.push_back(parseExpression());
            } while (match(TokenType::COMMA));
        }

        if (!match(TokenType::RPAREN)) {
            throw std::runtime_error("Expected ')' after constructor arguments");
        }
    }

    std::shared_ptr<Expression> init = std::make_shared<CallExpression>(
        typeName,
        args
    );

    if (match(TokenType::ASSIGN)) {
        init = parseExpression();
    }

    return std::make_shared<Assignment>(
        varName,
        init
    );
}

std::shared_ptr<ASTNode> Parser::parseForStatement() {
    advance();

    if (!check(TokenType::NAME)) {
        throw std::runtime_error("Expected loop variable name");
    }

    std::string varName = current().value;
    advance();

    if (!checkKeyword("in")) {
        throw std::runtime_error("Expected 'in' in for loop");
    }

    advance();

    auto iterable = parseExpression();

    if (!match(TokenType::COLON)) {
        throw std::runtime_error("Expected ':' after for loop header");
    }

    return std::make_shared<ForStatement>(
        varName,
        iterable,
        parseBlock()
    );
}

std::shared_ptr<ASTNode> Parser::parseBreakStatement() {
    advance();
    return std::make_shared<BreakStatement>();
}

std::shared_ptr<ASTNode> Parser::parseContinueStatement() {
    advance();
    return std::make_shared<ContinueStatement>();
}

std::shared_ptr<ASTNode> Parser::parseThrowStatement() {
    advance();
    if (check(TokenType::NEWLINE) || check(TokenType::EOF_TOKEN)) {
        throw std::runtime_error("Expected expression after throw");
    }
    return std::make_shared<ThrowStatement>(parseExpression());
}

std::shared_ptr<ASTNode> Parser::parseTryStatement() {
    advance();

    if (!match(TokenType::COLON)) {
        throw std::runtime_error("Expected ':' after try");
    }

    auto tryBody = parseBlock();

    if (!checkKeyword("catch")) {
        throw std::runtime_error("Expected 'catch'");
    }

    advance();

    if (!check(TokenType::NAME)) {
        throw std::runtime_error("Expected exception name");
    }

    std::string exceptionName = current().value;
    advance();

    if (!match(TokenType::COLON)) {
        throw std::runtime_error("Expected ':' after catch");
    }

    return std::make_shared<TryStatement>(
        tryBody,
        exceptionName,
        parseBlock()
    );
}

std::shared_ptr<ASTNode> Parser::parseImportStatement() {
    bool fromImport = checkKeyword("from");
    advance();

    auto parsePath = [&]() -> std::string {
        std::string path;

        if (!check(TokenType::NAME)) {
            throw std::runtime_error("Expected module path");
        }

        path = current().value;
        advance();

        while (check(TokenType::DOT) || check(TokenType::BACKSLASH)) {
            if (check(TokenType::DOT)) {
                advance();
                if (!check(TokenType::NAME)) {
                    throw std::runtime_error("Expected name after '.'");
                }
                path += "." + current().value;
                advance();
            } else {
                advance();
                if (!check(TokenType::NAME)) {
                    throw std::runtime_error("Expected name after '\\'");
                }
                path += "\\" + current().value;
                advance();
            }
        }

        return path;
    };

    std::string module = parsePath();

    if (fromImport) {
        if (!checkKeyword("import")) {
            throw std::runtime_error("Expected 'import' after module path");
        }

        advance();

        if (!check(TokenType::NAME)) {
            throw std::runtime_error("Expected name after import");
        }

        std::string item = current().value;
        advance();

        return std::make_shared<Assignment>(
            item,
            std::make_shared<CallExpression>(
                "__import_from__",
                std::vector<std::shared_ptr<Expression>>{
                    std::make_shared<StringLiteral>(module),
                    std::make_shared<StringLiteral>(item)
                }
            )
        );
    }

    std::string normalized = module;
    for (char& c : normalized) {
        if (c == '\\') c = '/';
    }

    std::filesystem::path path(normalized);
    std::string binding = path.stem().string();
    if (binding.empty()) binding = path.filename().string();
    if (binding.empty()) binding = module;

    return std::make_shared<Assignment>(
        binding,
        std::make_shared<CallExpression>(
            "__import__",
            std::vector<std::shared_ptr<Expression>>{
                std::make_shared<StringLiteral>(module)
            }
        )
    );
}

std::shared_ptr<Expression> Parser::parseListLiteral() {
    if (!match(TokenType::LBRACKET)) {
        throw std::runtime_error("Expected '['");
    }

    std::vector<std::shared_ptr<Expression>> elems;

    if (!check(TokenType::RBRACKET)) {
        do {
            elems.push_back(parseExpression());
            if (!match(TokenType::COMMA)) break;
        } while (!check(TokenType::RBRACKET));
    }

    if (!match(TokenType::RBRACKET)) {
        throw std::runtime_error("Expected ']'");
    }

    return std::make_shared<ListLiteral>(elems);
}

std::shared_ptr<Expression> Parser::parseDictLiteral() {
    if (!match(TokenType::LBRACE)) {
        throw std::runtime_error("Expected '{'");
    }

    std::vector<
        std::pair<
            std::shared_ptr<Expression>,
            std::shared_ptr<Expression>
        >
    > entries;

    if (!check(TokenType::RBRACE)) {
        do {
            auto key = parseExpression();

            if (!match(TokenType::COLON)) {
                throw std::runtime_error("Expected ':' in dictionary");
            }

            auto value = parseExpression();
            entries.push_back({key, value});

            if (!match(TokenType::COMMA)) break;
        } while (!check(TokenType::RBRACE));
    }

    if (!match(TokenType::RBRACE)) {
        throw std::runtime_error("Expected '}'");
    }

    return std::make_shared<DictLiteral>(entries);
}

std::shared_ptr<ASTNode> Parser::parseCallStatement() {
    std::string callee = current().value;
    advance();

    if (match(TokenType::DOT)) {
        if (!check(TokenType::NAME)) {
            throw std::runtime_error("Expected method name after '.'");
        }

        callee += "." + current().value;
        advance();
    }

    if (!match(TokenType::LPAREN)) {
        throw std::runtime_error("Expected '(' after function name");
    }

    std::vector<std::shared_ptr<Expression>> args;

    if (!check(TokenType::RPAREN)) {
        do {
            args.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }

    if (!match(TokenType::RPAREN)) {
        throw std::runtime_error("Expected ')' after arguments");
    }

    return std::make_shared<CallExpression>(
        callee,
        args
    );
}

std::shared_ptr<Expression> Parser::parseExpression() {
    auto left = parseComparison();
    while (checkKeyword("and") || checkKeyword("or")) {
        std::string op = current().value;
        advance();
        auto right = parseComparison();
        left = std::make_shared<BinaryOp>(left, op, right);
    }
    return left;
}

std::shared_ptr<Expression> Parser::parseComparison() {
    auto left = parseAdditive();

    while (
        check(TokenType::EQUAL) ||
        check(TokenType::NOT_EQUAL) ||
        check(TokenType::LESS) ||
        check(TokenType::LESS_EQUAL) ||
        check(TokenType::GREATER) ||
        check(TokenType::GREATER_EQUAL)
    ) {
        std::string op = current().value;
        advance();

        auto right = parseAdditive();

        left = std::make_shared<BinaryOp>(
            left,
            op,
            right
        );
    }

    return left;
}

std::shared_ptr<Expression> Parser::parseAdditive() {
    auto left = parseMultiplicative();

    while (
        check(TokenType::PLUS) ||
        check(TokenType::MINUS)
    ) {
        std::string op = current().value;
        advance();

        auto right = parseMultiplicative();

        left = std::make_shared<BinaryOp>(
            left,
            op,
            right
        );
    }

    return left;
}

std::shared_ptr<Expression> Parser::parseMultiplicative() {
    auto left = parseUnary();

    while (
        check(TokenType::STAR) ||
        check(TokenType::SLASH) ||
        check(TokenType::MOD)
    ) {
        std::string op = current().value;
        advance();

        auto right = parseUnary();

        left = std::make_shared<BinaryOp>(
            left,
            op,
            right
        );
    }

    return left;
}

std::shared_ptr<Expression> Parser::parseUnary() {
    if (checkKeyword("not")) {
        advance();
        return std::make_shared<UnaryOp>("not", parseUnary());
    }
    if (check(TokenType::PLUS)) {
        advance();
        return std::make_shared<UnaryOp>("+", parseUnary());
    }
    if (check(TokenType::MINUS)) {
        advance();
        return std::make_shared<UnaryOp>("-", parseUnary());
    }
    return parsePrimary();
}

std::shared_ptr<Expression> Parser::parsePrimary() {
    std::shared_ptr<Expression> expr;

    if (check(TokenType::LPAREN)) {
        advance();
        expr = parseExpression();
        if (!match(TokenType::RPAREN)) throw std::runtime_error("Expected ')' after expression");
    } else if (check(TokenType::NUMBER)) {
        std::string text = current().value;
        advance();
        if (text.find('.') != std::string::npos) expr = std::make_shared<FloatLiteral>(std::stod(text));
        else expr = std::make_shared<NumberLiteral>(std::stoi(text));
    } else if (check(TokenType::STR)) {
        std::string value = current().value;
        advance();
        expr = std::make_shared<StringLiteral>(value);
    } else if (checkKeyword("true") || checkKeyword("false")) {
        bool value = checkKeyword("true");
        advance();
        expr = std::make_shared<BooleanLiteral>(value);
    } else if (check(TokenType::LBRACKET)) {
        expr = parseListLiteral();
    } else if (check(TokenType::LBRACE)) {
        expr = parseDictLiteral();
    } else if (check(TokenType::NAME)) {
        std::string name = current().value;
        advance();
        expr = std::make_shared<Identifier>(name);
    } else {
        throw std::runtime_error("Unexpected token in expression: '" + current().value + "'");
    }

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
                    if (!match(TokenType::COMMA)) break;
                } while (!check(TokenType::RPAREN));
            }
            if (!match(TokenType::RPAREN)) throw std::runtime_error("Expected ')' after arguments");
            std::function<std::string(const std::shared_ptr<Expression>&)> buildName =
                [&](const std::shared_ptr<Expression>& e) -> std::string {
                    if (auto id = std::dynamic_pointer_cast<Identifier>(e)) return id->name;
                    if (auto attr = std::dynamic_pointer_cast<AttributeAccess>(e)) return buildName(attr->object) + "." + attr->attribute;
                    throw std::runtime_error("Invalid call target");
                };
            expr = std::make_shared<CallExpression>(buildName(expr), args);
            continue;
        }
        break;
    }
    return expr;
}

RuntimeValue Interpreter::callFunction(
    const std::shared_ptr<RuntimeFunction>& function,
    const std::vector<RuntimeValue>& args
) {
    size_t offset = (function->self && !function->parameters.empty() && function->parameters[0] == "self") ? 1 : 0;
    size_t parameterCount = function->parameters.size() - offset;

    if (function->variadicParameter.empty() && args.size() > parameterCount) {
        throw std::runtime_error("Function " + function->name + " expected at most " + std::to_string(parameterCount) + " arguments, got " + std::to_string(args.size()));
    }

    if (function->variadicParameter.empty() && args.size() < parameterCount) {
        for (size_t i = offset; i < function->parameters.size(); ++i) {
            size_t argIndex = i - offset;
            if (argIndex >= args.size() &&
                !(i < function->defaultArgs.size() && function->defaultArgs[i])) {
                throw std::runtime_error("Function " + function->name + " missing argument: " + function->parameters[i]);
            }
        }
    }

    std::map<std::string, RuntimeValue> localScope = function->closure;
    if (function->self) localScope["self"] = function->self;

    for (size_t i = offset; i < function->parameters.size(); ++i) {
        size_t argIndex = i - offset;
        if (argIndex < args.size()) {
            localScope[function->parameters[i]] = args[argIndex];
        } else if (i < function->defaultArgs.size() && function->defaultArgs[i]) {
            localScope[function->parameters[i]] = evaluate(function->defaultArgs[i], &localScope);
        } else {
            throw std::runtime_error("Function " + function->name + " missing argument: " + function->parameters[i]);
        }
    }

    if (!function->variadicParameter.empty()) {
        auto packed = std::make_shared<RuntimeObject>();
        packed->className = "__list__";

        size_t start = parameterCount;
        int size = 0;
        for (size_t i = start; i < args.size(); ++i) {
            packed->fields[std::to_string(size++)] = args[i];
        }
        packed->fields["__size__"] = size;
        localScope[function->variadicParameter] = packed;
    }

    auto result = executeBlock(function->body, localScope);
    return result.value_or(RuntimeValue{0});
}

std::shared_ptr<RuntimeObject> Interpreter::loadModule(
    const std::string& moduleName
) {
    if (moduleName == "math" || moduleName == "random" || moduleName == "time" || moduleName == "os" || moduleName == "fs") {
        auto cached = modules.find(moduleName);
        if (cached != modules.end()) return cached->second;

        auto m = std::make_shared<RuntimeObject>();
        m->className = "__module__";

        auto number = [](const RuntimeValue& v) {
            if (auto p = std::get_if<double>(&v)) return *p;
            if (auto p = std::get_if<int>(&v)) return static_cast<double>(*p);
            throw std::runtime_error("Expected number");
        };

        auto native = [&](const std::string& n, auto fn) {
            auto f = std::make_shared<NativeFunction>();
            f->name = n;
            f->call = fn;
            m->fields[n] = f;
        };

        if (moduleName == "math") {
            native("add", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 2) throw std::runtime_error("add() expects two arguments");
                return number(number(a.at(0)) + number(a.at(1)));
            });
            native("subtract", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 2) throw std::runtime_error("subtract() expects two arguments");
                return number(number(a.at(0)) - number(a.at(1)));
            });
            native("multiply", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 2) throw std::runtime_error("multiply() expects two arguments");
                return number(number(a.at(0)) * number(a.at(1)));
            });
            native("divide", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 2) throw std::runtime_error("divide() expects two arguments");
                double denominator = number(a.at(1));
                if (denominator == 0.0) throw std::runtime_error("divide() error: Division by zero");
                return number(number(a.at(0)) / denominator);
            });
            native("modulo", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 2) throw std::runtime_error("modulo() expects two arguments");
                return number(std::fmod(number(a.at(0)), number(a.at(1))));
            });
            native("pow", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 2) throw std::runtime_error("pow() expects two arguments");
                return number(std::pow(number(a.at(0)), number(a.at(1))));
            });
            native("sqrt", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("sqrt() expects one argument");
                return number(std::sqrt(number(a.at(0))));
            });
            native("log", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("log() expects one argument");
                return number(std::log(number(a.at(0))));
            });
            native("log10", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("log10() expects one argument");
                return number(std::log10(number(a.at(0))));
            });
            native("exp", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("exp() expects one argument");
                return number(std::exp(number(a.at(0))));
            });
            native("sin", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("sin() expects one argument");
                return number(std::sin(number(a.at(0))));
            });
            native("cos", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("cos() expects one argument");
                return number(std::cos(number(a.at(0))));
            });
            native("tan", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("tan() expects one argument");
                return number(std::tan(number(a.at(0))));
            });
            native("asin", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("asin() expects one argument");
                return number(std::asin(number(a.at(0))));
            });
            native("acos", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("acos() expects one argument");
                return number(std::acos(number(a.at(0))));
            });
            native("atan", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("atan() expects one argument");
                return number(std::atan(number(a.at(0))));
            });
            native("abs", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("abs() expects one argument");
                return number(std::abs(number(a.at(0))));
            });
            native("ceil", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("ceil() expects one argument");
                return number(std::ceil(number(a.at(0))));
            });
            native("floor", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("floor() expects one argument");
                return number(std::floor(number(a.at(0))));
            });
            native("round", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("round() expects one argument");
                return number(std::round(number(a.at(0))));
            });
            native("min", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 2) throw std::runtime_error("min() expects two arguments");
                return number(std::min(number(a.at(0)), number(a.at(1))));
            });
            native("max", [number](const auto& a) -> RuntimeValue {
                if (a.size() != 2) throw std::runtime_error("max() expects two arguments");
                return number(std::max(number(a.at(0)), number(a.at(1))));
            });
            m->fields["pi"] = 3.141592653589793;
        } else if (moduleName == "random") {
            native("int", [](const auto& a) -> RuntimeValue {
                if (a.size() != 2) throw std::runtime_error("random.int() expects two arguments");
                static std::mt19937 g(std::random_device{}());
                return std::uniform_int_distribution<int>(asInt(a.at(0)), asInt(a.at(1)))(g);
            });
            native("float", [](const auto&) -> RuntimeValue {
                static std::mt19937 g(std::random_device{}());
                return std::generate_canonical<double, 53>(g);
            });
        } else if (moduleName == "time") {
            native("now", [](const auto&) -> RuntimeValue {
                return static_cast<int>(
                    std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now().time_since_epoch()
                    ).count()
                );
            });
            native("sleep", [](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("sleep() expects one argument");
                std::this_thread::sleep_for(std::chrono::milliseconds(asInt(a.at(0))));
                return 0;
            });
        } else if (moduleName == "fs") {
            native("exists", [](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("exists() expects one argument");
                return std::filesystem::exists(valueToString(a.at(0)));
            });
            native("is_file", [](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("is_file() expects one argument");
                return std::filesystem::is_regular_file(valueToString(a.at(0)));
            });
            native("is_dir", [](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("is_dir() expects one argument");
                return std::filesystem::is_directory(valueToString(a.at(0)));
            });
            native("read", [](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("read() expects one argument");
                std::ifstream in(valueToString(a.at(0)));
                if (!in) throw std::runtime_error("Cannot open file");
                std::ostringstream ss;
                ss << in.rdbuf();
                return ss.str();
            });
            native("write", [](const auto& a) -> RuntimeValue {
                if (a.size() != 2) throw std::runtime_error("write() expects two arguments");
                std::ofstream out(valueToString(a.at(0)));
                if (!out) throw std::runtime_error("Cannot open file");
                out << valueToString(a.at(1));
                return 0;
            });
        } else {
            native("cwd", [](const auto&) -> RuntimeValue {
                return std::filesystem::current_path().string();
            });
            native("listdir", [](const auto& a) -> RuntimeValue {
                if (a.size() != 1) throw std::runtime_error("listdir() expects one argument");
                auto r = std::make_shared<RuntimeObject>();
                r->className = "__list__";
                int i = 0;
                for (auto& e : std::filesystem::directory_iterator(valueToString(a.at(0)))) {
                    r->fields[std::to_string(i++)] = e.path().filename().string();
                }
                r->fields["__size__"] = i;
                return r;
            });
        }

        modules[moduleName] = m;
        return m;
    }

    auto cached = modules.find(moduleName);
    if (cached != modules.end()) {
        return cached->second;
    }

    std::string normalizedName = moduleName;
    for (char& c : normalizedName) {
        if (c == '\\') c = '/';
    }

    std::filesystem::path requested(normalizedName);
    bool explicitFile = requested.extension() == ".tk";
    bool hasPath = requested.has_parent_path();

    std::vector<std::filesystem::path> sourceFiles;
    std::filesystem::path packageRoot;

    auto collectPackageFiles = [&](const std::filesystem::path& root) {
        if (!std::filesystem::is_directory(root)) return;

        std::vector<std::filesystem::path> found;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
            if (entry.is_regular_file() && entry.path().extension() == ".tk") {
                found.push_back(std::filesystem::absolute(entry.path()));
            }
        }

        std::sort(found.begin(), found.end(),
            [](const auto& a, const auto& b) {
                const bool aDec = a.filename() == "dec.tk";
                const bool bDec = b.filename() == "dec.tk";
                if (aDec != bDec) return aDec;
                return a.generic_string() < b.generic_string();
            }
        );

        sourceFiles = std::move(found);
    };

    if (explicitFile || hasPath) {
        std::vector<std::filesystem::path> candidates = {
            currentDirectory / requested,
            projectDirectory / requested
        };

        for (const auto& candidate : candidates) {
            if (std::filesystem::is_regular_file(candidate)) {
                sourceFiles.push_back(std::filesystem::absolute(candidate));
                break;
            }

            if (std::filesystem::is_directory(candidate)) {
                collectPackageFiles(candidate);
                if (!sourceFiles.empty()) break;
            }
        }
    } else {
        const char* localAppData = std::getenv("LOCALAPPDATA");

        if (localAppData) {
            packageRoot =
                std::filesystem::path(localAppData) /
                "Tekst" / "packages" / requested;

            if (std::filesystem::is_directory(packageRoot)) {
                collectPackageFiles(packageRoot);
            }
        }

        if (sourceFiles.empty()) {
            packageRoot = projectDirectory / "packages" / requested;
            if (std::filesystem::is_directory(packageRoot)) {
                collectPackageFiles(packageRoot);
            }
        }

        if (sourceFiles.empty()) {
            packageRoot = currentDirectory / requested;
            if (std::filesystem::is_directory(packageRoot)) {
                collectPackageFiles(packageRoot);
            }
        }

        if (sourceFiles.empty()) {
            std::vector<std::filesystem::path> candidates = {
                currentDirectory / (normalizedName + ".tk"),
                currentDirectory / "lib" / (normalizedName + ".tk"),
                projectDirectory / (normalizedName + ".tk")
            };

            for (const auto& candidate : candidates) {
                if (std::filesystem::is_regular_file(candidate)) {
                    sourceFiles.push_back(std::filesystem::absolute(candidate));
                    break;
                }
            }
        }
    }

    if (sourceFiles.empty()) {
        throw std::runtime_error("Module not found: " + moduleName);
    }

    auto module = std::make_shared<RuntimeObject>();
    module->className = moduleName;
    modules[moduleName] = module;

    std::map<std::string, RuntimeValue> moduleScope;
    auto oldDirectory = currentDirectory;

    try {
        for (const auto& sourceFile : sourceFiles) {
            currentDirectory = sourceFile.parent_path();

            std::ifstream file(sourceFile);
            if (!file) {
                throw std::runtime_error(
                    "Could not open module: " + sourceFile.string()
                );
            }

            std::ostringstream ss;
            ss << file.rdbuf();

            auto tokens = lexer(ss.str());
            Parser parser(tokens);
            auto program = parser.parse();

            executeBlock(program->statements, moduleScope);
        }
    } catch (...) {
        currentDirectory = oldDirectory;
        modules.erase(moduleName);
        throw;
    }

    currentDirectory = oldDirectory;
    module->fields = moduleScope;

    return module;
}


RuntimeValue Interpreter::evaluate(
    const std::shared_ptr<Expression>& expr,
    std::map<std::string, RuntimeValue>* scope
) {
    if (auto num = std::dynamic_pointer_cast<NumberLiteral>(expr)) {
        return num->value;
    }

    if (auto num = std::dynamic_pointer_cast<FloatLiteral>(expr)) {
        return num->value;
    }

    if (auto b = std::dynamic_pointer_cast<BooleanLiteral>(expr)) {
        return b->value;
    }

    if (auto s = std::dynamic_pointer_cast<StringLiteral>(expr)) {
        return s->value;
    }

    if (auto id = std::dynamic_pointer_cast<Identifier>(expr)) {
        if (scope) {
            auto it = scope->find(id->name);

            if (it != scope->end()) {
                return it->second;
            }
        }

        auto it = variables.find(id->name);

        if (it == variables.end()) {
            throw std::runtime_error(
                "Undefined variable: " + id->name
            );
        }

        return it->second;
    }

    if (auto attr = std::dynamic_pointer_cast<AttributeAccess>(expr)) {
        auto objectValue = evaluate(attr->object, scope);

        if (const auto* obj =
                std::get_if<std::shared_ptr<RuntimeObject>>(&objectValue)) {

            auto it = (*obj)->fields.find(attr->attribute);

            if (it == (*obj)->fields.end()) {
                throw std::runtime_error(
                    "Undefined attribute: " + attr->attribute
                );
            }

            return it->second;
        }

        throw std::runtime_error(
            "Attribute access requires an object"
        );
    }

    if (auto index = std::dynamic_pointer_cast<IndexAccess>(expr)) {
        auto container = evaluate(index->object, scope);
        auto idx = evaluate(index->index, scope);

        if (const auto* obj =
                std::get_if<std::shared_ptr<RuntimeObject>>(&container)) {

            if ((*obj)->className == "__list__") {
                int i = asInt(idx);
                auto it = (*obj)->fields.find(std::to_string(i));

                if (it == (*obj)->fields.end()) {
                    throw std::runtime_error("List index out of range");
                }

                return it->second;
            }

            if ((*obj)->className == "__dict__") {
                if (const auto* key =
                        std::get_if<std::string>(&idx)) {

                    auto it = (*obj)->fields.find(*key);

                    if (it == (*obj)->fields.end()) {
                        throw std::runtime_error(
                            "Key not found: " + *key
                        );
                    }

                    return it->second;
                }

                throw std::runtime_error(
                    "Dictionary keys must be strings"
                );
            }
        }

        throw std::runtime_error(
            "Indexing requires a list or dictionary"
        );
    }

    if (auto list = std::dynamic_pointer_cast<ListLiteral>(expr)) {
        auto obj = std::make_shared<RuntimeObject>();
        obj->className = "__list__";

        for (size_t i = 0; i < list->elements.size(); ++i) {
            obj->fields[std::to_string(i)] =
                evaluate(list->elements[i], scope);
        }

        obj->fields["__size__"] =
            static_cast<int>(list->elements.size());

        return obj;
    }

    if (auto dict = std::dynamic_pointer_cast<DictLiteral>(expr)) {
        auto obj = std::make_shared<RuntimeObject>();
        obj->className = "__dict__";

        for (const auto& [keyExpr, valueExpr] : dict->entries) {
            auto key = evaluate(keyExpr, scope);

            if (const auto* k =
                    std::get_if<std::string>(&key)) {
                obj->fields[*k] =
                    evaluate(valueExpr, scope);
            } else {
                throw std::runtime_error(
                    "Dictionary keys must be strings"
                );
            }
        }

        return obj;
    }

    if (auto call = std::dynamic_pointer_cast<CallExpression>(expr)) {
        std::vector<RuntimeValue> arguments;

        for (const auto& arg : call->args) {
            arguments.push_back(evaluate(arg, scope));
        }

        if (call->callee == "__import__") {
            if (arguments.size() != 1) {
                throw std::runtime_error(
                    "import() requires one module name"
                );
            }

            std::string moduleName =
                valueToString(arguments[0]);

            auto module = loadModule(moduleName);

            if (scope) {
                for (const auto& [name, value] : module->fields) {
                    if (name != "__size__" && name != "__type__") {
                        (*scope)[name] = value;
                    }
                }
            }

            return module;
        }

        if (call->callee == "__import_from__") {
            if (arguments.size() != 2) {
                throw std::runtime_error(
                    "from import requires module and name"
                );
            }

            std::string moduleName =
                valueToString(arguments[0]);

            std::string item =
                valueToString(arguments[1]);

            auto module = loadModule(moduleName);

            auto it = module->fields.find(item);

            if (it == module->fields.end()) {
                throw std::runtime_error(
                    "Module '" + moduleName +
                    "' has no member '" + item + "'"
                );
            }

            return it->second;
        }

        if (call->callee == "input") {
            if (!arguments.empty()) {
                std::cout << valueToString(arguments[0]);
            }

            std::string value;
            std::getline(std::cin, value);

            return value;
        }

        if (call->callee == "int") {
            if (arguments.empty()) {
                throw std::runtime_error(
                    "int() requires an argument"
                );
            }

            if (const auto* v =
                    std::get_if<int>(&arguments[0])) {
                return *v;
            }

            if (const auto* v =
                    std::get_if<bool>(&arguments[0])) {
                return *v ? 1 : 0;
            }

            if (const auto* v =
                    std::get_if<std::string>(&arguments[0])) {
                return std::stoi(*v);
            }

            throw std::runtime_error(
                "int() expected number or string"
            );
        }

        if (call->callee == "str") {
            if (arguments.empty()) {
                return std::string{};
            }

            return valueToString(arguments[0]);
        }

        if (call->callee == "bool") {
            if (arguments.empty()) {
                return false;
            }

            return asBool(arguments[0]);
        }

        if (call->callee == "float") {
            if (arguments.empty()) {
                throw std::runtime_error(
                    "float() requires an argument"
                );
            }

            if (const auto* v =
                    std::get_if<int>(&arguments[0])) {
                return static_cast<double>(*v);
            }

            if (const auto* v =
                    std::get_if<bool>(&arguments[0])) {
                return *v ? 1.0 : 0.0;
            }

            if (const auto* v =
                    std::get_if<double>(&arguments[0])) {
                return *v;
            }

            if (const auto* v =
                    std::get_if<std::string>(&arguments[0])) {
                return std::stod(*v);
            }

            throw std::runtime_error(
                "float() expected number or string"
            );
        }

        if (call->callee == "str") {
            if (arguments.size() != 1) throw std::runtime_error("str() expects one argument");
            return valueToString(arguments[0]);
        }

        if (call->callee == "float") {
            if (arguments.size() != 1) throw std::runtime_error("float() expects one argument");
            if (auto p = std::get_if<double>(&arguments[0])) return *p;
            if (auto p = std::get_if<int>(&arguments[0])) return static_cast<double>(*p);
            if (auto p = std::get_if<bool>(&arguments[0])) return *p ? 1.0 : 0.0;
            if (auto p = std::get_if<std::string>(&arguments[0])) return std::stod(*p);
            throw std::runtime_error("float() expected a number or numeric string");
        }

        if (call->callee == "type") {
            if (arguments.size() != 1) throw std::runtime_error("type() expects one argument");
            if (std::holds_alternative<int>(arguments[0])) return std::string("int");
            if (std::holds_alternative<double>(arguments[0])) return std::string("float");
            if (std::holds_alternative<bool>(arguments[0])) return std::string("bool");
            if (std::holds_alternative<std::string>(arguments[0])) return std::string("string");
            if (std::holds_alternative<std::shared_ptr<RuntimeFunction>>(arguments[0])) return std::string("function");
            if (std::holds_alternative<std::shared_ptr<NativeFunction>>(arguments[0])) return std::string("native_function");
            if (auto p = std::get_if<std::shared_ptr<RuntimeObject>>(&arguments[0])) return (*p)->className;
        }

        if (call->callee == "len") {
            if (arguments.empty()) {
                throw std::runtime_error(
                    "len() requires an argument"
                );
            }

            if (const auto* v =
                    std::get_if<std::string>(&arguments[0])) {
                return static_cast<int>(v->size());
            }

            if (const auto* obj =
                    std::get_if<std::shared_ptr<RuntimeObject>>(
                        &arguments[0])) {

                if ((*obj)->className == "__list__") {
                    int size = 0;

                    while (
                        (*obj)->fields.find(
                            std::to_string(size)
                        ) != (*obj)->fields.end()
                    ) {
                        ++size;
                    }

                    return size;
                }

                if ((*obj)->className == "__dict__") {
                    int size = 0;

                    for (const auto& [k, v] : (*obj)->fields) {
                        if (k != "__size__" &&
                            k != "__type__") {
                            ++size;
                        }
                    }

                    return size;
                }
            }

            throw std::runtime_error(
                "len() expected string, list, or dictionary"
            );
        }

        if (call->callee.find('.') != std::string::npos) {
            size_t dot = call->callee.find('.');

            std::string objectName =
                call->callee.substr(0, dot);

            std::string methodName =
                call->callee.substr(dot + 1);

            RuntimeValue objectValue;

            if (scope) {
                auto it = scope->find(objectName);

                if (it != scope->end()) {
                    objectValue = it->second;
                } else {
                    auto global = variables.find(objectName);

                    if (global == variables.end()) {
                        throw std::runtime_error(
                            "Undefined object: " + objectName
                        );
                    }

                    objectValue = global->second;
                }
            } else {
                auto it = variables.find(objectName);

                if (it == variables.end()) {
                    throw std::runtime_error(
                        "Undefined object: " + objectName
                    );
                }

                objectValue = it->second;
            }

            if (const auto* str = std::get_if<std::string>(&objectValue)) {
                if (methodName == "upper") {
                    if (!arguments.empty()) throw std::runtime_error("upper() expects no arguments");
                    std::string r = *str;
                    for (char& c : r) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                    return r;
                }
                if (methodName == "lower") {
                    if (!arguments.empty()) throw std::runtime_error("lower() expects no arguments");
                    std::string r = *str;
                    for (char& c : r) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    return r;
                }
                if (methodName == "contains") {
                    if (arguments.size() != 1) throw std::runtime_error("contains() expects one argument");
                    return str->find(valueToString(arguments[0])) != std::string::npos;
                }
                if (methodName == "replace") {
                    if (arguments.size() != 2) throw std::runtime_error("replace() expects two arguments");
                    std::string r = *str, from = valueToString(arguments[0]), to = valueToString(arguments[1]);
                    if (!from.empty()) { size_t pos = 0; while ((pos = r.find(from, pos)) != std::string::npos) { r.replace(pos, from.size(), to); pos += to.size(); } }
                    return r;
                }
                if (methodName == "trim") {
                    if (!arguments.empty()) throw std::runtime_error("trim() expects no arguments");
                    size_t a = str->find_first_not_of(" \t\r\n"), b = str->find_last_not_of(" \t\r\n");
                    return a == std::string::npos ? std::string{} : str->substr(a, b-a+1);
                }
            }

            auto obj =
                std::get_if<std::shared_ptr<RuntimeObject>>(
                    &objectValue
                );

            if (!obj) {
                throw std::runtime_error(
                    "Method call target is not an object"
                );
            }

            if ((*obj)->className == "__list__") {
                int size = 0;
                if (auto it = (*obj)->fields.find("__size__"); it != (*obj)->fields.end()) size = asInt(it->second);
                if (methodName == "append") {
                    if (arguments.size() != 1) throw std::runtime_error("append() expects one argument");
                    (*obj)->fields[std::to_string(size)] = arguments[0];
                    (*obj)->fields["__size__"] = size + 1;
                    return 0;
                }
                if (methodName == "pop") {
                    if (size == 0) throw std::runtime_error("pop() from empty list");
                    auto key = std::to_string(size - 1);
                    RuntimeValue value = (*obj)->fields.at(key);
                    (*obj)->fields.erase(key);
                    (*obj)->fields["__size__"] = size - 1;
                    return value;
                }
                if (methodName == "contains") {
                    if (arguments.size() != 1) throw std::runtime_error("contains() expects one argument");
                    for (int i=0;i<size;++i) if (valueToString((*obj)->fields.at(std::to_string(i))) == valueToString(arguments[0])) return true;
                    return false;
                }
                if (methodName == "clear") {
                    for (int i=0;i<size;++i) (*obj)->fields.erase(std::to_string(i));
                    (*obj)->fields["__size__"] = 0;
                    return 0;
                }
            }

            if ((*obj)->className == "__dict__") {
                if (methodName == "keys") {
                    auto r=std::make_shared<RuntimeObject>(); r->className="__list__"; int i=0;
                    for (const auto& [k,v] : (*obj)->fields) if(k!="__size__"&&k!="__type__") r->fields[std::to_string(i++)]=k;
                    r->fields["__size__"]=i; return r;
                }
                if (methodName == "values") {
                    auto r=std::make_shared<RuntimeObject>(); r->className="__list__"; int i=0;
                    for (const auto& [k,v] : (*obj)->fields) if(k!="__size__"&&k!="__type__") r->fields[std::to_string(i++)]=v;
                    r->fields["__size__"]=i; return r;
                }
                if (methodName == "get") {
                    if (arguments.empty()) throw std::runtime_error("get() expects a key");
                    auto key=valueToString(arguments[0]); auto it=(*obj)->fields.find(key);
                    if(it!=(*obj)->fields.end()) return it->second;
                    return arguments.size()>1 ? arguments[1] : RuntimeValue{0};
                }
            }

            auto method =
                (*obj)->fields.find(methodName);

            if (method == (*obj)->fields.end()) {
                throw std::runtime_error(
                    "Undefined method: " + methodName
                );
            }

            if (const auto* fn = std::get_if<std::shared_ptr<RuntimeFunction>>(&method->second)) {
                auto bound = std::make_shared<RuntimeFunction>(**fn);
                bound->self = *obj;
                return callFunction(bound, arguments);
            }

            if (const auto* nf = std::get_if<std::shared_ptr<NativeFunction>>(&method->second)) {
                return (*nf)->call(arguments);
            }

            throw std::runtime_error(
                "Object member is not callable"
            );
        }

        if (call->callee == "range") {
            if (arguments.size() < 1 || arguments.size() > 3) throw std::runtime_error("range() expects 1 to 3 arguments");
            auto toIntArg = [](const RuntimeValue& v){ return asInt(v); };
            int start = 0, stop = 0, step = 1;
            if (arguments.size() == 1) stop = toIntArg(arguments[0]);
            else { start = toIntArg(arguments[0]); stop = toIntArg(arguments[1]); if (arguments.size() == 3) step = toIntArg(arguments[2]); }
            if (step == 0) throw std::runtime_error("range() step cannot be zero");
            auto r = std::make_shared<RuntimeObject>(); r->className = "__list__"; int n=0;
            if (step > 0) for (int x=start; x<stop; x+=step) r->fields[std::to_string(n++)]=x;
            else for (int x=start; x>stop; x+=step) r->fields[std::to_string(n++)]=x;
            r->fields["__size__"] = n; return r;
        }

        auto callNative = [&](const std::string& name) -> std::optional<RuntimeValue> {
            auto lookup = [&](const std::map<std::string, RuntimeValue>& m) -> const RuntimeValue* {
                auto it = m.find(name);
                return it == m.end() ? nullptr : &it->second;
            };
            const RuntimeValue* v = scope ? lookup(*scope) : nullptr;
            if (!v) v = lookup(variables);
            if (!v) return std::nullopt;
            if (auto nf = std::get_if<std::shared_ptr<NativeFunction>>(v))
                return (*nf)->call(arguments);
            return std::nullopt;
        };

        if (auto nativeResult = callNative(call->callee)) return *nativeResult;

        RuntimeValue functionValue;

        if (scope) {
            auto it = scope->find(call->callee);

            if (it != scope->end()) {
                functionValue = it->second;
            } else {
                auto global = variables.find(call->callee);

                if (global == variables.end()) {
                    throw std::runtime_error(
                        "Undefined function or class: " +
                        call->callee
                    );
                }

                functionValue = global->second;
            }
        } else {
            auto it = variables.find(call->callee);

            if (it == variables.end()) {
                throw std::runtime_error(
                    "Undefined function or class: " +
                    call->callee
                );
            }

            functionValue = it->second;
        }

        if (const auto* fn =
                std::get_if<std::shared_ptr<RuntimeFunction>>(
                    &functionValue)) {

            return callFunction(
                *fn,
                arguments
            );
        }

        if (const auto* obj =
                std::get_if<std::shared_ptr<RuntimeObject>>(
                    &functionValue)) {

            auto instance =
                std::make_shared<RuntimeObject>();

            instance->className =
                (*obj)->className;

            auto initIt =
                (*obj)->fields.find("__init__");

            if (initIt != (*obj)->fields.end()) {
                if (const auto* initFn =
                        std::get_if<std::shared_ptr<RuntimeFunction>>(
                            &initIt->second)) {

                    auto init =
                        std::make_shared<RuntimeFunction>(**initFn);

                    init->self = instance;

                    std::vector<std::string> params;

                    if (init->parameters.size() > 1) {
                        params.assign(
                            init->parameters.begin() + 1,
                            init->parameters.end()
                        );
                    }

                    if (arguments.size() != params.size()) {
                        throw std::runtime_error(
                            "__init__ expected " +
                            std::to_string(params.size()) +
                            " arguments, got " +
                            std::to_string(arguments.size())
                        );
                    }

                    std::map<std::string, RuntimeValue> localScope =
                        init->closure;

                    localScope["self"] = instance;

                    for (size_t i = 0; i < params.size(); ++i) {
                        localScope[params[i]] = arguments[i];
                    }

                    executeBlock(
                        init->body,
                        localScope
                    );
                }
            }

            for (const auto& [name, value] : (*obj)->fields) {
                if (name != "__init__") {
                    instance->fields[name] = value;
                }
            }

            return instance;
        }

        throw std::runtime_error(
            "Object is not callable"
        );
    }

    if (auto unary = std::dynamic_pointer_cast<UnaryOp>(expr)) {
        RuntimeValue value = evaluate(unary->operand, scope);
        if (unary->op == "not") return !asBool(value);
        if (unary->op == "+") {
            if (std::holds_alternative<double>(value)) return value;
            return asInt(value);
        }
        if (unary->op == "-") {
            if (std::holds_alternative<double>(value)) return -std::get<double>(value);
            return -asInt(value);
        }
        throw std::runtime_error("Unknown unary operator: " + unary->op);
    }

    if (auto binop = std::dynamic_pointer_cast<BinaryOp>(expr)) {
        RuntimeValue left = evaluate(binop->left, scope);
        if (binop->op == "and") {
            if (!asBool(left)) return false;
            return asBool(evaluate(binop->right, scope));
        }
        if (binop->op == "or") {
            if (asBool(left)) return true;
            return asBool(evaluate(binop->right, scope));
        }
        RuntimeValue right = evaluate(binop->right, scope);

        auto num = [](const RuntimeValue& v) -> double {
            if (auto p = std::get_if<int>(&v)) return *p;
            if (auto p = std::get_if<double>(&v)) return *p;
            if (auto p = std::get_if<bool>(&v)) return *p ? 1.0 : 0.0;
            throw std::runtime_error("Expected numeric value");
        };
        bool floating = std::holds_alternative<double>(left) || std::holds_alternative<double>(right);

        if (binop->op == "+") {
            if (std::get_if<std::string>(&left) && std::get_if<std::string>(&right))
                return std::get<std::string>(left) + std::get<std::string>(right);
            if (floating) return num(left) + num(right);
            if (std::get_if<int>(&left) && std::get_if<int>(&right)) return asInt(left) + asInt(right);
            return valueToString(left) + valueToString(right);
        }
        if (binop->op == "-") return floating ? RuntimeValue(num(left)-num(right)) : RuntimeValue(asInt(left)-asInt(right));
        if (binop->op == "*") return floating ? RuntimeValue(num(left)*num(right)) : RuntimeValue(asInt(left)*asInt(right));
        if (binop->op == "/") {
            double divisor = num(right);
            if (divisor == 0) throw std::runtime_error("Division by zero");
            return num(left) / divisor;
        }
        if (binop->op == "%") {
            if (floating) {
                double divisor = num(right);
                if (divisor == 0.0) throw std::runtime_error("Modulo by zero");
                return std::fmod(num(left), divisor);
            }
            int divisor = asInt(right);
            if (divisor == 0) throw std::runtime_error("Modulo by zero");
            return asInt(left) % divisor;
        }
        if (binop->op == "==") {
            return valueToString(left) ==
                   valueToString(right);
        }

        if (binop->op == "!=") {
            return valueToString(left) !=
                   valueToString(right);
        }

        if (binop->op == "<") return num(left) < num(right);

        if (binop->op == "<=") return num(left) <= num(right);

        if (binop->op == ">") return num(left) > num(right);

        if (binop->op == ">=") return num(left) >= num(right);

        throw std::runtime_error(
            "Unknown operator: " + binop->op
        );
    }

    throw std::runtime_error(
        "Cannot evaluate expression"
    );
}

namespace {
struct BreakSignal {};
struct ContinueSignal {};
}

std::optional<RuntimeValue> Interpreter::executeStatement(
    const std::shared_ptr<ASTNode>& stmt,
    std::map<std::string, RuntimeValue>& scope
) {
    if (auto assign =
            std::dynamic_pointer_cast<Assignment>(stmt)) {

        auto value =
            evaluate(assign->expr, &scope);

        if (assign->var.find('.') != std::string::npos) {
            size_t dot = assign->var.find('.');

            std::string objectName =
                assign->var.substr(0, dot);

            std::string attribute =
                assign->var.substr(dot + 1);

            auto it = scope.find(objectName);

            if (it == scope.end()) {
                throw std::runtime_error(
                    "Undefined object: " + objectName
                );
            }

            auto obj =
                std::get_if<std::shared_ptr<RuntimeObject>>(
                    &it->second
                );

            if (!obj) {
                throw std::runtime_error(
                    "Attribute assignment requires object"
                );
            }

            (*obj)->fields[attribute] = value;

            return std::nullopt;
        }

        scope[assign->var] = value;

        return std::nullopt;
    }

    if (auto printStmt =
            std::dynamic_pointer_cast<PrintStatement>(stmt)) {

        std::cout <<
            valueToString(
                evaluate(printStmt->expr, &scope)
            ) << '\n';

        return std::nullopt;
    }

    if (auto callExpr =
            std::dynamic_pointer_cast<CallExpression>(stmt)) {

        evaluate(callExpr, &scope);

        return std::nullopt;
    }

    if (auto thr = std::dynamic_pointer_cast<ThrowStatement>(stmt)) {
        throw std::runtime_error(valueToString(evaluate(thr->expr, &scope)));
    }

    if (std::dynamic_pointer_cast<BreakStatement>(stmt)) {
        throw BreakSignal{};
    }

    if (std::dynamic_pointer_cast<ContinueStatement>(stmt)) {
        throw ContinueSignal{};
    }

    if (auto ret =
            std::dynamic_pointer_cast<ReturnStatement>(stmt)) {

        if (ret->expr) {
            return evaluate(ret->expr, &scope);
        }

        return RuntimeValue{0};
    }

    if (auto func =
            std::dynamic_pointer_cast<FunctionDef>(stmt)) {

        auto fn = std::make_shared<RuntimeFunction>();

        fn->name = func->name;
        fn->parameters = func->parameters;
        fn->defaultArgs = func->defaultArgs;
        fn->variadicParameter = func->variadicParameter;
        fn->body = func->body;
        fn->closure = scope;
        fn->closure[func->name] = fn;

        scope[func->name] = fn;

        return std::nullopt;
    }

    if (auto cls =
            std::dynamic_pointer_cast<ClassDef>(stmt)) {

        auto classObject =
            std::make_shared<RuntimeObject>();

        classObject->className = cls->name;

        for (const auto& child : cls->body) {
            if (auto innerFunc =
                    std::dynamic_pointer_cast<FunctionDef>(child)) {

                auto fn =
                    std::make_shared<RuntimeFunction>();

                fn->name = innerFunc->name;
                fn->parameters = innerFunc->parameters;
                fn->defaultArgs = innerFunc->defaultArgs;
                fn->variadicParameter = innerFunc->variadicParameter;
                fn->body = innerFunc->body;
                fn->closure = scope;

                classObject->fields[innerFunc->name] = fn;
            } else if (auto innerAssign =
                           std::dynamic_pointer_cast<Assignment>(child)) {

                classObject->fields[innerAssign->var] =
                    evaluate(innerAssign->expr, &scope);
            }
        }

        scope[cls->name] = classObject;

        return std::nullopt;
    }

    if (auto ifStmt =
            std::dynamic_pointer_cast<IfStatement>(stmt)) {

        if (asBool(
                evaluate(ifStmt->condition, &scope)
            )) {

            return executeBlock(
                ifStmt->body,
                scope
            );
        }

        if (!ifStmt->elseBody.empty()) {
            return executeBlock(
                ifStmt->elseBody,
                scope
            );
        }

        return std::nullopt;
    }

    if (auto whileStmt =
            std::dynamic_pointer_cast<WhileStatement>(stmt)) {

        while (asBool(
            evaluate(whileStmt->condition, &scope)
        )) {
            try {
                auto result = executeBlock(whileStmt->body, scope);
                if (result.has_value()) return result;
            } catch (const ContinueSignal&) {
                continue;
            } catch (const BreakSignal&) {
                break;
            }
        }

        return std::nullopt;
    }

    if (auto forStmt =
            std::dynamic_pointer_cast<ForStatement>(stmt)) {

        auto iterable =
            evaluate(forStmt->iterable, &scope);

        auto obj =
            std::get_if<std::shared_ptr<RuntimeObject>>(
                &iterable
            );

        if (!obj || (*obj)->className != "__list__") {
            throw std::runtime_error("For loops require an iterable list");
        }

        for (size_t i = 0; i < 1000; ++i) {
            auto it =
                (*obj)->fields.find(
                    std::to_string(i)
                );

            if (it == (*obj)->fields.end()) {
                break;
            }

            scope[forStmt->var] = it->second;

            try {
                auto result = executeBlock(forStmt->body, scope);
                if (result.has_value()) return result;
            } catch (const ContinueSignal&) {
                continue;
            } catch (const BreakSignal&) {
                break;
            }
        }

        return std::nullopt;
    }

    if (auto tryStmt =
            std::dynamic_pointer_cast<TryStatement>(stmt)) {

        try {
            return executeBlock(
                tryStmt->tryBody,
                scope
            );
        } catch (const BreakSignal&) {
            throw;
        } catch (const ContinueSignal&) {
            throw;
        } catch (const std::exception&) {
            return executeBlock(
                tryStmt->catchBody,
                scope
            );
        }
    }

    throw std::runtime_error(
        "Unsupported statement type"
    );
}

std::optional<RuntimeValue> Interpreter::executeBlock(
    const std::vector<std::shared_ptr<ASTNode>>& block,
    std::map<std::string, RuntimeValue>& scope
) {
    for (const auto& stmt : block) {
        auto result =
            executeStatement(stmt, scope);

        if (result.has_value()) {
            return result;
        }
    }

    return std::nullopt;
}

void Interpreter::execute(
    const std::shared_ptr<Program>& program,
    const std::filesystem::path& sourceDirectory
) {
    currentDirectory = std::filesystem::absolute(sourceDirectory);
    projectDirectory = currentDirectory;

    while (!projectDirectory.empty()) {
        if (std::filesystem::exists(projectDirectory / "tekst.toml") ||
            std::filesystem::is_directory(projectDirectory / "local" / "tekst") ||
            std::filesystem::is_directory(projectDirectory / "packages")) {
            break;
        }

        auto parent = projectDirectory.parent_path();
        if (parent == projectDirectory) {
            projectDirectory.clear();
            break;
        }

        projectDirectory = parent;
    }

    if (projectDirectory.empty()) {
        projectDirectory = currentDirectory;
    }

    executeBlock(
        program->statements,
        variables
    );
}

void Interpreter::printVariables() const {
    std::cout << "\nVariables:\n";

    for (const auto& [name, value] : variables) {
        std::cout <<
            "  " <<
            name <<
            " = " <<
            valueToString(value) <<
            '\n';
    }
}







