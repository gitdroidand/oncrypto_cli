#include <iostream>
#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <fstream>
#include <chrono>
#include "CryptoRepository.hpp"
#include "utils/FileUtils.hpp"

using namespace crypto;

namespace Color {
    constexpr const char* RESET   = "\033[0m";
    constexpr const char* RED     = "\033[31m";
    constexpr const char* GREEN   = "\033[32m";
    constexpr const char* YELLOW  = "\033[33m";
    constexpr const char* BLUE    = "\033[34m";
    constexpr const char* MAGENTA = "\033[35m";
    constexpr const char* CYAN    = "\033[36m";
    constexpr const char* BOLD    = "\033[1m";
    constexpr const char* DIM     = "\033[2m";
}

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

void printBanner() {
    std::cout << Color::CYAN << R"(
  ___         ____                  _
 / _ \ _ __  / ___|_ __ _   _ _ __ | |_ ___
| | | | '_ \| |   | '__| | | | '_ \| __/ _ \
| |_| | | | | |___| |  | |_| | |_) | || (_) |
 \___/|_| |_|\____|_|   \__, | .__/ \__\___/
                        |___/|_|
)" << Color::RESET;
    std::cout << Color::BOLD << "OnCrypto v1.3.1" << Color::RESET;
    std::cout << Color::DIM << " - Smart Encryption Tool" << Color::RESET << "\n";
    std::cout << Color::DIM << "=====================================\n\n" << Color::RESET;
}

void printHelp() {
    printBanner();
    std::cout << Color::BOLD << "Usage:" << Color::RESET << " oncrypto [options]\n\n";
    std::cout << Color::BOLD << "Options:" << Color::RESET << "\n";
    std::cout << "  " << Color::CYAN << "-text" << Color::RESET << " STRING      Input text to encrypt/decrypt\n";
    std::cout << "  " << Color::CYAN << "-file" << Color::RESET << " PATH        Input file to encrypt/decrypt\n";
    std::cout << "  " << Color::CYAN << "-key" << Color::RESET << " PASSWORD     Password for encryption/decryption\n";
    std::cout << "  " << Color::CYAN << "-encrypt" << Color::RESET << "          Encrypt the input (default)\n";
    std::cout << "  " << Color::CYAN << "-decrypt" << Color::RESET << "          Decrypt the input\n";
    std::cout << "  " << Color::CYAN << "-out" << Color::RESET << " PATH         Save output to file\n";
    std::cout << "  " << Color::CYAN << "-i" << Color::RESET << ", " << Color::CYAN << "-interactive" << Color::RESET << "  Interactive mode\n";
    std::cout << "  " << Color::CYAN << "-q" << Color::RESET << ", " << Color::CYAN << "-quiet" << Color::RESET << "       Quiet mode (less output)\n";
    std::cout << "  " << Color::CYAN << "-no-algo" << Color::RESET << "         Don't show algorithm information\n";
    std::cout << "  " << Color::CYAN << "-h" << Color::RESET << ", " << Color::CYAN << "-help" << Color::RESET << "        Show this help\n\n";
    
    std::cout << Color::BOLD << "Smart Algorithm Selection:" << Color::RESET << "\n";
    std::cout << "  " << Color::YELLOW << "Small data (<1KB)" << Color::RESET << "    -> " << Color::GREEN << "XChaCha20-Poly1305" << Color::RESET << " (maximum security)\n";
    std::cout << "  " << Color::YELLOW << "Medium data" << Color::RESET << "          -> " << Color::GREEN << "ChaCha20-Poly1305" << Color::RESET << " (balanced)\n";
    std::cout << "  " << Color::YELLOW << "Large data (>1MB)" << Color::RESET << "    -> " << Color::GREEN << "AES-256-GCM" << Color::RESET << " (hardware accelerated)\n\n";
    
    std::cout << Color::BOLD << "Examples:" << Color::RESET << "\n";
    std::cout << "  oncrypto -text \"Hello\" -key \"mysecret\"\n";
    std::cout << "  oncrypto -file secret.txt -key \"pass123\" -out encrypted.bin\n";
    std::cout << "  oncrypto -file encrypted.bin -key \"pass123\" -decrypt\n";
    std::cout << "  oncrypto -i  # Interactive mode\n";
}

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
        std::cerr << Color::RED << "Error: No input provided\n" << Color::RESET;
        return false;
    }

    if (!args.interactive && args.password.empty()) {
        std::cerr << Color::RED << "Error: Password required (use -key)\n" << Color::RESET;
        return false;
    }

    if (!args.encrypt && !args.decrypt && !args.interactive) {
        args.encrypt = true;
    }

    return true;
}

