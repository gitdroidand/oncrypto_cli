# Building OnCrypto

This document describes how to configure, build, test, install, and integrate the OnCrypto native SDK.

OnCrypto uses **CMake** as its primary build system and requires a **C++20-capable toolchain**.

The build system supports:

* Linux
* Windows
* macOS
* Android

Android is an officially supported target.

---

## 1. Build Requirements

### Required

| Requirement  | Version     |
| ------------ | ----------- |
| CMake        | 3.16+       |
| C++ compiler | C++20       |
| Git          | Recommended |
| Ninja        | Recommended |

Supported compiler families include:

* GCC
* Clang
* MSVC

For Android:

* Android NDK
* Android SDK/NDK toolchain
* vcpkg with an Android target triplet

---

# 2. Clone the Repository

```bash
git clone https://github.com/gitdroidand/oncrypto_cli.git
cd oncrypto_cli
```

---

# 3. CMake Configuration

The standard build flow is:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build
```

For development:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug

cmake --build build
```

Ninja is recommended, but CMake generators other than Ninja can also be used when supported by the selected toolchain.

---

# 4. CMake Build Options

OnCrypto exposes the following configuration options:

| Option              | Default | Description                                       |
| ------------------- | ------: | ------------------------------------------------- |
| `BUILD_CLI`         |   `OFF` | Build the command-line application                |
| `BUILD_TESTS`       |   `OFF` | Build the native test suite                       |
| `BUILD_BENCHMARKS`  |   `OFF` | Build benchmarks                                  |
| `BUILD_EXAMPLES`    |   `OFF` | Build example programs                            |
| `BUILD_STATIC_SDK`  |   `OFF` | Build the public static SDK                       |
| `ENABLE_VISIBILITY` |   `OFF` | Enable symbol visibility controls where supported |

Example development configuration:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTS=ON \
    -DBUILD_EXAMPLES=ON
```

Example full development configuration:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_CLI=ON \
    -DBUILD_TESTS=ON \
    -DBUILD_BENCHMARKS=ON \
    -DBUILD_EXAMPLES=ON
```

---

# 5. Release Build

For a normal release build:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build
```

The Release configuration enables compiler optimizations and disables debug assertions where applicable.

---

# 6. Debug Build

For development and debugging:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug

cmake --build build
```

Debug builds enable debugging information and the project's development compiler diagnostics.

---

# 7. Native Libraries

The native build is structured around separate implementation and public SDK components.

Conceptually:

```text
                    OnCrypto
                       │
              ┌────────┴────────┐
              │                 │
          Public API        Internal Core
              │                 │
              └────────┬────────┘
                       │
                Native Engine
                       │
              Platform backend
```

The main public shared library is:

```text
liboncrypto.so
```

on Unix-like platforms.

On Windows:

```text
oncrypto.dll
```

On macOS:

```text
liboncrypto.dylib
```

---

# 8. Static SDK

The public static SDK can be enabled with:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_STATIC_SDK=ON

cmake --build build
```

The static SDK is intended for applications that require static linking.

The exact additional runtime and platform dependencies depend on the selected target and toolchain.

---

# 9. Internal Build Artifacts

The build system also creates internal components used to assemble the SDK.

For example:

```text
liboncrypto_core.a
```

is an internal implementation artifact.

It is **not a stable public application interface**.

Application developers should use:

```text
Public OnCrypto headers
        +
liboncrypto
```

rather than depending directly on internal build targets.

---

# 10. CLI

The command-line application is optional.

Enable it with:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_CLI=ON
```

Then build:

```bash
cmake --build build
```

The CLI target is:

```text
oncrypto_cli
```

It can also be built explicitly:

```bash
cmake --build build --target oncrypto_cli
```

Run:

```bash
./build/oncrypto_cli --help
```

On multi-configuration generators such as Visual Studio, the output location may differ from the single-configuration Ninja layout.

---

# 11. Tests

Enable the test target:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTS=ON
```

Build:

```bash
cmake --build build
```

Build the test target directly:

```bash
cmake --build build --target oncrypto_test
```

The native test executable is:

```text
oncrypto_test
```

On a standard Ninja build it can normally be run with:

```bash
./build/oncrypto_test
```

A successful test run confirms that the configured native test suite completed successfully.

---

# 12. Benchmarks

Benchmarks are optional.

Enable them with:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_BENCHMARKS=ON
```

Then:

```bash
cmake --build build
```

Benchmarks should be run using the generated benchmark target/executable for the current repository revision.

For meaningful performance comparisons, use a Release build and keep CPU frequency scaling, thermal throttling, and background workloads under control.

