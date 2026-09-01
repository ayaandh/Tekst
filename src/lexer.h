#pragma once

#include <string>
#include <vector>
#include <iostream>

enum class TokenType {
    NUMBER,
    NAME,
    STR,
    KEYWORD,
    ASSIGN,
    EQUAL,
    NOT_EQUAL,
    LESS,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    MOD,
    LPAREN,
    RPAREN,
    LBRACKET,
    RBRACKET,
    LBRACE,
    RBRACE,
    COLON,
    COMMA,
    DOT,
    INDENT,
    DEDENT,
    NEWLINE,
    EOF_TOKEN
};

struct Token {
    TokenType type;
    std::string value;
};

std::vector<Token> lexer(const std::string& source);
