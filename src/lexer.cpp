#include "lexer.h"
#include <stdexcept>
#include <cctype>

static bool isKeyword(const std::string& text) {
    static const std::vector<std::string> keywords = {
        "print", "if", "else", "elif", "while", "for", "in",
        "and", "or", "not", "def", "fn", "let", "class",
        "return", "extends", "try", "catch", "throw", "break", "continue",
        "import", "from", "as", "true", "false", "null"
    };

    for (const auto& kw : keywords) {
        if (text == kw) return true;
    }

    return false;
}

std::vector<Token> lexer(const std::string& source) {
    std::vector<Token> tokens;
    std::vector<int> indentationStack = {0};

    size_t i = 0;
    bool atLineStart = true;
    int currentLineIndent = 0;
    int line = 1;
    int column = 1;

    while (i < source.length()) {
        char c = source[i];

        if (atLineStart) {
            if (c == '\n') {
                tokens.push_back({TokenType::NEWLINE, "\n"});
                ++i;
                ++line;
                column = 1;
                currentLineIndent = 0;
                continue;
            }

            if (c == ' ' || c == '\t') {
                currentLineIndent += c == '\t' ? 4 : 1;
                ++i;
                ++column;
                continue;
            }

            while (indentationStack.size() > 1 &&
                   currentLineIndent < indentationStack.back()) {
                indentationStack.pop_back();
                tokens.push_back({
                    TokenType::DEDENT,
                    std::to_string(indentationStack.back())
                });
            }

            if (currentLineIndent > indentationStack.back()) {
                indentationStack.push_back(currentLineIndent);
                tokens.push_back({
                    TokenType::INDENT,
                    std::to_string(currentLineIndent)
                });
            } else if (currentLineIndent < indentationStack.back()) {
                throw std::runtime_error("Inconsistent indentation");
            }

            atLineStart = false;
            column = currentLineIndent + 1;
            continue;
        }

        if (c == '\n') {
            tokens.push_back({TokenType::NEWLINE, "\n"});
            ++i;
            ++line;
            column = 1;
            atLineStart = true;
            currentLineIndent = 0;
            continue;
        }

        if (std::isspace(static_cast<unsigned char>(c))) {
            ++i;
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c)) ||
            (c == '.' && i + 1 < source.length() &&
             std::isdigit(static_cast<unsigned char>(source[i + 1])))) {
            std::string num;
            bool hasDot = false;

            while (i < source.length()) {
                char n = source[i];
                if (std::isdigit(static_cast<unsigned char>(n))) {
                    num += n;
                    ++i;
                } else if (n == '.' && !hasDot) {
                    hasDot = true;
                    num += n;
                    ++i;
                } else {
                    break;
                }
            }

            tokens.push_back({TokenType::NUMBER, num});
            continue;
        }

        if (c == '"') {
            ++i;
            std::string str;

            while (i < source.length() && source[i] != '"') {
                if (source[i] == '\\') {
                    ++i;
                    if (i >= source.length()) break;
                    char e = source[i++];
                    switch (e) {
                        case 'n': str += '\n'; break;
                        case 't': str += '\t'; break;
                        case 'r': str += '\r'; break;
                        case '\\': str += '\\'; break;
                        case '"': str += '"'; break;
                        default: str += e; break;
                    }
                } else {
                    str += source[i++];
                }
            }

            if (i >= source.length()) {
                throw std::runtime_error("Unterminated string");
            }

            ++i;
            tokens.push_back({TokenType::STR, str});
            continue;
        }

        if (c == '=' && i + 1 < source.length() && source[i + 1] == '=') {
            tokens.push_back({TokenType::EQUAL, "=="});
            i += 2;
            continue;
        }

        if (c == '!' && i + 1 < source.length() && source[i + 1] == '=') {
            tokens.push_back({TokenType::NOT_EQUAL, "!="});
            i += 2;
            continue;
        }

        if (c == '!') {
            tokens.push_back({TokenType::NOT, "!", line, column});
            ++i; ++column; continue;
        }

        if (c == '<') {
            if (i + 1 < source.length() && source[i + 1] == '=') {
                tokens.push_back({TokenType::LESS_EQUAL, "<="});
                i += 2;
            } else {
                tokens.push_back({TokenType::LESS, "<"});
                ++i;
            }
            continue;
        }

        if (c == '>') {
            if (i + 1 < source.length() && source[i + 1] == '=') {
                tokens.push_back({TokenType::GREATER_EQUAL, ">="});
                i += 2;
            } else {
                tokens.push_back({TokenType::GREATER, ">"});
                ++i;
            }
            continue;
        }

        if (c == '=') {
            tokens.push_back({TokenType::ASSIGN, "=", line, column});
            ++i;
            continue;
        }

        if (c == '+' && i + 1 < source.length() && source[i + 1] == '=') {
            tokens.push_back({TokenType::PLUS_ASSIGN, "+=", line, column});
            i += 2; column += 2; continue;
        }

        if (c == '+') {
            tokens.push_back({TokenType::PLUS, "+", line, column});
            ++i;
            continue;
        }

        if (c == '-' && i + 1 < source.length() && source[i + 1] == '=') {
            tokens.push_back({TokenType::MINUS_ASSIGN, "-=", line, column});
            i += 2; column += 2; continue;
        }

        if (c == '-') {
            tokens.push_back({TokenType::MINUS, "-", line, column});
            ++i;
            continue;
        }

        if (c == '*' && i + 1 < source.length() && source[i + 1] == '=') {
            tokens.push_back({TokenType::STAR_ASSIGN, "*=", line, column});
            i += 2; column += 2; continue;
        }

        if (c == '*') {
            tokens.push_back({TokenType::STAR, "*", line, column});
            ++i;
            continue;
        }

        if (c == '/' && i + 1 < source.length() && source[i + 1] == '=') {
            tokens.push_back({TokenType::SLASH_ASSIGN, "/=", line, column});
            i += 2; column += 2; continue;
        }

        if (c == '/') {
            tokens.push_back({TokenType::SLASH, "/", line, column});
            ++i;
            continue;
        }

        if (c == '%' && i + 1 < source.length() && source[i + 1] == '=') {
            tokens.push_back({TokenType::MOD_ASSIGN, "%=", line, column});
            i += 2; column += 2; continue;
        }

        if (c == '%') {
            tokens.push_back({TokenType::MOD, "%", line, column});
            ++i;
            continue;
        }

        if (c == '&' && i + 1 < source.length() && source[i + 1] == '&') {
            tokens.push_back({TokenType::AND, "&&", line, column}); i += 2; column += 2; continue;
        }

        if (c == '|' && i + 1 < source.length() && source[i + 1] == '|') {
            tokens.push_back({TokenType::OR, "||", line, column}); i += 2; column += 2; continue;
        }

        if (c == '(') {
            tokens.push_back({TokenType::LPAREN, "("});
            ++i;
            continue;
        }

        if (c == ')') {
            tokens.push_back({TokenType::RPAREN, ")"});
            ++i;
            continue;
        }

        if (c == '[') {
            tokens.push_back({TokenType::LBRACKET, "["});
            ++i;
            continue;
        }

        if (c == ']') {
            tokens.push_back({TokenType::RBRACKET, "]"});
            ++i;
            continue;
        }

        if (c == '{') {
            tokens.push_back({TokenType::LBRACE, "{"});
            ++i;
            continue;
        }

        if (c == '}') {
            tokens.push_back({TokenType::RBRACE, "}"});
            ++i;
            continue;
        }

        if (c == ':') {
            tokens.push_back({TokenType::COLON, ":"});
            ++i;
            continue;
        }

        if (c == ',') {
            tokens.push_back({TokenType::COMMA, ","});
            ++i;
            continue;
        }

        if (c == '.') {
            tokens.push_back({TokenType::DOT, "."});
            ++i;
            continue;
        }
        if (c == '\\') {
            tokens.push_back({TokenType::BACKSLASH, "\\"});
            ++i;
            continue;
        }


        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::string name;

            while (i < source.length() &&
                   (std::isalnum(static_cast<unsigned char>(source[i])) ||
                    source[i] == '_')) {
                name += source[i++];
            }

            if (isKeyword(name)) {
                tokens.push_back({TokenType::KEYWORD, name});
            } else {
                tokens.push_back({TokenType::NAME, name});
            }

            continue;
        }

        throw std::runtime_error(std::string("Unknown character: ") + c);
    }

    while (indentationStack.size() > 1) {
        indentationStack.pop_back();
        tokens.push_back({
            TokenType::DEDENT,
            std::to_string(indentationStack.back())
        });
    }

    tokens.push_back({TokenType::EOF_TOKEN, ""});

    return tokens;
}