---

# 13. Examples

Examples are disabled by default.

Enable them with:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_EXAMPLES=ON
```

Then:

```bash
cmake --build build
```

Example targets and output paths may vary as examples evolve.

---

# 14. Linux

## Requirements

A typical Linux development environment requires:

* CMake 3.16+
* GCC or Clang with C++20 support
* Ninja
* development packages required by the native build

For Debian/Ubuntu-based systems, a typical starting point is:

```bash
sudo apt update
sudo apt install \
    build-essential \
    cmake \
    ninja-build \
    git
```

Configure:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release
```

Build:

```bash
cmake --build build
```

---

# 15. Windows

On Windows, OnCrypto supports C++20-capable MSVC and MinGW-based toolchains.

## MSVC

Open a Visual Studio Developer Command Prompt and configure with CMake.

For Ninja:

```powershell
cmake -B build `
    -G Ninja `
    -DCMAKE_BUILD_TYPE=Release

cmake --build build
```

The shared library is:

```text
oncrypto.dll
```

---

## MinGW

A MinGW toolchain can be used when its C++20 support and required native dependencies are correctly configured.

Example:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build
```

The generated shared library is:

```text
oncrypto.dll
```

---

# 16. macOS

On macOS, use a C++20-capable Clang toolchain.

Install the required development tools and CMake, then configure:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release
```

Build:

```bash
cmake --build build
```

The shared library is:

```text
liboncrypto.dylib
```

---

# 17. Android

Android is an officially supported OnCrypto target.

Android builds use the Android NDK CMake toolchain and require a target-specific dependency environment.

The current CMake configuration expects a vcpkg Android triplet.

Examples include:

```text
arm64-android
armv7-android
```

The exact triplet must match the dependency environment being used.

---

## 17.1 Android prerequisites

Install:

* Android SDK
* Android NDK
* CMake
* Ninja
* vcpkg

Set the Android NDK path:

```bash
export ANDROID_NDK=/path/to/android-ndk
```

The exact environment variable names can differ depending on the surrounding build environment; the important value is the NDK directory used by the CMake toolchain.

---

## 17.2 Android toolchain

Use the NDK CMake toolchain:

```text
$ANDROID_NDK/build/cmake/android.toolchain.cmake
```

A typical ARM64 configuration is:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-24 \
    -DVCPKG_TARGET_TRIPLET=arm64-android \
    -DCMAKE_BUILD_TYPE=Release
```

Then:

```bash
cmake --build build
```

The exact Android platform level and vcpkg triplet should match the Android toolchain and dependency configuration used by the project.

---

## 17.3 Android output

The shared native library uses the standard Android library name:

```text
liboncrypto.so
```

For Android, this library can be packaged into an application's native library directory for the corresponding ABI.

Example:

```text
app/
└── src/
    └── main/
        └── jniLibs/
            └── arm64-v8a/
                └── liboncrypto.so
```

The exact packaging layout depends on the consuming Android project.

---

# 18. Android ABI Builds

Android native libraries are ABI-specific.

For example:

```text
arm64-v8a
armeabi-v7a
x86
x86_64
```

A library compiled for one ABI cannot simply be used as another ABI.

When producing Android artifacts, build each required ABI separately or use the project's supported multi-ABI build workflow.

The dependency libraries must target the same ABI as OnCrypto.

---

# 19. Cross Compilation

OnCrypto can be cross-compiled by supplying an appropriate CMake toolchain.

The general model is:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build
```

For Android, use the NDK-provided toolchain.

For other platforms, use the platform's appropriate cross-compilation toolchain.

---

# 20. Dependency Boundary

OnCrypto deliberately separates its public API from its implementation.

Applications should depend on:

```text
Public OnCrypto Headers
          │
          ▼
     OnCrypto SDK
```

and should not depend on internal implementation headers or internal engine symbols.

The public boundary is centered around:

```text
core/include/
include/
```

and the exported SDK library.

Internal implementation details may change without constituting a public API change.

---

# 21. C ABI and Language Bindings

OnCrypto provides a C-compatible ABI for integrations that should not depend directly on the C++ ABI.

The architecture is:

```text
Application / Binding
        │
        ▼
      C ABI
        │
        ▼
    OnCrypto SDK
        │
        ▼
 Native implementation
```

This makes the C ABI suitable for:

* C
* Python
* Rust
* other FFI-based integrations

Language-specific build instructions should be documented in their respective binding documentation rather than duplicated here.

---

# 22. Installation

On platforms where the project installation rules are enabled, the SDK can be installed through CMake:

```bash
cmake --install build --prefix /usr/local
```

For a user-local installation:

