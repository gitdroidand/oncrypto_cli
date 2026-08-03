#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <string>
#include <optional>

// ============================================================
// Simple CLI Parser for testing (copied from main.cpp)
// ============================================================

struct Arguments {
    bool encrypt = false;
    bool decrypt = false;
    bool interactive = false;
    std::string input;
    bool isFile = false;
    std::string password;
    std::optional<std::string> outputFile;
    bool showHelp = false;
    bool showAlgorithm = true;
    bool quiet = false;
};

bool parseArguments(int argc, char* argv[], Arguments& args) {
    if (argc < 2) {
        args.interactive = true;
        return true;
    }

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            args.showHelp = true;
            return true;
        } else if (arg == "-i" || arg == "--interactive") {
            args.interactive = true;
        } else if (arg == "-q" || arg == "--quiet") {
            args.quiet = true;
        } else if (arg == "-text" && i + 1 < argc) {
            args.input = argv[++i];
            args.isFile = false;
        } else if (arg == "-file" && i + 1 < argc) {
            args.input = argv[++i];
            args.isFile = true;
        } else if (arg == "-key" && i + 1 < argc) {
            args.password = argv[++i];
        } else if (arg == "-encrypt") {
            args.encrypt = true;
        } else if (arg == "-decrypt") {
            args.decrypt = true;
        } else if (arg == "-out" && i + 1 < argc) {
            args.outputFile = argv[++i];
        } else if (arg == "-no-algo") {
            args.showAlgorithm = false;
        }
    }

    if (!args.interactive && args.input.empty()) {
        return false;
    }

    if (!args.interactive && args.password.empty()) {
        return false;
    }

    if (!args.encrypt && !args.decrypt && !args.interactive) {
        args.encrypt = true;
    }

    return true;
}

// ============================================================
// Tests
// ============================================================

TEST_CASE("CLI: Parse -text and -key") {
    const char* argv[] = {"oncrypto", "-text", "Hello", "-key", "pass"};
    int argc = 5;
    
    Arguments args;
    bool result = parseArguments(argc, (char**)argv, args);
    
    CHECK(result == true);
    CHECK(args.input == "Hello");
    CHECK(args.password == "pass");
    CHECK(args.isFile == false);
    CHECK(args.encrypt == true);
}

TEST_CASE("CLI: Parse -file and -key") {
    const char* argv[] = {"oncrypto", "-file", "secret.txt", "-key", "pass"};
    int argc = 5;
    
    Arguments args;
    bool result = parseArguments(argc, (char**)argv, args);
    
    CHECK(result == true);
    CHECK(args.input == "secret.txt");
    CHECK(args.password == "pass");
    CHECK(args.isFile == true);
}

TEST_CASE("CLI: Parse -decrypt flag") {
    const char* argv[] = {"oncrypto", "-text", "Hi", "-key", "pass", "-decrypt"};
    int argc = 6;
    
    Arguments args;
    bool result = parseArguments(argc, (char**)argv, args);
    
    CHECK(result == true);
    CHECK(args.decrypt == true);
    CHECK(args.encrypt == false);
}

TEST_CASE("CLI: Parse -out flag") {
    const char* argv[] = {"oncrypto", "-text", "Hi", "-key", "pass", "-out", "out.bin"};
    int argc = 7;
    
    Arguments args;
    bool result = parseArguments(argc, (char**)argv, args);
    
    CHECK(result == true);
    CHECK(args.outputFile.has_value());
    CHECK(args.outputFile.value() == "out.bin");
}

TEST_CASE("CLI: Parse -no-algo flag") {
    const char* argv[] = {"oncrypto", "-text", "Hi", "-key", "pass", "-no-algo"};
    int argc = 6;
    
    Arguments args;
    bool result = parseArguments(argc, (char**)argv, args);
    
    CHECK(result == true);
    CHECK(args.showAlgorithm == false);
}

TEST_CASE("CLI: Interactive mode when no args") {
    const char* argv[] = {"oncrypto"};
    int argc = 1;
    
    Arguments args;
    bool result = parseArguments(argc, (char**)argv, args);
    
    CHECK(result == true);
    CHECK(args.interactive == true);
}

TEST_CASE("CLI: Missing password fails") {
    const char* argv[] = {"oncrypto", "-text", "Hello"};
    int argc = 3;
    
    Arguments args;
    bool result = parseArguments(argc, (char**)argv, args);
    
    CHECK(result == false);
}

TEST_CASE("CLI: Missing input fails") {
    const char* argv[] = {"oncrypto", "-key", "pass"};
    int argc = 3;
    
    Arguments args;
    bool result = parseArguments(argc, (char**)argv, args);
    
    CHECK(result == false);
}

TEST_CASE("CLI: Quiet mode") {
    const char* argv[] = {"oncrypto", "-text", "Hi", "-key", "pass", "-q"};
    int argc = 6;
    
    Arguments args;
    bool result = parseArguments(argc, (char**)argv, args);
    
    CHECK(result == true);
    CHECK(args.quiet == true);
}
