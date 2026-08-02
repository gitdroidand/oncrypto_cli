# 🔐 OnCrypto

<div align="center">

![Version](https://img.shields.io/badge/version-1.3.1-blue.svg)
![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus)
![OpenSSL](https://img.shields.io/badge/OpenSSL-3.x-green)
![CMake](https://img.shields.io/badge/CMake-3.20+-064F8C?logo=cmake)
![Ninja](https://img.shields.io/badge/Ninja-Build-black)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows%20%7C%20macOS%20%7C%20Android-success)

**Modern Cross-Platform Encryption Library & CLI built with C++20**

</div>

---

# 📚 Table of Contents

- 📖 Introduction
- ✨ Features
- 🔐 Supported Algorithms
- 🤖 Automatic Algorithm Selection
- 🏗 Project Structure
- ⚙ Requirements
- 🚀 Installation
- 🔨 Build Options
- 💻 CLI Usage
- 📦 Library Usage
- 🧪 Running Tests
- 👨‍💻 Development
- 📜 Version History
- 📄 License
- 📬 Contact

---

# 📖 Introduction

**OnCrypto** is a modern, lightweight and cross-platform encryption library written in **C++20**.

It provides both:

- 🖥 Command Line Interface (CLI)
- 📚 Shared Library API

The library uses **OpenSSL 3.x** and supports modern authenticated encryption algorithms with automatic algorithm selection for optimal performance.

Designed for:

- Desktop Applications
- Embedded Utilities
- Android (Termux)
- Linux Servers
- Cross-platform Software

---

# ✨ Features

| Feature | Status |
|----------|--------|
| 🔐 AES-256-GCM | ✅ |
| ⚡ ChaCha20-Poly1305 | ✅ |
| 🚀 XChaCha20-Poly1305 | ✅ |
| 🤖 Automatic Algorithm Selection | ✅ |
| 🔑 PBKDF2 Key Derivation | ✅ |
| 🧂 16-byte Random Salt | ✅ |
| 🔄 Shared Library | ✅ |
| 💻 CLI Tool | ✅ |
| 🧪 Unit Tests | ✅ |
| 🌍 Cross Platform | ✅ |
| ⚙ CMake Build System | ✅ |
| 📦 Ninja Support | ✅ |

---

# 🔐 Supported Algorithms

| 🔒 Algorithm | 🔑 Security | ⚡ Speed | 📦 Best For |
|--------------|------------|----------|------------|
| 🚀 XChaCha20-Poly1305 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | Small data (<1KB) |
| ⚡ ChaCha20-Poly1305 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐☆ | Medium files |
| 🛡 AES-256-GCM | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | Large files (>1MB) |

---

# 🤖 Automatic Algorithm Selection


             Input Data
                  │
                  ▼
       Is size < 1 KB ?
         │
   Yes ──┴────► XChaCha20
         │
         No
         │
         ▼
    Is size > 1 MB ?
         │
   Yes ──┴────► AES-256-GCM
         │
         No
         │
         ▼
  ChaCha20-Poly1305


No manual selection required.

OnCrypto automatically chooses the most suitable encryption algorithm.

---

# 🏗 Project Structure


OnCrypto/
├── CMakeLists.txt
├── main.cpp
├── include/
│   ├── oncrypto/
│   │   └── oncrypto.hpp
│   ├── algo/
│   │   ├── AES256GCM.hpp
│   │   ├── ChaCha20.hpp
│   │   ├── XChaCha20.hpp
│   │   └── Algorithm.hpp
│   └── utils/
│       ├── FileUtils.hpp
│       └── KeyDerivation.hpp
│
├── src/
│   ├── oncrypto.cpp
│   ├── repo/
│   │   └── CryptoRepository.cpp
│   ├── algo/
│   └── utils/
│
├── examples/
│   └── example.cpp
│
└── tests/
├── test.cpp
└── include/
└── doctest.h


---

# ⚙ Requirements

- C++20 Compiler
- OpenSSL 3.x
- CMake 3.20+
- Ninja (Recommended)

Supported compilers:

- GCC
- Clang
- MSVC

---

# 🚀 Installation

## Clone Repository

```bash
git clone https://github.com/yourusername/OnCrypto.git
cd OnCrypto

---

Linux

Install dependencies

Ubuntu/Debian

sudo apt install build-essential cmake ninja-build libssl-dev

Fedora

sudo dnf install gcc-c++ cmake ninja-build openssl-devel

Arch Linux

sudo pacman -S cmake ninja openssl

Build

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

---

macOS

brew install openssl cmake ninja

cmake -B build \
-G Ninja \
-DOPENSSL_ROOT_DIR=$(brew --prefix openssl)

cmake --build build

---

Windows

Requirements

- Visual Studio 2022
- CMake
- Ninja
- OpenSSL 3.x

cmake -B build -G Ninja
cmake --build build

---

Android (Termux)

pkg update

pkg install clang

pkg install cmake

pkg install ninja

pkg install openssl

git clone https://github.com/yourusername/OnCrypto.git

cd OnCrypto

cmake -B build -G Ninja

cmake --build build

---

Install Library (Optional)

sudo cmake --install build

---

🔨 Build Options

Option| Default| Description
BUILD_SHARED_LIB| ON| Build shared library
BUILD_TESTS| ON| Build unit tests
BUILD_EXAMPLES| ON| Build examples

Example:

cmake -B build \
-G Ninja \
-DBUILD_TESTS=OFF \
-DBUILD_EXAMPLES=OFF

---

💻 CLI Usage

Encrypt Text

./build/oncrypto_cli \
-text "Hello" \
-key "mysecret"

---

Save Encrypted Output

./build/oncrypto_cli \
-text "Hello" \
-key "mysecret" \
-out encrypted.bin

---

Encrypt File

./build/oncrypto_cli \
-file secret.txt \
-key pass \
-out encrypted.bin

---

Decrypt File

./build/oncrypto_cli \
-file encrypted.bin \
-key pass \
-decrypt \
-out decrypted.txt

---

📦 Library Usage

#include <oncrypto/oncrypto.hpp>

std::string data = "Hello World";

auto encrypted =
    crypto::encrypt(data, "password");

auto decrypted =
    crypto::decrypt(encrypted, "password");

std::cout << decrypted;

---

🧪 Running Tests

OnCrypto uses doctest.

Run:

./build/oncrypto_test

Output

8 test cases
8 passed
0 failed

1020 assertions

✔ All tests passed successfully.

---

👨‍💻 Development

Recommended workflow

git clone ...

cmake -B build -G Ninja

cmake --build build

./build/oncrypto_test

Coding Guidelines

- Use C++20
- Keep APIs header-only where appropriate
- Prefer RAII
- Follow modern CMake practices
- Write tests for new features

---

📜 Version History

v1.3.1

- Added automatic algorithm selection
- Improved PBKDF2 implementation
- Added XChaCha20 support
- Improved CLI
- Shared library improvements
- More unit tests
- Performance improvements
- Code cleanup

---

v1.3.0

- Initial public release

---

📄 License

This project is licensed under the MIT License.

See the "LICENSE" file for details.

---

📬 Contact

Project Name:

OnCrypto

Issues:

https://github.com/yourusername/OnCrypto/issues

Repository:

https://github.com/yourusername/OnCrypto

---

<div align="center">Made with ❤️ using Modern C++20 and OpenSSL

⭐ If you like this project, consider giving it a star!

</div>
```