```bash
cmake --install build --prefix "$HOME/.local"
```

The exact installed file set is controlled by the repository's CMake installation rules.

---

# 23. Clean Build

When changing toolchains, target platforms, dependency configurations, or CMake options, a clean build is recommended.

Linux/macOS:

```bash
rm -rf build
```

Windows PowerShell:

```powershell
Remove-Item -Recurse -Force build
```

Then configure again.

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release
```

---

# 24. Recommended Development Workflow

A typical development cycle is:

```bash
# Configure
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTS=ON \
    -DBUILD_EXAMPLES=ON

# Build
cmake --build build

# Run tests
./build/oncrypto_test

# Inspect CLI
./build/oncrypto_cli --help
```

For release validation:

```bash
rm -rf build

cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON

cmake --build build
```

---

# 25. Troubleshooting

## CMake version is too old

Check:

```bash
cmake --version
```

OnCrypto requires CMake 3.16 or newer.

---

## C++20 is unavailable

Check the compiler:

```bash
g++ --version
```

or:

```bash
clang++ --version
```

On Windows:

```powershell
cl
```

Use a compiler with complete enough C++20 support for the current OnCrypto source tree.

---

## Wrong Android ABI

Verify:

```text
ANDROID_ABI
VCPKG_TARGET_TRIPLET
```

represent the same target architecture.

For example:

```text
ANDROID_ABI=arm64-v8a
VCPKG_TARGET_TRIPLET=arm64-android
```

Do not mix dependencies built for another architecture.

---

## Android dependency not found

Verify that the requested vcpkg triplet is installed and that the build is configured with:

```bash
-DVCPKG_TARGET_TRIPLET=<target-triplet>
```

The Android build configuration expects a target-specific dependency installation.

---

## Stale CMake configuration

If changing:

* compiler
* generator
* Android ABI
* NDK
* vcpkg triplet
* platform
* major build options

remove the build directory and configure again.

```bash
rm -rf build
```

---

## Shared library cannot be loaded

If an application cannot locate the generated shared library, verify that the runtime library search path includes the directory containing the library.

On Linux during development:

```bash
export LD_LIBRARY_PATH=/path/to/oncrypto/lib:$LD_LIBRARY_PATH
```

For Android, place the `.so` in the appropriate ABI-specific native library directory of the consuming application.

---

# 26. Build Matrix

| Platform | Toolchain    | Build System | Status    |
| -------- | ------------ | ------------ | --------- |
| Linux    | GCC / Clang  | CMake        | Supported |
| Windows  | MSVC / MinGW | CMake        | Supported |
| macOS    | Clang        | CMake        | Supported |
| Android  | Android NDK  | CMake        | Supported |

---

# 27. Public vs Internal Artifacts

### Public

```text
Public headers
liboncrypto.so
liboncrypto.dylib
oncrypto.dll
Public static SDK (when enabled)
C ABI
```

### Internal

```text
liboncrypto_core.a
Internal engine targets
Internal implementation headers
Internal backend symbols
```

Internal artifacts are implementation details and should not be treated as stable application interfaces.

---

# 28. Build System Philosophy

The OnCrypto build system separates:

```text
Public API
    │
    ▼
SDK Library
    │
    ▼
Internal Implementation
    │
    ▼
Platform-specific cryptographic engine
```

This allows the public C++ API and C ABI to remain independent from internal implementation details.

Applications should therefore target the public API rather than internal build components.

---

# 29. Quick Reference

### Standard release build

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Debug + tests

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTS=ON

cmake --build build
```

### Full development build

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_CLI=ON \
    -DBUILD_TESTS=ON \
    -DBUILD_BENCHMARKS=ON \
    -DBUILD_EXAMPLES=ON

cmake --build build
```

### Static SDK

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_STATIC_SDK=ON

cmake --build build
```

### Android ARM64

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-24 \
    -DVCPKG_TARGET_TRIPLET=arm64-android \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build
```

### Clean build

```bash
rm -rf build
```

---

# 30. Summary

The canonical OnCrypto build flow is:

```text
Configure
   │
   ▼
CMake
   │
   ▼
Build
   │
   ├── Shared SDK
   ├── Optional Static SDK
   ├── Optional CLI
   ├── Optional Tests
   ├── Optional Benchmarks
   └── Optional Examples
```

For most native development:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

For development:

```bash
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTS=ON

cmake --build build
```

Android follows the same CMake-based architecture while using the Android NDK toolchain and an ABI-specific dependency environment.

The public SDK and C ABI are the supported integration boundaries. Internal implementation libraries and symbols should not be used as application-level dependencies.
