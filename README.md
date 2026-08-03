🔐 OnCrypto

<div align="center">

https://img.shields.io/badge/version-1.6.0-blue.svg
https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus
https://img.shields.io/badge/CMake-3.20+-064F8C?logo=cmake
https://img.shields.io/badge/Ninja-Build-black
https://img.shields.io/badge/License-MIT-yellow.svg
https://img.shields.io/badge/Platform-Linux%20%7C%20Windows%20%7C%20macOS%20%7C%20Android-success

Modern Cross-Platform Encryption Library & CLI built with C++20

</div>

---

📚 Table of Contents

· Introduction
· Features
· Supported Algorithms
· Automatic Algorithm Selection
· Architecture
· Project Structure
· Requirements
· Installation
· CLI Usage
· Library Usage
· Streaming API
· Running Tests
· Version History
· License
· Contact

---

📖 Introduction

OnCrypto is a modern, lightweight, and high-performance cross-platform encryption library written in C++20.

It provides both:

· 🖥 Command Line Interface (CLI)
· 📚 Shared and Static Libraries (liboncrypto.so / liboncrypto.a / oncrypto.dll)

The library features a decoupled, backend-agnostic design with high-security authenticated encryption algorithms and automatic algorithm selection.

🎯 Design Philosophy

· Simple API – Just encrypt() and decrypt()
· Smart Defaults – Auto-selects the optimal algorithm for your payload size
· Modular Backend – Crypto backend is completely isolated via an abstracted C ABI interface (oncrypto_engine)
· Self-Hosting Evolution – Transitioned to an independent core model powered by liboncrypto
· Cross-Platform – Native support across Linux, macOS, Windows, and Android
· Production Ready – Fully unit-tested with 1052+ assertions

---

✨ Features

Feature Status
🔐 AES-256-GCM ✅
⚡ ChaCha20-Poly1305 ✅
🚀 XChaCha20-Poly1305 ✅
🤖 Automatic Algorithm Selection ✅
🔑 PBKDF2-HMAC-SHA256 Key Derivation ✅
🧂 16-byte Random Salt ✅
🔄 Shared Library (liboncrypto.so) ✅
📦 Static Library (liboncrypto.a) ✅
💻 Interactive & Direct CLI Tool ✅
🌊 Streaming Support (Large Files) ✅
📦 OnC Binary Format (ONC1) ✅
🧪 Unit Tests (doctest Integration) ✅
🌍 Cross Platform ✅
⚙ CMake + Ninja Build System ✅

---

🔐 Supported Algorithms

Algorithm Security Speed Best For
XChaCha20-Poly1305 ⭐⭐⭐⭐⭐ ⭐⭐⭐⭐⭐ Small data (<1KB)
ChaCha20-Poly1305 ⭐⭐⭐⭐⭐ ⭐⭐⭐⭐☆ Medium files (1KB - 1MB)
AES-256-GCM ⭐⭐⭐⭐⭐ ⭐⭐⭐⭐⭐ Large files (>1MB)

---

🤖 Automatic Algorithm Selection

OnCrypto dynamically evaluates the payload size to pick the safest and most efficient algorithm for the task:

· Payload < 1 KB: Uses XChaCha20-Poly1305 (Optimized for short strings, keys, or metadata with extended nonce size)
· 1 KB ≤ Payload ≤ 1 MB: Uses ChaCha20-Poly1305 (Balanced performance for general data)
· Payload > 1 MB: Uses AES-256-GCM (Leverages hardware acceleration for bulk processing)

---

🏗 Architecture

```text
[ Application / CLI ]
         │
         ▼
[ Public C++ SDK Layer ] (crypto:: namespace, Streaming, Builder)
         │
         ▼
[ Core Engine / CryptoRepository ]
         │
         ▼
[ C ABI Engine Layer ] (oncrypto_engine.h)
         │
         ▼
[ Backend Implementation ] (OpenSSL)

```

Note: OpenSSL serves as the internal backend implementation and is not exposed through the public API.

---

📂 Project Structure

```text
oncrypto_cli/
├── CMakeLists.txt         # Root build script
├── include/               # Engine C ABI public header
│   └── oncrypto_engine.h
├── core/                  # Core Library implementation
│   ├── include/           # C++ SDK headers (crypto:: namespace)
│   └── src/               # Core repository, backend & format engines
├── cli/                   # Command line interface application
│   └── src/main.cpp
├── tests/                 # Unit tests (doctest)
│   ├── oncrypto_test      # Library tests
│   └── oncrypto_cli_test  # CLI tests
├── docs/                  # Detailed architectural and API documentation
└── examples/              # Usage examples

```

---

⚙ Requirements

· C++ Compiler: Modern compiler with C++20 support (GCC 10+, Clang 12+, MSVC 2019+)
· CMake: Version 3.20 or higher
· Build System: Ninja (recommended), Make, or MSVC/IDE generators

