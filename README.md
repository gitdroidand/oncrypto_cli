# 🔐 OnCrypto

<div align="center">

![Version](https://img.shields.io/badge/version-1.6.0-blue.svg)
![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus)
![CMake](https://img.shields.io/badge/CMake-3.20+-064F8C?logo=cmake)
![Ninja](https://img.shields.io/badge/Ninja-Build-black)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows%20%7C%20macOS%20%7C%20Android-success)

**Modern Cross-Platform Encryption Library & CLI built with C++20**

</div>

---

## 📚 Table of Contents

- [Introduction](#-introduction)
- [Features](#-features)
- [Supported Algorithms](#-supported-algorithms)
- [Automatic Algorithm Selection](#-automatic-algorithm-selection)
- [Architecture](#-architecture)
- [Project Structure](#-project-structure)
- [Requirements](#-requirements)
- [Installation & Building](#-installation--building)
- [CLI Usage](#-cli-usage)
- [Library Usage](#-library-usage)
- [Streaming API](#-streaming-api)
- [C ABI Engine](#-c-abi-engine)
- [Running Tests](#-running-tests)
- [Version History](#-version-history)
- [License](#-license)
- [Contact](#-contact)

---

## 📖 Introduction

**OnCrypto** is a modern, lightweight, and high-performance cross-platform encryption library written in **C++20**.

It provides both:

- 🖥 **Command Line Interface (CLI)** (`oncrypto_cli`)
- 📚 **Library APIs** (`liboncrypto.so` / `liboncrypto.a`)

The library features a decoupled, backend-agnostic design with high-security authenticated encryption algorithms and automatic algorithm selection.

### 🎯 Design Philosophy

- **Simple API** – Core functions like `crypto::encrypt()` and `crypto::decrypt()`
- **Smart Defaults** – Auto-selects the optimal algorithm for your payload size
- **Modular Backend** – Crypto backend is isolated via an abstracted C ABI layer (`oncrypto_engine`)
- **Cross-Platform** – Native support across Linux, macOS, Windows, and Android
- **Production Ready** – Fully unit-tested (14 test cases, 1052 assertions passing)

---

## ✨ Features

| Feature | Status |
|----------|--------|
| 🔐 AES-256-GCM | ✅ |
| ⚡ ChaCha20-Poly1305 | ✅ |
| 🚀 XChaCha20-Poly1305 | ✅ |
| 🤖 Automatic Algorithm Selection | ✅ |
| 🔑 PBKDF2-HMAC-SHA256 Key Derivation | ✅ |
| 🔄 Shared (`liboncrypto.so`) & Static (`liboncrypto.a`) Libraries | ✅ |
| 💻 Interactive & Command Line Interface (`oncrypto_cli`) | ✅ |
| 🌊 Streaming Encryption & Decryption | ✅ |
| 📦 OnC Binary Format | ✅ |
| 🧪 Unit Tests (`oncrypto_test`, `oncrypto_cli_test`) | ✅ |
| 🌍 Cross Platform | ✅ |
| ⚙ CMake + Ninja Build System | ✅ |

---

## 🔐 Supported Algorithms

| Algorithm | Security | Speed | Best For |
|-----------|----------|-------|----------|
| **XChaCha20-Poly1305** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | Small data (<1KB) |
| **ChaCha20-Poly1305** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐☆ | Medium files (1KB - 1MB) |
| **AES-256-GCM** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | Large files (>1MB) |

---

## 🤖 Automatic Algorithm Selection

OnCrypto dynamically evaluates the payload size to pick the safest and most efficient algorithm for the task:

- **Payload < 1 KB:** Uses **XChaCha20-Poly1305**
- **1 KB ≤ Payload ≤ 1 MB:** Uses **ChaCha20-Poly1305**
- **Payload > 1 MB:** Uses **AES-256-GCM**

---

## 🏗 Architecture

```text
[ Application / CLI ]
         │
         ▼
[ Public C++ SDK Layer ] (crypto::, onc::streaming::, crypto::builder::)
         │
         ▼
[ C ABI Engine Layer ] (oncrypto_engine)
         │
         ▼
[ Internal Crypto Backend ]
```

---

## 📂 Project Structure

```text
oncrypto/
├── CMakeLists.txt         # Root build script
├── include/               # Engine C ABI public header
│   └── oncrypto_engine.h
├── core/                  # Core Library implementation
│   ├── include/           # C++ SDK headers
│   └── src/               # Core repository, backend & format engines
├── cli/                   # Command line interface application
│   └── src/main.cpp
├── tests/                 # Unit tests
├── docs/                  # Architectural and API documentation
└── examples/              # Usage examples
```

---

## ⚙ Requirements

* **C++ Compiler:** Modern compiler with C++20 support (`GCC 10+`, `Clang 12+`, `MSVC 2019+`)
* **CMake:** Version 3.20 or higher
* **Build System:** Ninja

---

## 🛠 Installation & Building

```bash
# Clone the repository
git clone [https://github.com/gitdroidand/oncrypto_cli.git](https://github.com/gitdroidand/oncrypto_cli.git)
cd oncrypto_cli

# Create build directory
mkdir build
cd build

# Configure and build using Ninja
cmake -G Ninja ..
ninja
```

Build outputs in `build/`:
- `liboncrypto.so` (Shared library)
- `liboncrypto.a` (Static library)
- `oncrypto_cli` (CLI executable)
- `oncrypto_test` (Test executable)
- `oncrypto_cli_test` (CLI test executable)

---

## 💻 CLI Usage

OnCrypto CLI supports direct command arguments as well as interactive mode.

### Direct Command Examples

Encrypt text directly:
```bash
./oncrypto_cli -text "Hello" -key "secret" -encrypt
```

Encrypt a file:
```bash
./oncrypto_cli -file secret.txt -key pass123 -out encrypted.bin
```

Decrypt a file:
```bash
./oncrypto_cli -file encrypted.bin -key pass123 -decrypt
```

### Command Line Options

```text
-text STRING     Input text string
-file PATH       Input file path
-key PASSWORD    Encryption/decryption password
-encrypt         Encrypt payload
-decrypt         Decrypt payload
-out PATH        Output file path
-i, -interactive Run in interactive mode
-q, -quiet       Quiet mode
-no-algo         Disable algorithm display
-h, -help        Show help options
```

### Interactive Mode

Launch interactive mode using `-i` or `-interactive`:

```bash
./oncrypto_cli -i
```

---

## 📚 Library Usage

All public C++ SDK functions are located under the `crypto::` namespace.

### Basic API

```cpp
#include <oncrypto/oncrypto.hpp>
#include <iostream>

int main() {
    std::string text = "Hello, OnCrypto!";
    std::string password = "SecretPassword123";

    // Encryption & Decryption
    auto encrypted = crypto::encrypt(text, password);
    auto decrypted = crypto::decrypt(encrypted, password);

    // Version & Algorithm information
    std::cout << "Version: " << crypto::getVersion() << "\n";
    std::cout << "Algorithm: " << crypto::getAlgorithmName() << "\n";

    return 0;
}
```

### File Encryption & Decryption

```cpp
crypto::encryptFile("plain.txt", "encrypted.bin", "pass123");
crypto::decryptFile("encrypted.bin", "decrypted.txt", "pass123");
```

### Builder & Advanced APIs

For object-oriented and custom workflows:
- `crypto::builder::Encryptor`
- `crypto::builder::Decryptor`
- `crypto::advanced::encrypt(...)`
- `crypto::advanced::decrypt(...)`

---

## 🌊 Streaming API

For large file streaming, use the `onc::streaming::` namespace:

```cpp
#include <oncrypto/oncrypto.hpp>

onc::streaming::EncryptStream encryptStream;
onc::streaming::DecryptStream decryptStream;
```

---

## 🔌 C ABI Engine

The project contains a public C ABI header (`oncrypto_engine.*`), acting as an internal abstraction layer between the public C++ SDK and the underlying cryptography backend.

---

## 🧪 Running Tests

Build the repository and execute the built test targets:

```bash
cd build
./oncrypto_test
./oncrypto_cli_test
```

**Status:** 14 test cases, 1052 assertions (All passing).

---

## 📜 Version History

* **v1.6.0**
  * Core C ABI engine abstraction (`oncrypto_engine`).
  * Updated CMake build targets (`liboncrypto.so`, `liboncrypto.a`, `oncrypto_cli`).
  * Full test suite validation (14 test cases, 1052 assertions passing).
* **v1.5.0**
  * Added Streaming API (`onc::streaming::EncryptStream`, `onc::streaming::DecryptStream`).
  * Support for OnC binary layout format.
* **v1.0.0**
  * Initial release with AEAD support and automatic algorithm selection.

---

## 📄 License

This project is licensed under the **MIT License**.

---

## 📬 Contact & Support

* **Email:** [droidandsoftwaresinc@gmail.com](mailto:droidandsoftwaresinc@gmail.com)
* **Telegram:** [@droidand_off](https://t.me/droidand_off)
* **X (Twitter):** [@xdroidand](https://x.com/xdroidand)
