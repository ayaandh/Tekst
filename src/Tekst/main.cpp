#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <regex>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include "lexer.h"
#include "parser.h"

namespace fs = std::filesystem;

static constexpr const char* TK_REGISTRY = "https://tekst.ayaan.is-a.dev/packages";

static std::string readFile(const fs::path& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Could not open file: " + path.string());
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static void writeFile(const fs::path& path, const std::string& content) {
    if (!path.parent_path().empty()) fs::create_directories(path.parent_path());
    std::ofstream file(path);
    if (!file) throw std::runtime_error("Could not write file: " + path.string());
    file << content;
}

static std::string shellQuote(const std::string& value) {
#ifdef _WIN32
    std::string out = "\"";
    for (char c : value) {
        if (c == '"') out += "\\\"";
        else out += c;
    }
    out += "\"";
    return out;
#else
    std::string out = "'";
    for (char c : value) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
#endif
}

static bool validPackageName(const std::string& name) {
    if (name.empty()) return false;
    return std::all_of(name.begin(), name.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '-' || c == '_' || c == '.';
    });
}

static bool downloadFile(const std::string& url, const fs::path& destination) {
    fs::create_directories(destination.parent_path());
#ifdef _WIN32
    std::string command = "curl.exe -fL --silent --show-error -o " + shellQuote(destination.string()) + " " + shellQuote(url);
#else
    std::string command = "curl -fL --silent --show-error -o " + shellQuote(destination.string()) + " " + shellQuote(url);
#endif
    return std::system(command.c_str()) == 0;
}

static void printTkHelp() {
    std::cout << "tk - Tekst package manager\n\n";
    std::cout << "Usage:\n";
    std::cout << "  tk init                    Create tekst.toml\n";
    std::cout << "  tk new <name>              Create a new project\n";
    std::cout << "  tk install                 Install dependencies from tekst.toml\n";
    std::cout << "  tk install <package>       Install a package\n";
    std::cout << "  tk remove <package>        Remove a package\n";
    std::cout << "  tk list                    List installed packages\n";
    std::cout << "  tk search <query>          Search the package registry\n";
    std::cout << "  tk update                  Reinstall dependencies\n";
}

static fs::path findProjectFile(const fs::path& start, const std::string& name) {
    fs::path current = fs::absolute(start);
    while (true) {
        fs::path candidate = current / name;
        if (fs::exists(candidate)) return candidate;
        if (current == current.root_path()) break;
        current = current.parent_path();
    }
    return {};
}

static void tkInit() {
    fs::path path = fs::current_path() / "tekst.toml";
    if (fs::exists(path)) {
        std::cout << "tekst.toml already exists.\n";
        return;
    }
    writeFile(path,
        "name = \"my-tekst-project\"\n"
        "version = \"0.1.0\"\n\n"
        "[dependencies]\n");
    writeFile(fs::current_path() / "src" / "main.tk",
        "print(\"Hello from Tekst!\")\n");
    std::cout << "Created tekst.toml and src/main.tk\n";
}

static void tkNew(const std::string& name) {
    if (!validPackageName(name)) throw std::runtime_error("Invalid project name");
    fs::path root = fs::current_path() / name;
    if (fs::exists(root)) throw std::runtime_error("Directory already exists: " + root.string());
    fs::create_directories(root / "src");
    writeFile(root / "tekst.toml",
        "name = \"" + name + "\"\nversion = \"0.1.0\"\n\n[dependencies]\n");
    writeFile(root / "src" / "main.tk", "print(\"Hello from " + name + "!\")\n");
    std::cout << "Created " << root.string() << "\n";
}

