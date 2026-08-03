# Building OnCrypto

This document describes how to build the OnCrypto SDK and CLI.

## Requirements

- CMake 3.16 or newer
- C++20-capable compiler (GCC, Clang, MSVC)
- Ninja (recommended)
- OpenSSL development libraries

## Build commands

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Produced artifacts

- `build/liboncrypto.so` / `build/liboncrypto.dll`
- `build/liboncrypto.a`
- `build/oncrypto_cli`
- `build/oncrypto_test`

## Notes

- The SDK exports only OnCrypto public symbols and hides internal implementation details.
- Applications should link against `liboncrypto` and include public headers from `include/`.
- The engine backend is internal and not part of the public SDK interface.

## Running tests

```bash
cmake --build build --target oncrypto_test
./build/oncrypto_test
```
