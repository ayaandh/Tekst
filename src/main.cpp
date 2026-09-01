#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include "lexer.h"
#include "parser.h"

void printTokens(const std::vector<Token>& tokens) {
    std::cout << "=== LEXICAL ANALYSIS ===\n";

    for (const auto& token : tokens) {
        std::cout << "Token: ";

        switch (token.type) {
            case TokenType::NUMBER: std::cout << "NUMBER"; break;
            case TokenType::NAME: std::cout << "NAME"; break;
            case TokenType::STR: std::cout << "STR"; break;
            case TokenType::KEYWORD: std::cout << "KEYWORD"; break;
            case TokenType::ASSIGN: std::cout << "ASSIGN"; break;
            case TokenType::EQUAL: std::cout << "EQUAL"; break;
            case TokenType::NOT_EQUAL: std::cout << "NOT_EQUAL"; break;
            case TokenType::LESS: std::cout << "LESS"; break;
            case TokenType::LESS_EQUAL: std::cout << "LESS_EQUAL"; break;
            case TokenType::GREATER: std::cout << "GREATER"; break;
            case TokenType::GREATER_EQUAL: std::cout << "GREATER_EQUAL"; break;
            case TokenType::PLUS: std::cout << "PLUS"; break;
            case TokenType::MINUS: std::cout << "MINUS"; break;
            case TokenType::STAR: std::cout << "STAR"; break;
            case TokenType::SLASH: std::cout << "SLASH"; break;
            case TokenType::MOD: std::cout << "MOD"; break;
            case TokenType::LPAREN: std::cout << "LPAREN"; break;
            case TokenType::RPAREN: std::cout << "RPAREN"; break;
            case TokenType::LBRACKET: std::cout << "LBRACKET"; break;
            case TokenType::RBRACKET: std::cout << "RBRACKET"; break;
            case TokenType::LBRACE: std::cout << "LBRACE"; break;
            case TokenType::RBRACE: std::cout << "RBRACE"; break;
            case TokenType::COLON: std::cout << "COLON"; break;
            case TokenType::COMMA: std::cout << "COMMA"; break;
            case TokenType::DOT: std::cout << "DOT"; break;
            case TokenType::INDENT: std::cout << "INDENT"; break;
            case TokenType::DEDENT: std::cout << "DEDENT"; break;
            case TokenType::NEWLINE: std::cout << "NEWLINE"; break;
            case TokenType::EOF_TOKEN: std::cout << "EOF"; break;
        }

        std::cout << " -> '" << token.value << "'\n";
    }

    std::cout << '\n';
}

int main(int argc, char* argv[]) {
    bool debug = false;
    std::string filePath;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            std::cout << "Tekst interpreter\n";
            std::cout << "Usage: parser [--debug] <source-file>\n";
            return 0;
        }

        if (arg == "--debug" || arg == "-d") {
            debug = true;
        } else if (filePath.empty()) {
            filePath = arg;
        }
    }

    if (filePath.empty()) {
        filePath = "src/main.tekst";
    }

    std::filesystem::path path = filePath;

    if (!std::filesystem::exists(path)) {
        std::filesystem::path fallback =
            std::filesystem::path("src") / filePath;

        if (std::filesystem::exists(fallback)) {
            path = fallback;
        }
    }

    std::ifstream file(path);

    if (!file) {
        std::cerr <<
            "Could not open file: " <<
            path.string() <<
            '\n';

        return 1;
    }

    std::string source;
    std::string line;

    while (std::getline(file, line)) {
        source += line;
        source += '\n';
    }

    file.close();

    try {
        auto tokens = lexer(source);

        if (debug) {
            printTokens(tokens);
        }

        Parser parser(tokens);
        auto program = parser.parse();

        if (debug) {
            std::cout << "=== PARSED PROGRAM ===\n";
            std::cout << program->toString() << '\n';
        }

        Interpreter interpreter;

        interpreter.execute(
            program,
            path.parent_path().empty() ? std::filesystem::path(".") : path.parent_path()
        );
    }
    catch (const std::exception& e) {
        std::cerr <<
            "Error: " <<
            e.what() <<
            '\n';

        return 1;
    }

    return 0;
}