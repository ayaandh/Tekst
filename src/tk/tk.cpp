#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

static const std::string REGISTRY_OWNER = "ayaandh";
static const std::string REGISTRY_REPO = "tkpackages";
static const std::string REGISTRY =
    "https://github.com/" + REGISTRY_OWNER + "/" + REGISTRY_REPO;

std::string localAppData() {
    const char* value = std::getenv("LOCALAPPDATA");

    if (!value)
        return ".";

    return value;
}

fs::path packageRoot() {
    return fs::path(localAppData()) / "Tekst" / "packages";
}

std::string lower(std::string value) {
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        }
    );

    return value;
}

bool validPackageName(const std::string& name) {
    if (name.empty())
        return false;

    for (unsigned char c : name) {
        if (!std::isalnum(c) && c != '-' && c != '_' && c != '.')
            return false;
    }

    return true;
}

bool downloadRegistry(const fs::path& destination) {
    fs::path zip = destination / "tkpackages.zip";
    fs::path temp = destination / "tkpackages_tmp";

    fs::create_directories(destination);

    fs::remove(zip);
    fs::remove_all(temp);

    std::string url =
        REGISTRY + "/archive/refs/heads/main.zip";

    std::string download =
        "powershell -NoProfile -ExecutionPolicy Bypass -Command "
        "\"Invoke-WebRequest -Uri '" + url +
        "' -OutFile '" + zip.string() + "'\"";

    if (std::system(download.c_str()) != 0) {
        fs::remove(zip);
        return false;
    }

    fs::create_directories(temp);

    std::string extract =
        "powershell -NoProfile -ExecutionPolicy Bypass -Command "
        "\"Expand-Archive -Path '" + zip.string() +
        "' -DestinationPath '" + temp.string() + "' -Force\"";

    if (std::system(extract.c_str()) != 0) {
        fs::remove(zip);
        fs::remove_all(temp);
        return false;
    }

    fs::path extracted =
        temp / (REGISTRY_REPO + "-main");

    if (!fs::exists(extracted)) {
        fs::remove(zip);
        fs::remove_all(temp);
        return false;
    }

    fs::remove_all(destination / "repository");

    fs::rename(
        extracted,
        destination / "repository"
    );

    fs::remove(zip);
    fs::remove_all(temp);

    return true;
}

bool packageExistsInRegistry(const std::string& name) {
    std::string url =
        "https://api.github.com/repos/" +
        REGISTRY_OWNER + "/" +
        REGISTRY_REPO +
        "/contents/" + name;

    std::string command =
        "powershell -NoProfile -ExecutionPolicy Bypass -Command "
        "\"try { "
        "$r=Invoke-WebRequest -Uri '" + url +
        "' -Headers @{ 'User-Agent'='tk' } -UseBasicParsing; "
        "if ($r.StatusCode -eq 200) { exit 0 } else { exit 1 } "
        "} catch { exit 1 }\"";

    return std::system(command.c_str()) == 0;
}

bool downloadPackage(
    const std::string& name,
    const fs::path& destination
) {
    fs::path work =
        destination.parent_path() / ".tk_registry";

    fs::remove_all(work);
    fs::create_directories(work);

    if (!downloadRegistry(work)) {
        fs::remove_all(work);
        return false;
    }

    fs::path package =
        work / "repository" / name;

    if (!fs::exists(package) || !fs::is_directory(package)) {
        fs::remove_all(work);
        return false;
    }

    fs::remove_all(destination);

    fs::copy(
        package,
        destination,
        fs::copy_options::recursive |
        fs::copy_options::overwrite_existing
    );

    fs::remove_all(work);

    return true;
}

