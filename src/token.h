
#pragma once
#include <string>
#include <vector>
#include <cstddef>

enum class TokenKind {
    Identifier, Number, String,
    Newline, Indent, Dedent, End,
    Plus, Minus, Star, Slash, Percent, Ampersand,
    Eq, EqEq, NotEq, Lt, Le, Gt, Ge,
    LParen, RParen, LBracket, RBracket, LBrace, RBrace,
    Comma, Colon, Dot,
    PlusEq, MinusEq, StarEq, SlashEq,
    Keyword
};

struct Token {
    TokenKind kind;
    std::string text;
    int line = 1;
    int column = 1;
};
std::vector<Token> lex(const std::string& source);
