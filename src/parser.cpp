#include "parser.h"
#include "value.h"
#include "parser.h"
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
    if (!check(TokenType::NAME)) throw std::runtime_error(parserError(current(), "expected assignment target"));
    std::string name = current().value;
    advance();

    if (match(TokenType::LBRACKET)) {
        auto index = parseExpression();
        if (!match(TokenType::RBRACKET)) throw std::runtime_error(parserError(current(), "expected ']' after index"));
        std::string op = "=";
        if (check(TokenType::ASSIGN) || check(TokenType::PLUS_ASSIGN) || check(TokenType::MINUS_ASSIGN) || check(TokenType::STAR_ASSIGN) || check(TokenType::SLASH_ASSIGN) || check(TokenType::MOD_ASSIGN)) {
            op = current().value;
            advance();
        } else {
            throw std::runtime_error(parserError(current(), "expected assignment operator after index"));
        }
        return std::make_shared<IndexAssignment>(std::make_shared<Identifier>(name), index, parseExpression(), op);
    }

    std::string varName = name;
    if (match(TokenType::DOT)) {
        if (!check(TokenType::NAME)) throw std::runtime_error(parserError(current(), "expected attribute name after '.'"));
        varName += "." + current().value;
        advance();
    }

    std::string op = "=";
    if (check(TokenType::ASSIGN) || check(TokenType::PLUS_ASSIGN) || check(TokenType::MINUS_ASSIGN) || check(TokenType::STAR_ASSIGN) || check(TokenType::SLASH_ASSIGN) || check(TokenType::MOD_ASSIGN)) {
        op = current().value;
        advance();
    } else {
        throw std::runtime_error(parserError(current(), "expected assignment operator after variable name"));
    }

    auto right = parseExpression();
    if (op != "=") {
        std::string base = op.substr(0, 1);
        right = std::make_shared<BinaryOp>(std::make_shared<Identifier>(varName), base, right);
    }
    return std::make_shared<Assignment>(varName, right);
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
    while (checkKeyword("and") || checkKeyword("or") || check(TokenType::AND) || check(TokenType::OR)) {
        std::string op = current().value;
        advance();
        auto right = parseComparison();
        left = std::make_shared<BinaryOp>(left, op, right);
    }
    return left;
}

std::shared_ptr<Expression> Parser::parseComparison() {
    auto left = parseAdditive();
    while (check(TokenType::EQUAL) || check(TokenType::NOT_EQUAL) || check(TokenType::LESS) || check(TokenType::LESS_EQUAL) || check(TokenType::GREATER) || check(TokenType::GREATER_EQUAL)) {
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
    if (checkKeyword("not") || check(TokenType::NOT)) {
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
        if (!match(TokenType::RPAREN)) throw std::runtime_error(parserError(current(), "expected ')' after expression"));
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
    } else if (checkKeyword("null")) {
        advance();
        expr = std::make_shared<NullLiteral>();
    } else if (check(TokenType::LBRACKET)) {
        expr = parseListLiteral();
    } else if (check(TokenType::LBRACE)) {
        expr = parseDictLiteral();
    } else if (check(TokenType::NAME)) {
        std::string name = current().value;
        advance();
        expr = std::make_shared<Identifier>(name);
    } else {
        throw std::runtime_error(parserError(current(), "unexpected token '" + current().value + "'"));
    }

    while (true) {
        if (match(TokenType::DOT)) {
            if (!check(TokenType::NAME)) throw std::runtime_error(parserError(current(), "expected attribute name after '.'"));
            std::string attribute = current().value;
            advance();
            expr = std::make_shared<AttributeAccess>(expr, attribute);
            continue;
        }
        if (match(TokenType::LBRACKET)) {
            std::shared_ptr<Expression> first;
            std::shared_ptr<Expression> second;
            if (!check(TokenType::COLON)) first = parseExpression();
            if (match(TokenType::COLON)) {
                if (!check(TokenType::RBRACKET)) second = parseExpression();
                if (!match(TokenType::RBRACKET)) throw std::runtime_error(parserError(current(), "expected ']' after slice"));
                expr = std::make_shared<SliceAccess>(expr, first, second);
            } else {
                if (!first) throw std::runtime_error(parserError(current(), "expected index or slice"));
                if (!match(TokenType::RBRACKET)) throw std::runtime_error(parserError(current(), "expected ']' after index"));
                expr = std::make_shared<IndexAccess>(expr, first);
            }
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
            if (!match(TokenType::RPAREN)) throw std::runtime_error(parserError(current(), "expected ')' after arguments"));
            std::function<std::string(const std::shared_ptr<Expression>&)> buildName =
                [&](const std::shared_ptr<Expression>& e) -> std::string {
                    if (auto id = std::dynamic_pointer_cast<Identifier>(e)) return id->name;
                    if (auto attr = std::dynamic_pointer_cast<AttributeAccess>(e)) return buildName(attr->object) + "." + attr->attribute;
                    throw std::runtime_error(parserError(current(), "invalid call target"));
                };
            expr = std::make_shared<CallExpression>(buildName(expr), args);
            continue;
        }
        break;
    }
    return expr;
}