void installPackage(const std::string& name) {
    if (!validPackageName(name)) {
        std::cout
            << "Invalid package name: "
            << name
            << ".\n";

        return;
    }

    fs::path destination =
        packageRoot() / name;

    if (fs::exists(destination)) {
        std::cout
            << "Package '"
            << name
            << "' is already installed.\n";

        return;
    }

    std::cout
        << "Checking "
        << REGISTRY_OWNER
        << "/"
        << REGISTRY_REPO
        << "/"
        << name
        << "...\n";

    if (!packageExistsInRegistry(name)) {
        std::cout
            << "Package '"
            << name
            << "' was not found.\n";

        return;
    }

    std::cout
        << "Downloading "
        << name
        << "...\n";

    fs::create_directories(
        destination.parent_path()
    );

    if (!downloadPackage(name, destination)) {
        std::cout
            << "Failed to install '"
            << name
            << "'.\n";

        return;
    }

    std::cout
        << "Installed "
        << name
        << ".\n";
}

void removePackage(const std::string& name) {
    fs::path destination =
        packageRoot() / name;

    if (!fs::exists(destination)) {
        std::cout
            << "Package '"
            << name
            << "' is not installed.\n";

        return;
    }

    fs::remove_all(destination);

    std::cout
        << "Removed "
        << name
        << ".\n";
}

void listPackages() {
    fs::path root = packageRoot();

    if (!fs::exists(root)) {
        std::cout
            << "No packages installed.\n";

        return;
    }

    bool found = false;

    for (const auto& entry :
         fs::directory_iterator(root)) {

        if (entry.is_directory()) {
            std::cout
                << entry.path()
                       .filename()
                       .string()
                << '\n';

            found = true;
        }
    }

    if (!found)
        std::cout
            << "No packages installed.\n";
}

void searchPackages(const std::string& query) {
    std::string url =
        "https://api.github.com/repos/" +
        REGISTRY_OWNER + "/" +
        REGISTRY_REPO +
        "/git/trees/main?recursive=1";

    std::string command =
        "powershell -NoProfile -ExecutionPolicy Bypass -Command "
        "\"try { "
        "$r=Invoke-RestMethod "
        "-Uri '" + url + "' "
        "-Headers @{ 'User-Agent'='tk' }; "
        "$r.tree | "
        "Where-Object { "
        "$_.type -eq 'tree' -and "
        "$_.path -notlike '*/*' -and "
        "$_.path -like '*" + query + "*' "
        "} | "
        "Select-Object -ExpandProperty path "
        "} catch { exit 1 }\"";

    if (std::system(command.c_str()) != 0) {
        std::cout
            << "Failed to search package registry.\n";
    }
}

void initProject() {
    fs::path current =
        fs::current_path();

    fs::path manifest =
        current / "tk.toml";

    if (fs::exists(manifest)) {
        std::cout
            << "tk.toml already exists.\n";

        return;
    }

    std::ofstream file(manifest);

    file << "name = \""
         << current.filename().string()
         << "\"\n";

    file << "version = \"0.1.0\"\n\n";

    file << "[dependencies]\n";

    fs::create_directories(
        current / "src"
    );

    std::cout
        << "Created tk.toml\n";

    std::cout
        << "Created src/\n";
}

void printUsage() {
    std::cout
        << "Tekst Package Manager\n\n"
        << "Usage:\n"
        << "  tk install <package>\n"
        << "  tk remove <package>\n"
        << "  tk search <query>\n"
        << "  tk list\n"
        << "  tk init\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string command =
        lower(argv[1]);

    if (command == "install") {
        if (argc < 3) {
            std::cout
                << "Missing package name.\n";

            return 1;
        }

        installPackage(argv[2]);
        return 0;
    }

    if (command == "remove") {
        if (argc < 3) {
            std::cout
                << "Missing package name.\n";

            return 1;
        }

        removePackage(argv[2]);
        return 0;
    }

    if (command == "search") {
        if (argc < 3) {
            std::cout
                << "Missing search query.\n";

            return 1;
        }

        searchPackages(argv[2]);
        return 0;
    }

    if (command == "list") {
        listPackages();
        return 0;
    }

    if (command == "init") {
        initProject();
        return 0;
    }

    if (command == "help") {
        printUsage();
        return 0;
    }

    std::cout
        << "Unknown command: "
        << command
        << '\n';

    printUsage();

    return 1;
}