void interactiveMode(Arguments& args) {
    std::cout << Color::CYAN << "\nInteractive Mode\n" << Color::RESET;
    std::cout << Color::DIM << "=================\n\n" << Color::RESET;

    std::string choice;
    std::cout << "Do you want to encrypt or decrypt? [e/d]: ";
    std::getline(std::cin, choice);
    args.encrypt = (choice == "e" || choice == "encrypt");
    args.decrypt = (choice == "d" || choice == "decrypt");

    if (!args.encrypt && !args.decrypt) {
        args.encrypt = true;
    }

    std::cout << "Input from text or file? [t/f]: ";
    std::getline(std::cin, choice);
    args.isFile = (choice == "f" || choice == "file");

    if (args.isFile) {
        std::cout << "Enter file path: ";
        std::getline(std::cin, args.input);
    } else {
        std::cout << "Enter text: ";
        std::getline(std::cin, args.input);
    }

    std::cout << "Enter password: ";
    std::getline(std::cin, args.password);

    std::cout << "Save output to file? [y/n]: ";
    std::getline(std::cin, choice);
    if (choice == "y" || choice == "yes") {
        std::cout << "Output file path: ";
        std::string outPath;
        std::getline(std::cin, outPath);
        args.outputFile = outPath;
    }

    std::cout << "\n" << Color::GREEN << "Starting operation...\n" << Color::RESET;
}

void showProgress(size_t current, size_t total, const std::string& label = "") {
    const int barWidth = 40;
    float progress = static_cast<float>(current) / total;
    int pos = static_cast<int>(barWidth * progress);

    std::cout << "\r" << Color::CYAN << label << " ";
    std::cout << "[";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100) << "%" << Color::RESET;
    std::cout.flush();
}

int main(int argc, char* argv[]) {
    Arguments args;
    if (!parseArguments(argc, argv, args)) {
        return 1;
    }

    if (args.showHelp) {
        printHelp();
        return 0;
    }

    if (args.interactive) {
        interactiveMode(args);
    }

    if (args.input.empty()) {
        std::cerr << Color::RED << "Error: No input provided\n" << Color::RESET;
        return 1;
    }

    if (args.password.empty()) {
        std::cerr << Color::RED << "Error: Password required\n" << Color::RESET;
        return 1;
    }

    try {
        CryptoRepository repo;
        std::vector<unsigned char> inputData;

        if (!args.quiet) {
            std::cout << Color::CYAN << "Reading input...\n" << Color::RESET;
        }

        if (args.isFile) {
            inputData = readFile(args.input);
            if (inputData.empty()) {
                std::cerr << Color::RED << "Error: Could not read file '" << args.input << "'\n" << Color::RESET;
                return 1;
            }
        } else {
            inputData.assign(args.input.begin(), args.input.end());
        }

        if (!args.quiet) {
            std::cout << Color::GREEN << "Read " << inputData.size() << " bytes\n" << Color::RESET;
            std::cout << Color::CYAN << "Processing...\n" << Color::RESET;
        }

        auto start = std::chrono::high_resolution_clock::now();

        if (args.encrypt) {
            auto result = repo.encrypt(inputData, args.password);

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - start);

            if (!args.quiet) {
                std::cout << Color::GREEN << "Encryption completed in " << duration.count() << "ms\n" << Color::RESET;
            }

            if (args.showAlgorithm && !args.quiet) {
                std::cout << Color::MAGENTA << "Algorithm: " << result.algorithmName << " - " << result.reason << Color::RESET << "\n";
                std::cout << Color::DIM << "Salt: " << bytesToHex(result.salt) << Color::RESET << "\n";
                std::cout << Color::YELLOW << "Data size: " << inputData.size() << " bytes -> " << result.data.size() << " bytes\n" << Color::RESET;
            }

            if (args.outputFile) {
                if (writeFile(*args.outputFile, result.data)) {
                    if (!args.quiet) {
                        std::cout << Color::GREEN << "Saved to: " << *args.outputFile << "\n" << Color::RESET;
                    }
                } else {
                    std::cerr << Color::RED << "Error: Could not write to file\n" << Color::RESET;
                    return 1;
                }
            } else if (!args.quiet) {
                std::cout << Color::CYAN << "Encrypted data (hex):\n" << Color::RESET;
                std::cout << bytesToHex(result.data) << '\n';
            } else if (!args.outputFile) {
                std::cout << bytesToHex(result.data) << '\n';
            }

        } else if (args.decrypt) {
            auto decrypted = repo.decrypt(inputData, args.password);

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - start);

            if (!args.quiet) {
                std::cout << Color::GREEN << "Decryption completed in " << duration.count() << "ms\n" << Color::RESET;
            }

            if (args.outputFile) {
                if (writeFile(*args.outputFile, decrypted)) {
                    if (!args.quiet) {
                        std::cout << Color::GREEN << "Decrypted saved to: " << *args.outputFile << "\n" << Color::RESET;
                    }
                } else {
                    std::cerr << Color::RED << "Error: Could not write to file\n" << Color::RESET;
                    return 1;
                }
            } else {
                std::string text(decrypted.begin(), decrypted.end());
                if (!args.quiet) {
                    std::cout << Color::CYAN << "Decrypted text:\n" << Color::RESET;
                }
                std::cout << text << '\n';
            }
        }

    } catch (const std::exception& ex) {
        std::cerr << Color::RED << "Error: " << ex.what() << "\n" << Color::RESET;
        return 1;
    }

    if (!args.quiet) {
        std::cout << Color::GREEN << "\nDone!\n" << Color::RESET;
    }

    return 0;
}
