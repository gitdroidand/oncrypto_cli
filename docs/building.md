# Building OnCrypto

This document describes how to build the **OnCrypto SDK**, native libraries, CLI, tests, and language bindings.

---

## Requirements

### Required

* CMake 3.16 or newer
* C++20-capable compiler

  * GCC
  * Clang
  * MSVC
* OpenSSL 3.x development libraries
* Ninja (recommended)

On Linux, install the required development packages with your distribution's package manager.

For Ubuntu/Debian:

```bash
sudo apt update
sudo apt install cmake ninja-build g++ libssl-dev
```

---

# Build

The recommended build configuration uses CMake with Ninja.

From the repository root:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

For a debug build:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

---

# Build Artifacts

The build directory contains the native OnCrypto components.

Typical artifacts include:

```text
build/
├── liboncrypto_core.a
├── liboncrypto.so
├── liboncrypto.a
├── oncrypto_cli
└── oncrypto_test
```

On Windows, the shared library is produced as:

```text
oncrypto.dll
```

On macOS:

```text
liboncrypto.dylib
```

## Artifact roles

### `liboncrypto_core.a`

Internal static implementation library.

It contains the internal OnCrypto implementation and is **not intended to be consumed directly by application developers**.

It exists primarily as an internal SDK build component.

### `liboncrypto.so`

Public OnCrypto shared library on Linux.

This is the primary dynamic SDK artifact for applications and language bindings.

### `liboncrypto.a`

Public OnCrypto static SDK library.

Applications that require static linking can consume this artifact when the corresponding platform and dependency configuration supports it.

### `oncrypto_cli`

The OnCrypto command-line application.

### `oncrypto_test`

Native test executable for validating the OnCrypto implementation.

---

# Installation

On Linux, the SDK can be installed using CMake:

```bash
sudo cmake --install build --prefix /usr/local
```

This installs the public SDK components according to the project's CMake installation rules.

After installation, applications can use the installed OnCrypto headers and library.

---

# Running Tests

Build the test target:

```bash
cmake --build build --target oncrypto_test
```

Run the test executable:

```bash
./build/oncrypto_test
```

A successful test run indicates that the native OnCrypto implementation passed its configured test suite.

---

# CLI

The CLI is built together with the SDK:

```bash
cmake --build build --target oncrypto_cli
```

The resulting executable is normally:

```text
build/oncrypto_cli
```

Run:

```bash
./build/oncrypto_cli --help
```

---

# OpenSSL

OnCrypto uses OpenSSL as its cryptographic backend.

The backend implementation is an **internal implementation detail**.

Application developers should not link directly against internal OnCrypto backend interfaces.

The intended dependency boundary is:

```text
Application
     │
     ▼
OnCrypto Public API
     │
     ▼
liboncrypto
     │
     ▼
Internal OnCrypto implementation
     │
     ▼
OpenSSL
```

The backend is not part of the public OnCrypto API.

---

# Static OpenSSL

Some OnCrypto build configurations require static OpenSSL archives.

CMake can be instructed to prefer static OpenSSL libraries with:

```bash
-DOpenSSL_USE_STATIC_LIBS=ON
```

Example:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DOpenSSL_USE_STATIC_LIBS=ON
```

If CMake cannot locate the required OpenSSL installation, specify its root directory:

```bash
-DOPENSSL_ROOT_DIR=/path/to/openssl
```

Example:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DOpenSSL_USE_STATIC_LIBS=ON \
    -DOPENSSL_ROOT_DIR=/opt/openssl
```

The exact OpenSSL library layout is platform-dependent.

---

# Android and Termux

On Android-based environments such as Termux, the same OnCrypto SDK architecture applies.

The important difference is that OpenSSL must be available for the **target architecture**.

For example:

```text
Android ARM64
     │
     ├── OnCrypto
     │
     └── ARM64 OpenSSL
```

The host OpenSSL installation cannot simply be reused when it targets a different architecture.

A compatible OpenSSL installation must therefore be supplied to CMake.

Typical configuration:

```bash
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DOpenSSL_USE_STATIC_LIBS=ON \
    -DOPENSSL_ROOT_DIR=/path/to/target/openssl
```

For Android builds, use the Android NDK toolchain file and specify the desired ABI.

Example:

