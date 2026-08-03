# OnCrypto SDK

OnCrypto is a C++20 encryption SDK built on top of OpenSSL. It provides a clean public API for file encryption, streaming encryption, algorithm selection, and key derivation without exposing OpenSSL to SDK consumers.

## Features

- AES-256-GCM encryption
- ChaCha20-Poly1305 encryption
- XChaCha20-Poly1305 encryption
- PBKDF2 key derivation
- File encryption and decryption APIs
- Streaming encryption and decryption
- Public C++ header-only API under `oncrypto/`
- Shared and static SDK libraries

## Quick Start

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build --prefix /usr/local
```

Use from applications:

```cpp
#include <oncrypto/oncrypto.hpp>

int main() {
    auto encrypted = crypto::encrypt({0x01, 0x02}, "password");
    auto decrypted = crypto::decrypt(encrypted, "password");
    return 0;
}
```