---

🛠 Installation & Building

```bash
# Clone the repository
git clone https://github.com/gitdroidand/oncrypto_cli.git
cd oncrypto_cli

# Create build directory
mkdir build && cd build

# Configure and build
cmake -G Ninja ..
ninja

# Build outputs:
# - liboncrypto.so / liboncrypto.a
# - oncrypto_cli
# - oncrypto_test
# - oncrypto_cli_test
```

---

💻 CLI Usage

OnCrypto CLI supports both interactive mode and direct command arguments.

Interactive Mode

Launch the interactive prompt:

```bash
./oncrypto_cli -i
```

Command Line Arguments

```
Usage:
  oncrypto_cli [options]

Options:
  -text STRING
  -file PATH
  -key PASSWORD
  -encrypt
  -decrypt
  -out PATH
  -i
  -interactive
  -q
  -quiet
  -no-algo
  -h
  -help
```

Examples

```bash
# Encrypt text
./oncrypto_cli -text "Hello" -key "secret" -encrypt

# Encrypt file
./oncrypto_cli -file secret.txt -key pass123 -out encrypted.bin

# Decrypt file
./oncrypto_cli -file encrypted.bin -key pass123 -decrypt

# Interactive mode
./oncrypto_cli -i
```

---

📚 Library Usage

High-Level API (C++)

```cpp
#include <oncrypto/oncrypto.hpp>
#include <iostream>
#include <vector>

int main() {
    std::string plaintext = "Hello, OnCrypto 1.6!";
    std::string password = "MySecurePassword123";

    // Auto-selected encryption
    std::vector<uint8_t> ciphertext = crypto::encrypt(plaintext, password);

    // Decryption
    std::string decrypted = crypto::decrypt(ciphertext, password);
    
    std::cout << "Decrypted: " << decrypted << "\n";

    // Get version info
    std::cout << "Version: " << crypto::getVersion() << "\n";
    std::cout << "Algorithm: " << crypto::getAlgorithmName() << "\n";
    
    return 0;
}
```

Builder API

```cpp
#include <oncrypto/oncrypto.hpp>

// Encryption using builder pattern
crypto::builder::Encryptor encryptor;
encryptor.setPlaintext(plaintext);
encryptor.setPassword(password);
std::vector<uint8_t> ciphertext = encryptor.encrypt();

// Decryption using builder pattern
crypto::builder::Decryptor decryptor;
decryptor.setCiphertext(ciphertext);
decryptor.setPassword(password);
std::string decrypted = decryptor.decrypt();
```

Advanced API

```cpp
#include <oncrypto/oncrypto.hpp>

// Explicit algorithm selection (optional)
std::vector<uint8_t> ciphertext = crypto::advanced::encrypt(
    plaintext, password, crypto::Algorithm::AES_256_GCM
);

std::string decrypted = crypto::advanced::decrypt(
    ciphertext, password, crypto::Algorithm::AES_256_GCM
);
```

File Encryption

```cpp
#include <oncrypto/oncrypto.hpp>

// Encrypt file
crypto::encryptFile("input.txt", "output.onc", password);

// Decrypt file
crypto::decryptFile("output.onc", "decrypted.txt", password);
```

---

🌊 Streaming API

For streaming large files efficiently without loading everything into memory:

```cpp
#include <oncrypto/Streaming.hpp>
#include <fstream>

// Encryption stream
std::ifstream input("large_file.bin", std::ios::binary);
std::ofstream output("large_file.bin.onc", std::ios::binary);
onc::streaming::EncryptStream encryptor(input, output, "password");
encryptor.process();

// Decryption stream
std::ifstream encrypted("large_file.bin.onc", std::ios::binary);
std::ofstream decrypted("large_file_decrypted.bin", std::ios::binary);
onc::streaming::DecryptStream decryptor(encrypted, decrypted, "password");
decryptor.process();
```

---

🧪 Running Tests

OnCrypto utilizes doctest for its test suite:

```bash
# Run all tests
cd build
./oncrypto_test
./oncrypto_cli_test

# Or use CTest
ctest --output-on-failure
```

Current test coverage: 14 test cases with 1052 assertions - all passing.

---

📜 Version History

v1.6.0

· Core Decoupling & Independence: Completely rewrote core logic to rely directly on liboncrypto via an abstracted C ABI layer
· Enhanced memory handling and backend isolation

v1.5.0

· Added Streaming API support for ultra-large files
· Introduced OnCFormat binary layout specification (ONC1)

v1.0.0

· Initial release with AEAD algorithm support and automatic selection

---

📄 License

This project is licensed under the MIT License.

---

📬 Contact & Support

· Email: droidandsoftwaresinc@gmail.com
· Telegram: @droidand_off
· X (Twitter): @xdroidand