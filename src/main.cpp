#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include "lexer.h"
#include "parser.h"

void printTokens(const std::vector<Token>& tokens) {
    std::cout << "=== LEXICAL ANALYSIS ===\n";
    for (const Token& token : tokens) {
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

std::string resolveInputPath(const std::string& requestedPath) {
    std::filesystem::path input = requestedPath;
    if (std::filesystem::exists(input)) return input.string();

    std::filesystem::path fallback = std::filesystem::path("src") / input;
    if (std::filesystem::exists(fallback)) return fallback.string();

    std::filesystem::path defaultFile = std::filesystem::path("src") / "main.tekst";
    if (std::filesystem::exists(defaultFile)) return defaultFile.string();

    return requestedPath;
}

int main(int argc, char* argv[]) {
    bool debug = false;
    std::string filePath;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            std::cout << "Mini Python-like interpreter\n";
            std::cout << "Usage: parser [--debug] <source-file>\n";
            std::cout << "Example: parser --debug src\\main.tekst\n";
            return 0;
        }
        if (arg == "--debug" || arg == "-d") {
            debug = true;
        } else if (filePath.empty()) {
            filePath = arg;
        }
    }

    if (filePath.empty()) filePath = "src/main.tekst";

    std::string resolvedPath = resolveInputPath(filePath);
    std::ifstream file(resolvedPath);

    if (!file) {
        std::cerr << "Could not open file: " << filePath << "\n";
        std::cerr << "Usage: parser [--debug] <source-file>\n";
        return 1;
    }

    std::string source;
    std::string line;
    while (std::getline(file, line)) {
        source += line + '\n';
    }
    file.close();

    try {
        std::vector<Token> tokens = lexer(source);
        if (debug) printTokens(tokens);

        Parser parser(tokens);
        auto program = parser.parse();
        if (debug) {
            std::cout << "=== PARSED PROGRAM ===\n";
            std::cout << program->toString() << "\n";
        }

        Interpreter interpreter;
        interpreter.execute(program);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        std::cerr << "Hint: check the token stream with --debug and verify syntax like x = 5, print(x), or ClassName instanceName\n";
        return 1;
    }

    return 0;
}
