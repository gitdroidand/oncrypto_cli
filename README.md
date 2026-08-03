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
- [Installation](#-installation)
- [Build Options](#-build-options)
- [CLI Usage](#-cli-usage)
- [Library Usage](#-library-usage)
- [Streaming API](#-streaming-api)
- [Running Tests](#-running-tests)
- [Version History](#-version-history)
- [License](#-license)
- [Contact](#-contact)

---

## 📖 Introduction

**OnCrypto** is a modern, lightweight, and high-performance cross-platform encryption library written in **C++20**.

It provides both:

- 🖥 **Command Line Interface (CLI)**
- 📚 **Shared Library API** (`liboncrypto.so` / `oncrypto.dll`)

The library features a decoupled, backend-agnostic design with high-security authenticated encryption algorithms and automatic algorithm selection.

### 🎯 Design Philosophy

- **Simple API** – Just `encrypt()` and `decrypt()`
- **Smart Defaults** – Auto-selects the optimal algorithm for your payload size
- **Modular Backend** – Crypto backend is completely isolated via an abstracted C ABI interface
- **Self-Hosting Evolution** – Transitioned to a independent core model powered by `liboncrypto`
- **Cross-Platform** – Native support across Linux, macOS, Windows, and Android
- **Production Ready** – Fully unit-tested with 1000+ internal assertions

---

## ✨ Features

| Feature | Status |
|----------|--------|
| 🔐 AES-256-GCM | ✅ |
| ⚡ ChaCha20-Poly1305 | ✅ |
| 🚀 XChaCha20-Poly1305 | ✅ |
| 🤖 Automatic Algorithm Selection | ✅ |
| 🔑 PBKDF2 Key Derivation | ✅ |
| 🧂 16-byte Random Salt | ✅ |
| 🔄 Shared Library (`liboncrypto`) | ✅ |
| 💻 Interactive & Direct CLI Tool | ✅ |
| 🌊 Streaming Support (Large Files) | ✅ |
| 📦 OnC Binary Format (`ONC1`) | ✅ |
| 🧪 Unit Tests (doctest Integration) | ✅ |
| 🌍 Cross Platform | ✅ |
| ⚙ CMake Build System | ✅ |

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

- **Payload < 1 KB:** Uses **XChaCha20-Poly1305** (Optimized for short strings, keys, or metadata with extended nonce size).
- **1 KB ≤ Payload ≤ 1 MB:** Uses **ChaCha20-Poly1305** (Balanced performance for general data).
- **Payload > 1 MB:** Uses **AES-256-GCM** (Leverages hardware acceleration for bulk processing).

---

## 🏗 Architecture

```text
[ Application / CLI ]
         │
         ▼
[ Public C++ SDK Layer ] (oncrypto.hpp, Streaming, Builder)
         │
         ▼
[ Core Engine / CryptoRepository ]
         │
         ▼
[ C ABI Engine Layer ] (oncrypto_engine.h)
         │
         ▼
[ Backend Implementation ] (OpenSSL / Native Engine)

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
├── tests/                 # Unit tests (doctest)
├── docs/                  # Detailed architectural and API documentation
└── examples/              # Usage examples

```

---

## ⚙ Requirements

* **C++ Compiler:** Modern compiler with C++20 support (`GCC 10+`, `Clang 12+`, `MSVC 2019+`)
* **CMake:** Version 3.20 or higher
* **Build System:** Ninja, Make, or MSVC/IDE generators

---

## 🛠 Installation & Building

```bash
# Clone the repository
git clone [https://github.com/gitdroidand/oncrypto_cli.git](https://github.com/gitdroidand/oncrypto_cli.git)
cd oncrypto

# Create build directory
mkdir build && cd build

# Configure and build
cmake -G Ninja ..
ninja

```

---

## 💻 CLI Usage

OnCrypto CLI supports both interactive mode and direct command arguments.

### Interactive Mode

Run the executable without arguments to launch the interactive prompt:

```bash
./oncrypto_cli

```

### Command Line Arguments

```bash
# Encrypt a string/file
./oncrypto_cli encrypt --input "My secret text" --password "strongpassword" --output secret.onc

# Decrypt a file
./oncrypto_cli decrypt --input secret.onc --password "strongpassword"

# Explicitly select an algorithm
./oncrypto_cli encrypt --input payload.bin --algo aes-256-gcm --password "pass"

```

---

## 📚 Library Usage

### High-Level API (C++)

```cpp
#include <oncrypto/oncrypto.hpp>
#include <iostream>

int main() {
    std::string secret = "Hello, OnCrypto 1.6!";
    std::string password = "MySecurePassword123";

    // Auto-selected encryption
    auto result = oncrypto::encrypt(secret, password);

    if (result.is_success()) {
        std::vector<uint8_t> encrypted_data = result.value();

        // Decryption
        auto dec_result = oncrypto::decrypt(encrypted_data, password);
        if (dec_result.is_success()) {
            std::cout << "Decrypted: " << dec_result.value_as_string() << "\n";
        }
    }
    return 0;
}

```

---

## 🌊 Streaming API

For streaming large files directly off the disk:

```cpp
#include <oncrypto/Streaming.hpp>

oncrypto::StreamingStreamer streamer("large_file.iso", "large_file.iso.onc", "password");
if (streamer.encrypt()) {
    std::cout << "Streaming encryption complete!\n";
}

```

---

## 🧪 Running Tests

OnCrypto utilizes `doctest` for its test suite:

```bash
cd build
ctest --output-on-failure
# Or run direct executable
./oncrypto_tests

```

---

## 📜 Version History

** **v1.6.0**
  * **Core Decoupling & Independence:** Completely rewrote core logic to rely directly on `liboncrypto` via an abstracted C ABI layer.
  * Enhanced memory handling and backend isolation.


* **v1.5.0**
* Added Streaming API support for ultra-large files.
* Introduced `OnCFormat` binary layout specification (`ONC1`).


* **v1.0.0**
* Initial release with AEAD algorithm support and automatic selection.



---

## 📄 License

This project is licensed under the **MIT License**.

---

## 📬 Contact & Support

* **Email:** [droidandsoftwaresinc@gmail.com](mailto:droidandsoftwaresinc@gmail.com)
* **Telegram:** [@droidand_off](https://t.me/droidand_off)
* **X (Twitter):** [@xdroidand](https://x.com/xdroidand)