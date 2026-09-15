# 🔐 OnCrypto

<div align="center">

![Version](https://img.shields.io/badge/version-1.6.0-blue.svg)
![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus)
![CMake](https://img.shields.io/badge/CMake-3.16+-064F8C?logo=cmake)
![Ninja](https://img.shields.io/badge/Ninja-Build-black)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows%20%7C%20macOS%20%7C%20Android-success)

**Modern cross-platform encryption library & CLI, built with C++20**

</div>

> [!IMPORTANT]
> **Active Development — 1.6.x series.** OnCrypto already provides a functional cryptographic core, C++ API, C ABI, CLI, streaming encryption/decryption, and Python and Rust bindings. Public APIs, formats, and build configuration may still evolve before a stable release. Applications integrating OnCrypto during this stage should pin their version.

---

## Overview

**OnCrypto** is a native, cross-platform cryptographic library and CLI providing authenticated encryption through a modern C++20 API, a stable C ABI, and language bindings for Python and Rust.

- 🖥 **CLI** — `oncrypto_cli`
- 📚 **C++ API**
- 🔌 **C ABI** — stable interoperability boundary
- 🐍 **Python binding** — `pyonc`
- 🦀 **Rust binding** — `oncrypto`
- 🌊 **Streaming encryption/decryption**

---

## Features

| Feature                       | Status |
| ------------------------------ | ------ |
| AES-256-GCM                   | ✅ |
| ChaCha20-Poly1305              | ✅ |
| XChaCha20-Poly1305             | ✅ |
| Automatic Algorithm Selection  | ✅ |
| PBKDF2-HMAC-SHA256              | ✅ |
| Shared Library                  | ✅ |
| Static Library                  | ✅ |
| Command Line Interface          | ✅ |
| Streaming API                   | ✅ |
| OnC Binary Format                | ✅ |
| C ABI                            | ✅ |
| Python Binding (`pyonc`)         | ✅ |
| Rust Binding (`oncrypto`)        | ✅ |
| Unit Tests                       | ✅ |
| Cross-Platform Architecture      | ✅ |
| CMake + Ninja                    | ✅ |
| Android                          | 🚧 Active development |

---

## Supported Algorithms

| Algorithm | Type | Use case |
|---|---|---|
| AES-256-GCM | AEAD | General-purpose authenticated encryption |
| ChaCha20-Poly1305 | AEAD | General-purpose authenticated encryption |
| XChaCha20-Poly1305 | AEAD | Authenticated encryption with an extended-nonce construction |
| PBKDF2-HMAC-SHA256 | KDF | Password-based key derivation |

All AEAD algorithms provide both confidentiality and integrity/authenticity — tampered or corrupted ciphertext fails authentication during decryption.

## Automatic Algorithm Selection

```text
Payload < 1 KB        → XChaCha20-Poly1305
1 KB ≤ Payload ≤ 1 MB  → ChaCha20-Poly1305
Payload > 1 MB         → AES-256-GCM
```

This policy is part of OnCrypto's evolving default behavior, not a security guarantee. Applications that require a deterministic, fixed algorithm should select one explicitly instead of relying on automatic selection.

---

## Architecture

```text
┌─────────────────────────────────────┐
│          Application / CLI          │
├─────────────────────────────────────┤
│ C++ API │ C ABI │ Python │ Rust     │
├─────────────────────────────────────┤
│            OnCrypto Core            │
├─────────────────────────────────────┤
│      Native Cryptographic Engine    │
└─────────────────────────────────────┘
```

OnCrypto exposes multiple interfaces over one native cryptographic engine. The C ABI is the language-independent boundary that the Python and Rust bindings are built on, so bindings don't depend on OnCrypto's internal C++ object model.

---

## Supported Platforms

| Platform | Status |
|---|---|
| Linux | Supported |
| Windows | Supported |
| macOS | Supported |
| Android | Active development |

---

## Requirements

- C++20-compatible compiler
- CMake 3.16+
- Ninja
- Git

---

## Building

```bash
git clone https://github.com/gitdroidand/oncrypto_cli.git
cd oncrypto_cli

mkdir build
cd build

cmake -G Ninja ..
ninja
```

Depending on platform and CMake options, the build can produce:

```text
liboncrypto.so / liboncrypto.a
oncrypto_cli
oncrypto_test
oncrypto_cli_test
```

Not every target is produced by every configuration.

### CMake options

| Option | Purpose |
|---|---|
| `BUILD_CLI` | Build the `oncrypto_cli` executable |
| `BUILD_TESTS` | Build the unit and CLI test suites |
| `BUILD_BENCHMARKS` | Build the benchmark suite |
| `BUILD_EXAMPLES` | Build example programs |
| `BUILD_STATIC_SDK` | Build the static library SDK |
| `ENABLE_VISIBILITY` | Control symbol visibility for shared builds |

---

## CLI

```bash
# Encrypt text
./oncrypto_cli -text "Hello" -key "secret" -encrypt

# Encrypt a file
./oncrypto_cli -file secret.txt -key pass123 -out encrypted.bin

# Decrypt a file
./oncrypto_cli -file encrypted.bin -key pass123 -decrypt

# Interactive mode
./oncrypto_cli -i

# Help
./oncrypto_cli -help
```

| Option | Description |
|---|---|
| `-text STRING` | Input text string |
| `-file PATH` | Input file path |
| `-key PASSWORD` | Encryption/decryption password |
| `-encrypt` | Encrypt payload |
| `-decrypt` | Decrypt payload |
| `-out PATH` | Output file path |
| `-i`, `-interactive` | Interactive mode |
| `-q`, `-quiet` | Quiet mode |
| `-no-algo` | Disable algorithm display |
| `-h`, `-help` | Show help |

---

## C++ API

```cpp
#include <oncrypto/oncrypto.hpp>

std::string encrypted = crypto::encrypt("Hello, OnCrypto!", "SecretPassword123");
std::string decrypted = crypto::decrypt(encrypted, "SecretPassword123");

crypto::encryptFile("plain.txt", "encrypted.bin", "pass123");
crypto::decryptFile("encrypted.bin", "decrypted.txt", "pass123");
```

For explicit configuration, use the builder API: `crypto::builder::Encryptor` / `crypto::builder::Decryptor`, with advanced options under `crypto::advanced`.

---

## C ABI

OnCrypto exposes a public C ABI as a stable interoperability boundary — the foundation the Python and Rust bindings are built on. It exists to support C interoperability, FFI, and integration from other languages without depending on OnCrypto's internal C++ implementation.

---

## Python Binding (`pyonc`)

Located under `binding/python/`. `pyonc` talks to the native OnCrypto engine through the C ABI.

```python
import pyonc

encrypted = pyonc.encrypt("Hello, OnCrypto!", "SecretPassword123")
decrypted = pyonc.decrypt(encrypted, "SecretPassword123")

pyonc.encrypt_file("secret.txt", "secret.enc", "SecretPassword123")
pyonc.decrypt_file("secret.enc", "restored.txt", "SecretPassword123")
```

Explicit algorithm selection and builder configuration:

```python
builder = (
    pyonc.EncryptorBuilder()
    .password("SecretPassword123")
    .algorithm(pyonc.Algorithm.AES256_GCM)  # AUTO, AES256_GCM, CHACHA20, XCHACHA20
    .iterations(100_000)
)
encrypted = builder.encrypt(b"Sensitive data")
```

Streaming:

```python
with pyonc.StreamSession("SecretPassword123", is_encrypt=True) as stream:
    output = stream.update(chunk)
    final = stream.final()
```

Errors surface as `pyonc.OnCryptoError`.

> `pyonc` is currently distributed as part of the OnCrypto source tree, not as a finalized PyPI package.

---

## Rust Binding (`oncrypto`)

Located under `binding/rs/oncrypto-rs/`. A safe Rust wrapper around the native OnCrypto library, using idiomatic types (`&[u8]`, `Vec<u8>`, `Path`, `Result`, typed configuration enums).

```rust
use oncrypto::{decrypt, encrypt};

let key = b"my secret";
let message = b"Hello OnCrypto!";

let encrypted = encrypt(message, key).unwrap();
let decrypted = decrypt(&encrypted, key).unwrap();

assert_eq!(decrypted, message);
```
+> The Rust binding is currently distributed as part of the OnCrypto source tree and may not yet be available as a published crates.io package.

Also exposes `Algorithm`, `EncryptorBuilder`, `OnCryptoError`, and streaming support via `EncryptStream` / `DecryptStream`.

```toml
[dependencies]
oncrypto = "1.6.0"
```

---

## Streaming API

For workloads where processing data incrementally is preferable to loading the full payload into memory — large files, large payloads, and similar cases.

- **Python:** `pyonc.StreamSession` (`update`, `final`, `close`)
- **Rust:** `EncryptStream`, `DecryptStream`

---

## OnC Binary Format

OnCrypto encrypts data into its own binary format ("OnC"), which encapsulates the metadata OnCrypto needs to decrypt the payload. The format is still evolving alongside the rest of the project's public surface, so long-term compatibility guarantees should not be assumed yet.

---

## Testing

```bash
cd build
./oncrypto_test
./oncrypto_cli_test
```

Enable test targets with `BUILD_TESTS`.

---

## Benchmarks

A benchmark suite is available under `benchmarks/`. Enable it with `BUILD_BENCHMARKS`, then run the resulting binary from your build directory. No committed benchmark results are published in this README — reproduce numbers locally for your own hardware and configuration.

---

## Security Considerations

- AES-256-GCM, ChaCha20-Poly1305, and XChaCha20-Poly1305 are AEAD algorithms: they provide confidentiality and integrity/authenticity together. Tampered or corrupted ciphertext fails authentication on decryption.
- Password-based encryption uses PBKDF2-HMAC-SHA256 for key derivation.
- Use strong, unique passwords or keys.
- Applications that need deterministic, reproducible behavior should select an algorithm explicitly rather than relying on automatic selection.

OnCrypto is under active development and has not undergone a completed independent security audit. Evaluate your specific version, key-management practices, and format-compatibility needs before using it for sensitive, long-lived data.

---

## Versioning & Compatibility

OnCrypto, its CMake project, and the `oncrypto` Rust crate are all versioned at **1.6.0** as part of the 1.6.x development series. Public APIs, the C ABI, and the OnC format may evolve before a stable release — pin your dependency version and review release notes when upgrading.

---

## Project Structure

```text
oncrypto_cli/
├── core/
├── cli/
├── binding/
│   ├── python/
│   └── rs/
│       └── oncrypto-rs/
├── tests/
├── benchmarks/
├── docs/
├── examples/
├── artifacts/
├── CMakeLists.txt
├── LICENSE
└── README.md
```

---

## License

+Released under the **MIT License**. See [`LICENSE`](LICENSE) for the full text.