```bash
cmake -B build \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-24 \
    -DCMAKE_BUILD_TYPE=Release
```

The exact Android configuration depends on the NDK version and target API level.

---

# Windows

On Windows, OnCrypto can be built with a C++20-capable MSVC toolchain when the required dependencies are available.

Example:

```powershell
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The shared library is produced as:

```text
oncrypto.dll
```

Depending on the CMake/toolchain configuration, additional import-library artifacts may also be generated.

---

# macOS

On macOS, use a C++20-capable Clang toolchain and a compatible OpenSSL installation.

Example:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The shared library is:

```text
liboncrypto.dylib
```

---

# Public SDK Boundary

OnCrypto deliberately separates its public API from its internal implementation.

The public boundary is:

```text
Public Headers
      │
      ▼
OnCrypto API
      │
      ▼
liboncrypto
```

Internal implementation components remain behind this boundary.

Application developers should therefore **not depend on**:

```text
liboncrypto_core.a
internal engine APIs
backend implementation symbols
OpenSSL-specific OnCrypto internals
```

unless they are explicitly working on OnCrypto itself.

---

# C ABI and Language Bindings

OnCrypto provides a dedicated C ABI for foreign-function interfaces.

The architecture is:

```text
                  OnCrypto
                     │
              Public C++ API
                     │
              C ABI Boundary
                     │
        ┌────────────┼────────────┐
        │            │            │
       C           Python         Go
                    pyonc        oncgo
        │
        ├──────── Rust
        ├──────── Zig
        ├──────── Kotlin
        └──────── Swift
```

The C ABI exists specifically so language bindings do not need to understand the internal C++ implementation.

For example, the Python binding uses:

```text
ctypes
   │
   ▼
OnCrypto C ABI
   │
   ▼
liboncrypto
```

See:

```text
docs/pyonc.md
```

for the Python binding documentation.

---

# Clean Build

If the build directory contains stale CMake configuration or artifacts, perform a clean build:

```bash
rm -rf build
```

Then configure again:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

On Windows PowerShell:

```powershell
Remove-Item -Recurse -Force build
```

Then:

```powershell
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

# Recommended Development Workflow

For normal development:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Run the native tests:

```bash
./build/oncrypto_test
```

Run the CLI:

```bash
./build/oncrypto_cli --help
```

For a release build:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

# Build Architecture

The intended build architecture is:

```text
                    CMake
                      │
        ┌─────────────┴─────────────┐
        │                           │
        ▼                           ▼
 liboncrypto_core.a             Tests / CLI
        │
        ▼
   OnCrypto SDK
        │
   ┌────┴─────┐
   ▼          ▼
 Shared      Static
   │          │
   ▼          ▼
liboncrypto.so  liboncrypto.a
   │
   ▼
 C ABI
   │
   ├── pyonc
   ├── oncgo
   ├── oncrypto_rs
   ├── onc_zig
   ├── onckt
   └── future bindings
```

This separation keeps the cryptographic implementation independent from the language-specific integration layers.

---

# Troubleshooting

## OpenSSL not found

If CMake cannot find OpenSSL:

```bash
-DOPENSSL_ROOT_DIR=/path/to/openssl
```

can be supplied during configuration.

Also verify that the OpenSSL installation contains development headers and libraries.

---

## Wrong architecture

When cross-compiling, verify that OpenSSL matches the target architecture.

For example, an ARM64 build cannot link against an x86_64 OpenSSL library.

---

## Shared library cannot be loaded

If an application cannot locate:

```text
liboncrypto.so
```

either install the SDK into a system library location or configure the runtime library search path appropriately.

For development/testing:

```bash
export LD_LIBRARY_PATH=/path/to/oncrypto/lib:$LD_LIBRARY_PATH
```

For Python, the native binding also supports:

```bash
export ONCRYPTO_LIB_PATH=/path/to/liboncrypto.so
```

---

# Summary

The recommended build is:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The resulting SDK exposes the public OnCrypto interface while keeping the implementation and backend layers internal.

The primary public artifacts are:

```text
liboncrypto.so
liboncrypto.a
public headers
```

while:

```text
liboncrypto_core.a
```

remains an internal implementation artifact.

The same public SDK and C ABI provide the foundation for OnCrypto's native applications and multi-language bindings.