static std::vector<std::pair<std::string, std::string>> readDependencies(const fs::path& toml) {
    std::vector<std::pair<std::string, std::string>> result;
    std::istringstream input(readFile(toml));
    std::string line;
    bool deps = false;
    while (std::getline(input, line)) {
        if (line.find("[dependencies]") != std::string::npos) { deps = true; continue; }
        if (!deps || line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string name = line.substr(0, eq);
        std::string version = line.substr(eq + 1);
        auto trim = [](std::string x) {
            auto a = x.find_first_not_of(" \t\r\n\"");
            auto b = x.find_last_not_of(" \t\r\n\"");
            return a == std::string::npos ? std::string{} : x.substr(a, b - a + 1);
        };
        name = trim(name); version = trim(version);
        if (validPackageName(name)) result.push_back({name, version});
    }
    return result;
}

static std::string resolveLatestVersion(const std::string& package, const std::string& requested) {
    if (requested != "latest" && !requested.empty()) return requested;
    fs::path index = fs::current_path() / ".tk-index.json";
    std::string url = std::string(TK_REGISTRY) + "/index.json";
    if (!downloadFile(url, index)) throw std::runtime_error("Could not reach package registry");
    std::string json = readFile(index);
    fs::remove(index);
    std::regex pattern("\\\"" + package + "\\\"\\s*:\\s*\\{[^}]*\\\"latest\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) throw std::runtime_error("Package not found: " + package);
    return match[1].str();
}

static void tkInstallOne(const fs::path& project, const std::string& package, const std::string& requestedVersion = "latest") {
    if (!validPackageName(package)) throw std::runtime_error("Invalid package name: " + package);
    fs::path packages = project / "packages" / package;
    fs::create_directories(packages);
    std::string version = resolveLatestVersion(package, requestedVersion.empty() ? "latest" : requestedVersion);
    std::string archiveName = package + "-" + version + ".zip";
    fs::path archive = packages / archiveName;
    std::string url = std::string(TK_REGISTRY) + "/" + package + "/" + version + "/" + archiveName;
    std::cout << "Installing " << package << " " << version << "...\n";
    if (!downloadFile(url, archive)) {
        fs::remove(archive);
        throw std::runtime_error("Could not download package from " + url);
    }
    std::string command = "python -c \"import zipfile,sys; zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])\" " + shellQuote(archive.string()) + " " + shellQuote(packages.string());
    if (std::system(command.c_str()) != 0) throw std::runtime_error("Could not extract package " + package);
    fs::remove(archive);
    std::ofstream lock(project / "tk.lock", std::ios::app);
    lock << package << " = \"" << version << "\"\n";
    std::cout << "Installed " << package << " " << version << "\n";
}

static void tkInstall(const std::string& package) {
    fs::path toml = findProjectFile(fs::current_path(), "tekst.toml");
    if (toml.empty()) throw std::runtime_error("No tekst.toml found");
    fs::path project = toml.parent_path();
    if (!package.empty()) {
        tkInstallOne(project, package);
        std::string tomlText = readFile(toml);
        if (tomlText.find(package + " =") == std::string::npos) {
            if (tomlText.find("[dependencies]") == std::string::npos) tomlText += "\n[dependencies]\n";
            tomlText += package + " = \"latest\"\n";
            writeFile(toml, tomlText);
        }
        return;
    }
    for (const auto& [name, version] : readDependencies(toml)) tkInstallOne(project, name, version);
}

static void tkRemove(const std::string& package) {
    if (!validPackageName(package)) throw std::runtime_error("Invalid package name");
    fs::path toml = findProjectFile(fs::current_path(), "tekst.toml");
    if (toml.empty()) throw std::runtime_error("No tekst.toml found");
    fs::path dir = toml.parent_path() / "packages" / package;
    if (!fs::exists(dir)) throw std::runtime_error("Package is not installed: " + package);
    fs::remove_all(dir);
    std::cout << "Removed " << package << "\n";
}

static void tkList() {
    fs::path toml = findProjectFile(fs::current_path(), "tekst.toml");
    if (toml.empty()) throw std::runtime_error("No tekst.toml found");
    fs::path dir = toml.parent_path() / "packages";
    if (!fs::exists(dir)) { std::cout << "No packages installed.\n"; return; }
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_directory()) std::cout << entry.path().filename().string() << "\n";
    }
}

static void tkSearch(const std::string& query) {
    fs::path index = fs::current_path() / ".tk-index.json";
    std::string url = std::string(TK_REGISTRY) + "/index.json";
    if (!downloadFile(url, index)) throw std::runtime_error("Could not reach package registry");
    std::string json = readFile(index);
    fs::remove(index);
    std::regex item(R"(\"([A-Za-z0-9_.-]+)\"\s*:\s*\{[^}]*\})");
    bool found = false;
    for (auto it = std::sregex_iterator(json.begin(), json.end(), item); it != std::sregex_iterator(); ++it) {
        std::string name = (*it)[1].str();
        if (query.empty() || name.find(query) != std::string::npos) { std::cout << name << "\n"; found = true; }
    }
    if (!found) std::cout << "No packages found.\n";
}

static int runTk(const std::vector<std::string>& args) {
    if (args.empty() || args[0] == "help" || args[0] == "--help" || args[0] == "-h") { printTkHelp(); return 0; }
    const std::string& command = args[0];
    if (command == "init") tkInit();
    else if (command == "new" && args.size() >= 2) tkNew(args[1]);
    else if (command == "install") tkInstall(args.size() >= 2 ? args[1] : "");
    else if (command == "remove" && args.size() >= 2) tkRemove(args[1]);
    else if (command == "list") tkList();
    else if (command == "search") tkSearch(args.size() >= 2 ? args[1] : "");
    else if (command == "update") tkInstall("");
    else { printTkHelp(); return 1; }
    return 0;
}

static void printTokens(const std::vector<Token>& tokens) {
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
            case TokenType::BACKSLASH: std::cout << "BACKSLASH"; break;
        }
        std::cout << " -> '" << token.value << "'\n";
    }
}

int main(int argc, char* argv[]) {
    try {
        std::string executable = fs::path(argv[0]).stem().string();
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);
        if (executable == "tk" || (!args.empty() && args[0] == "tk")) {
            if (executable != "tk" && !args.empty()) args.erase(args.begin());
            return runTk(args);
        }

        bool debug = false;
        std::string filePath;
        for (const auto& arg : args) {
            if (arg == "--help" || arg == "-h") {
                std::cout << "Tekst interpreter\nUsage: tekst [--debug] <source-file>\n";
                return 0;
            }
            if (arg == "--debug" || arg == "-d") debug = true;
            else if (filePath.empty()) filePath = arg;
        }
        if (filePath.empty()) filePath = "src/main.tk";
        fs::path path = fs::absolute(filePath);
        if (!fs::exists(path)) {
            fs::path fallback = fs::absolute(fs::path("src") / filePath);
            if (fs::exists(fallback)) path = fallback;
        }
        if (!fs::exists(path)) throw std::runtime_error("Could not open file: " + path.string());
        std::string source = readFile(path);
        auto tokens = lexer(source);
        if (debug) printTokens(tokens);
        Parser parser(tokens);
        auto program = parser.parse();
        if (debug) std::cout << "=== PARSED PROGRAM ===\n" << program->toString() << '\n';
        Interpreter interpreter;
        interpreter.execute(program, path.parent_path());
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}






