#include <iostream>
#include <format>
#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <fstream>
#include "CryptoRepository.hpp"
#include "utils/FileUtils.hpp"

using namespace crypto;

struct Arguments {
    bool encrypt = false;
    bool decrypt = false;
    std::string input;
    bool isFile = false;
    std::string password;
    std::optional<std::string> outputFile;
    bool showHelp = false;
    bool showAlgorithm = true;
};

void printHelp() {
    std::cout << std::format(
        "OnCrypto v1.2 - Smart Encryption Tool\n"
        "=====================================\n"
        "Usage:\n"
        "  oncrypto -text \"text\" -key \"password\" [options]\n"
        "  oncrypto -file path/to/file -key \"password\" [options]\n"
        "\n"
        "Options:\n"
        "  -text STRING      Input text to encrypt/decrypt\n"
        "  -file PATH        Input file to encrypt/decrypt\n"
        "  -key PASSWORD     Password for encryption/decryption\n"
        "  -encrypt          Encrypt the input (default)\n"
        "  -decrypt          Decrypt the input\n"
        "  -out PATH         Save output to file\n"
        "  -no-algo          Don't show algorithm information\n"
        "  -h, --help        Show this help\n"
        "\n"
        "Smart Algorithm Selection:\n"
        "  - Small data (<1KB)    -> XChaCha20-Poly1305 (maximum security)\n"
        "  - Medium data          -> ChaCha20-Poly1305 (balanced)\n"
        "  - Large data (>1MB)    -> AES-256-GCM (hardware accelerated)\n"
        "\n"
        "Examples:\n"
        "  oncrypto -text \"Hello\" -key \"mysecret\"\n"
        "  oncrypto -file secret.txt -key \"pass123\" -out encrypted.bin\n"
        "  oncrypto -file encrypted.bin -key \"pass123\" -decrypt -out decrypted.txt\n"
    );
}

bool parseArguments(int argc, char* argv[], Arguments& args) {
    if (argc < 2) {
        args.showHelp = true;
        return true;
    }

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            args.showHelp = true;
            return true;
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

    if (args.input.empty()) {
        std::cerr << "Error: No input provided\n";
        return false;
    }

    if (args.password.empty()) {
        std::cerr << "Error: Password required (use -key)\n";
        return false;
    }

    if (!args.encrypt && !args.decrypt) {
        args.encrypt = true;
    }

    return true;
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

    try {
        CryptoRepository repo;
        std::vector<unsigned char> inputData;

        if (args.isFile) {
            inputData = readFile(args.input);
            if (inputData.empty()) {
                std::cerr << std::format("Error: Could not read file '{}'\n", args.input);
                return 1;
            }
        } else {
            inputData.assign(args.input.begin(), args.input.end());
        }

        if (args.encrypt) {
            auto result = repo.encrypt(inputData, args.password);
            
            if (args.showAlgorithm) {
                std::cout << std::format(
                    "🔐 Algorithm: {} - {}\n",
                    result.algorithmName,
                    result.reason
                );
                std::cout << std::format("🧂 Salt: {}\n", bytesToHex(result.salt));
                std::cout << std::format("📊 Data size: {} bytes -> {} bytes\n",
                    inputData.size(),
                    result.data.size()
                );
            }

            if (args.outputFile) {
                if (writeFile(*args.outputFile, result.data)) {
                    std::cout << std::format("✅ Saved to: {}\n", *args.outputFile);
                } else {
                    std::cerr << "Error: Could not write to file\n";
                    return 1;
                }
            } else {
                std::cout << "📝 Encrypted data (hex):\n";
                std::cout << bytesToHex(result.data) << '\n';
            }

        } else if (args.decrypt) {
            auto decrypted = repo.decrypt(inputData, args.password);
            
            if (args.outputFile) {
                if (writeFile(*args.outputFile, decrypted)) {
                    std::cout << std::format("✅ Decrypted saved to: {}\n", *args.outputFile);
                } else {
                    std::cerr << "Error: Could not write to file\n";
                    return 1;
                }
            } else {
                std::string text(decrypted.begin(), decrypted.end());
                std::cout << "📝 Decrypted text:\n";
                std::cout << text << '\n';
            }
        }

    } catch (const std::exception& ex) {
        std::cerr << std::format("❌ Error: {}\n", ex.what());
        return 1;
    }

    return 0;
}