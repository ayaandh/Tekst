#pragma once

#include <string>
#include <vector>

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
    EOF_TOKEN,
    BACKSLASH,
    PLUS_ASSIGN,
    MINUS_ASSIGN,
    STAR_ASSIGN,
    SLASH_ASSIGN,
    MOD_ASSIGN,
    AND,
    OR,
    NOT,
    NULL_TOKEN
};

struct Token {
    TokenType type;
    std::string value;
    int line = 1;
    int column = 1;
};

std::vector<Token> lexer(const std::string& source);



