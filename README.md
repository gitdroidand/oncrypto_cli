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

> [!IMPORTANT]
> ## 🚧 Active Development
>
> **OnCrypto is actively developed and has not yet reached its final stable release.**
>
> The project has already progressed substantially and provides a functional cryptographic core, C++ API, C ABI engine, CLI, streaming capabilities, and Python binding.
>
> Development is focused on continued refinement, API evolution, compatibility, portability, testing, and long-term stability.
>
> As OnCrypto moves toward a stable release, some public APIs, interfaces, defaults, formats, build configuration, and other externally visible behaviors may be refined or changed when necessary.
>
> This should not be interpreted as meaning that the current implementation is inherently insecure or unusable. It means that the public interfaces have not yet been formally frozen and long-term compatibility guarantees are still being established.
>
> Applications integrating OnCrypto during this development stage should keep their dependency version pinned and should not assume that every API or behavior is permanently frozen.
>
> Where practical, API changes will follow a migration path. Existing APIs may remain available as deprecated interfaces so applications can continue to build while developers receive compiler warnings and have time to migrate to the newer API.

---

## 📚 Table of Contents

- [Introduction](#-introduction)
- [Development Status](#-development-status)
- [Features](#-features)
- [Supported Algorithms](#-supported-algorithms)
- [Automatic Algorithm Selection](#-automatic-algorithm-selection)
- [Architecture](#-architecture)
- [Project Structure](#-project-structure)
- [Requirements](#-requirements)
- [Building OnCrypto](#-building-oncrypto)
- [CLI Usage](#-cli-usage)
- [C++ Library Usage](#-c-library-usage)
- [Python Binding](#-python-binding)
- [Streaming API](#-streaming-api)
- [C ABI Engine](#-c-abi-engine)
- [Running Tests](#-running-tests)
- [Compatibility & Versioning](#-compatibility--versioning)
- [Version History](#-version-history)
- [Roadmap](#-roadmap)
- [Security Considerations](#-security-considerations)
- [License](#-license)
- [Contact](#-contact)

---

## 📖 Introduction

**OnCrypto** is a modern cross-platform encryption library and command-line tool written in **C++20**.

It provides multiple interfaces over a common native cryptographic engine:

- 🖥 **Command Line Interface** — `oncrypto_cli`
- 📚 **C++ Library API**
- 🔌 **C ABI Engine**
- 🐍 **Python Binding** — `pyonc`
- 🌊 **Streaming Encryption / Decryption**

OnCrypto is designed around a layered architecture that separates public interfaces from the underlying cryptographic implementation.

### Design Goals

- **Modern C++20 API**
- **Cross-platform architecture**
- **Authenticated encryption**
- **Password-based encryption**
- **Streaming support**
- **C ABI interoperability**
- **Python integration**
- **CLI tooling**
- **Modular cryptographic architecture**
- **Predictable API evolution**
- **Long-term compatibility and maintainability**

---

## 🚧 Development Status

OnCrypto is currently in the **1.6.x development series**.

The project has moved well beyond the initial experimental stage and already provides a functional encryption core with multiple usable interfaces.

The current development focus is on bringing the project toward a mature and stable release.

Areas that may continue to evolve include:

- Public C++ API
- C ABI
- Python binding API
- Encryption formats
- Algorithm-selection policy
- Default parameters
- Streaming interfaces
- Build configuration
- Platform integration
- Packaging
- Internal backend interfaces

### API Evolution

OnCrypto is designed to evolve without unnecessarily disrupting existing users.

When an API needs to be replaced, the preferred approach is to keep the existing interface available for a transition period and mark it as deprecated.

```text
Existing API
     │
     ▼
Deprecated API
     │
     ├── remains available
     ├── continues to compile
     └── compiler warning
              │
              ▼
       New API / Migration

This gives applications an opportunity to migrate before an obsolete API is eventually removed.

---

✨ Features

Feature| Status
🔐 AES-256-GCM| ✅
⚡ ChaCha20-Poly1305| ✅
🚀 XChaCha20-Poly1305| ✅
🤖 Automatic Algorithm Selection| ✅
🔑 PBKDF2-HMAC-SHA256| ✅
🔄 Shared Library| ✅
📦 Static Library| ✅
💻 Command Line Interface| ✅
🌊 Streaming API| ✅
📦 OnC Binary Format| ✅
🔌 C ABI Engine| ✅
🐍 Python Binding ("pyonc")| ✅
🧪 Unit Tests| ✅
🌍 Cross-Platform Architecture| ✅
⚙️ CMake + Ninja| ✅
📱 Android Support| 🚧 Active development

---

🔐 Supported Algorithms

OnCrypto provides authenticated-encryption algorithms through its native cryptographic engine.

Algorithm| Type| Primary Use
AES-256-GCM| AEAD| General-purpose authenticated encryption
ChaCha20-Poly1305| AEAD| General-purpose authenticated encryption
XChaCha20-Poly1305| AEAD| Extended-nonce authenticated encryption

All supported algorithms provide authenticated encryption, allowing encrypted data to be verified for integrity and authenticity during decryption.

---

🤖 Automatic Algorithm Selection

OnCrypto provides an automatic algorithm-selection mode.

The current selection policy is:

Payload < 1 KB
    → XChaCha20-Poly1305

1 KB ≤ Payload ≤ 1 MB
    → ChaCha20-Poly1305

Payload > 1 MB
    → AES-256-GCM

The selection policy is part of the project's evolving API behavior and may be refined before the stable release.

Applications that require a fixed algorithm should explicitly select the desired algorithm rather than relying on automatic selection.

---

🏗 Architecture

OnCrypto is organized as a layered system:

┌──────────────────────────────────────┐
│          Application / CLI           │
└──────────────────┬───────────────────┘
                   │
                   ▼
┌──────────────────────────────────────┐
│           Public C++ API             │
│     crypto:: / onc:: / builder       │
└──────────────────┬───────────────────┘
                   │
                   ▼
┌──────────────────────────────────────┐
│             C ABI Engine             │
│          oncrypto_engine             │
└──────────────────┬───────────────────┘
                   │
                   ▼
┌──────────────────────────────────────┐
│      OnCrypto Native Crypto Core     │
└──────────────────────────────────────┘

The C ABI provides a language-independent boundary between higher-level interfaces and the native OnCrypto engine.

This architecture allows other languages to interact with OnCrypto without depending directly on its internal C++ object model.

---

📂 Project Structure

oncrypto/
├── CMakeLists.txt
├── vcpkg.json
│
├── include/
│   └── oncrypto_engine.h
│
├── core/
│   ├── include/
│   │   └── oncrypto/
│   └── src/
│
├── cli/
│   └── src/
│       └── main.cpp
│
├── binding/
│   └── python/
│       ├── setup.py
│       ├── src/
│       │   └── pyonc/
│       │       ├── __init__.py
│       │       ├── pyonc.py
│       │       └── _ffi.py
│       └── example/
│           └── example.py
│
├── tests/
├── docs/
└── examples/

---

⚙️ Requirements

Native Library / CLI

- C++20-compatible compiler
- GCC 10+
- Clang 12+
- MSVC 2019+
- CMake 3.20+
- Ninja
- Git

Python Binding

- Python 3.x
- A built OnCrypto native library
- Standard Python "ctypes" support

The Python binding does not implement the cryptographic engine independently.

Instead, "pyonc" communicates with the native OnCrypto engine through the project's C ABI using Python's "ctypes".

---

🛠 Building OnCrypto

Clone

git clone https://github.com/gitdroidand/oncrypto_cli.git
cd oncrypto_cli

Configure

mkdir build
cd build

cmake -G Ninja ..

Build

ninja

Depending on the selected platform and configuration, the build directory can contain:

liboncrypto.so
liboncrypto.a
oncrypto_cli
oncrypto_test
oncrypto_cli_test

---

💻 CLI Usage

OnCrypto provides direct command-line operations as well as an interactive mode.

Encrypt Text

./oncrypto_cli -text "Hello" -key "secret" -encrypt

Encrypt a File

./oncrypto_cli \
    -file secret.txt \
    -key pass123 \
    -out encrypted.bin

Decrypt a File

./oncrypto_cli \
    -file encrypted.bin \
    -key pass123 \
    -decrypt

Command-Line Options

-text STRING       Input text string
-file PATH          Input file path
-key PASSWORD       Encryption/decryption password
-encrypt            Encrypt payload
-decrypt            Decrypt payload
-out PATH           Output file path
-i, -interactive    Run in interactive mode
-q, -quiet          Quiet mode
-no-algo            Disable algorithm display
-h, -help           Show help options

Interactive Mode

./oncrypto_cli -i

or:

./oncrypto_cli -interactive

---

📚 C++ Library Usage

The public C++ API is available through:

#include <oncrypto/oncrypto.hpp>

Basic Encryption and Decryption

#include <oncrypto/oncrypto.hpp>
#include <iostream>
#include <string>

int main()
{
    std::string text = "Hello, OnCrypto!";
    std::string password = "SecretPassword123";

    auto encrypted =
        crypto::encrypt(text, password);

    auto decrypted =
        crypto::decrypt(encrypted, password);

    std::cout
        << "Version: "
        << crypto::getVersion()
        << '\n';

    std::cout
        << "Algorithm: "
        << crypto::getAlgorithmName()
        << '\n';

    return 0;
}

File Encryption

#include <oncrypto/oncrypto.hpp>

int main()
{
    crypto::encryptFile(
        "plain.txt",
        "encrypted.bin",
        "pass123"
    );

    return 0;
}

File Decryption

#include <oncrypto/oncrypto.hpp>

int main()
{
    crypto::decryptFile(
        "encrypted.bin",
        "decrypted.txt",
        "pass123"
    );

    return 0;
}

Builder API

For applications that require more explicit control over encryption configuration, OnCrypto provides builder functionality including:

crypto::builder::Encryptor
crypto::builder::Decryptor

Advanced APIs are also available through the appropriate "crypto::advanced" interfaces.

---

🐍 Python Binding

OnCrypto provides a Python binding named:

pyonc

The binding is located under:

binding/python/

"pyonc" provides a Python interface to the native OnCrypto engine through the project's C ABI.

This keeps the Python layer lightweight while allowing it to use the same native cryptographic functionality as the C++ API and CLI.

---

Python API

import pyonc

Version

print(pyonc.version())

Encrypt

import pyonc

encrypted = pyonc.encrypt(
    "Hello, OnCrypto!",
    "SecretPassword123"
)

Decrypt

decrypted = pyonc.decrypt(
    encrypted,
    "SecretPassword123"
)

print(decrypted.decode("utf-8"))

The decrypted result is returned as bytes.

---

File Encryption

import pyonc

pyonc.encrypt_file(
    "secret.txt",
    "secret.enc",
    "SecretPassword123"
)

File Decryption

import pyonc

pyonc.decrypt_file(
    "secret.enc",
    "restored.txt",
    "SecretPassword123"
)

---

Algorithm Selection

The binding exposes the native algorithm selection through:

pyonc.Algorithm

Available algorithm values include:

pyonc.Algorithm.AUTO
pyonc.Algorithm.AES256_GCM
pyonc.Algorithm.CHACHA20
pyonc.Algorithm.XCHACHA20

Example:

import pyonc

builder = (
    pyonc.EncryptorBuilder()
    .password("SecretPassword123")
    .algorithm(pyonc.Algorithm.AES256_GCM)
    .iterations(100_000)
)

encrypted = builder.encrypt(
    b"Sensitive data"
)

---

EncryptorBuilder

"EncryptorBuilder" provides a fluent interface for configuring encryption:

builder = (
    pyonc.EncryptorBuilder()
    .password("SecretPassword123")
    .algorithm(pyonc.Algorithm.AES256_GCM)
    .iterations(100_000)
)

encrypted = builder.encrypt(
    b"Hello, OnCrypto!"
)

The current builder interface includes:

password(...)
algorithm(...)
iterations(...)
encrypt(...)

---

Streaming

The Python binding provides:

pyonc.StreamSession

Example encryption session:

import pyonc

with pyonc.StreamSession(
    "SecretPassword123",
    is_encrypt=True
) as stream:

    output = stream.update(chunk)
    final = stream.final()

Decryption:

import pyonc

with pyonc.StreamSession(
    "SecretPassword123",
    is_encrypt=False
) as stream:

    output = stream.update(chunk)
    final = stream.final()

The session provides:

update(chunk)
final()
close()

and supports Python's context-manager protocol.

---

Error Handling

The binding exposes:

pyonc.OnCryptoError

Example:

import pyonc

try:
    encrypted = pyonc.encrypt(
        b"Sensitive data",
        b"password"
    )
except pyonc.OnCryptoError as error:
    print(error)

Native errors are surfaced to Python through the binding's exception interface.

---

📦 Installing "pyonc"

The current Python binding is distributed as part of the OnCrypto source tree.

At the current development stage, "pyonc" is not distributed as a finalized PyPI package.

The native OnCrypto library must first be built.

1. Build OnCrypto

From the repository root:

mkdir build
cd build

cmake -G Ninja ..
ninja

2. Add the Python source directory to "PYTHONPATH"

The Python package source is located at:

binding/python/src

Linux / macOS

From the repository root:

export PYTHONPATH="$PWD/binding/python/src:$PYTHONPATH"

Windows PowerShell

$env:PYTHONPATH="$PWD\binding\python\src;$env:PYTHONPATH"

3. Configure the Native Library Path

"pyonc" can use the environment variable:

ONCRYPTO_LIB_PATH

to explicitly specify the native OnCrypto library.

Linux

export ONCRYPTO_LIB_PATH="/path/to/liboncrypto.so"

macOS

export ONCRYPTO_LIB_PATH="/path/to/liboncrypto.dylib"

Windows PowerShell

$env:ONCRYPTO_LIB_PATH="C:\path\to\oncrypto.dll"

4. Verify the Installation

python -c "import pyonc; print(pyonc.version())"

If the native library is correctly available, this prints the version reported by OnCrypto.

«[!NOTE]
The Python packaging workflow is still evolving. The source-tree setup described above reflects the current development structure and may be replaced by a standard Python package distribution in a future release.»

---

🧪 Python Example

A Python example is included in:

binding/python/example/example.py

The example demonstrates usage of the Python binding, including encryption, decryption, file operations, builder configuration, algorithm selection, and related functionality.

---

🌊 Streaming API

OnCrypto provides streaming interfaces for workloads where processing an entire payload in memory is undesirable.

The native C++ streaming API is available under:

onc::streaming::

The Python binding exposes streaming functionality through:

pyonc.StreamSession

Streaming is intended for large payloads and file-oriented workloads.

---

🔌 C ABI Engine

OnCrypto provides a C-compatible engine interface:

include/oncrypto_engine.h

The C ABI provides a stable interoperability boundary between higher-level APIs and the native OnCrypto engine.

It is also the foundation used by language bindings such as "pyonc".

The native interface includes functionality for areas such as:

Buffer encryption / decryption
File encryption / decryption
Key derivation
Random byte generation
HMAC
Builder operations
Streaming operations
Version information
Error and status information

The C ABI allows external languages and runtimes to access the native engine without directly depending on OnCrypto's internal C++ implementation.

---

🧪 Running Tests

After building:

cd build

Run the core test suite:

./oncrypto_test

Run the CLI tests:

./oncrypto_cli_test

Current test-suite status:

14 test cases
1052 assertions

The test suite continues to evolve alongside the project.

---

🔄 Compatibility & Versioning

OnCrypto is currently developing toward a stable API and compatibility model.

Source Compatibility

C++ APIs may evolve during the development series.

When practical, replaced APIs can remain available as deprecated interfaces before removal.

ABI Compatibility

The C ABI provides a dedicated interoperability boundary, but long-term ABI guarantees should not be considered permanently frozen until the stable release.

Data / Encryption Format Compatibility

Encrypted data is different from an ordinary API because existing ciphertext may need to remain usable for years.

For this reason, format evolution and compatibility are treated as important parts of the project's stabilization process.

Applications that store long-lived encrypted data should pin the OnCrypto version they use and review migration guidance when upgrading across versions that introduce format changes.

---

📜 Version History

v1.6.0

- Introduced the C ABI engine architecture.
- Expanded the native engine interface.
- Updated CMake build targets.
- Added shared and static library targets.
- Continued cross-platform development.
- Added and expanded Python binding functionality.
- Added "pyonc" APIs for native encryption functionality.
- Added Python builder functionality.
- Added Python streaming functionality.
- Expanded testing and integration work.

v1.5.0

- Added the streaming API.
- Added support for the OnC binary layout.
- Continued improvements to the encryption architecture.

v1.0.0

- Initial OnCrypto release.
- AEAD encryption support.
- Core encryption/decryption API.
- Initial automatic algorithm-selection functionality.

---

🗺️ Roadmap

OnCrypto is actively progressing toward a mature and stable release.

Current development areas include:

- API stabilization
- ABI stabilization
- Encryption format stabilization
- Compatibility and migration strategy
- Cryptographic implementation review
- Expanded test coverage
- Cross-platform improvements
- Android integration
- Python packaging
- Documentation expansion
- Release and distribution infrastructure

The objective is not simply to freeze the current API, but to establish a well-defined, maintainable, and reliable foundation for long-term use.

---

⚠️ Security Considerations

OnCrypto is a cryptographic software project and should be integrated carefully.

The project is actively developed, and a final independent security audit has not yet been completed.

For security-sensitive deployments, users should evaluate:

- The exact OnCrypto version
- Key-management practices
- Password handling
- Data-storage requirements
- Encryption-format requirements
- Upgrade and migration procedures
- The current security review status

Cryptographic security depends on more than the choice of an algorithm. Correct key derivation, nonce handling, authentication, format design, error handling, secure storage, and application-level key management are all important.

Applications handling highly sensitive data should perform an appropriate security review before deployment.

---

📄 License

OnCrypto is released under the:

MIT License

See the "LICENSE" file for the complete license text.

---

📬 Contact & Support

Email

droidandsoftwaresinc@gmail.com

Telegram

"@droidand_off" (https://t.me/droidand_off)

X

"@xdroidand" (https://x.com/droidand_off)

---

<div align="center">OnCrypto — Secure by design, evolving by engineering.

</div>
```