# OnCrypto API

**OnCrypto 1.6.x — C++20 API Reference**

OnCrypto provides a modern C++ API for authenticated encryption, streaming file encryption, and explicit low-level cryptographic control.

The public C++ API is organized into three layers:

| Namespace      | Status          | Purpose                                                                                     |
| -------------- | --------------- | ------------------------------------------------------------------------------------------- |
| `onc`          | **Recommended** | Modern, simple API for normal application use                                               |
| `onc::extreme` | **Advanced**    | Explicit control over algorithms, KDF parameters, keys, nonces, salts, metadata and buffers |
| `crypto::*`    | **Deprecated**  | Legacy compatibility API                                                                    |

New applications should use `onc` or `onc::extreme`.

---

## 1. API Architecture

```text
┌──────────────────────────────────────────────┐
│                  Application                 │
└──────────────────────┬───────────────────────┘
                       │
            ┌──────────┴──────────┐
            │                     │
       onc:: API            onc::extreme
       Recommended          Advanced / Low-level
            │                     │
            └──────────┬──────────┘
                       │
                OnCrypto Core
                       │
             Native Crypto Backend
```

### Recommended API

Use `onc` when you want:

* authenticated encryption without manually managing cryptographic parameters
* automatic algorithm selection
* simple byte/vector based operations
* file streaming
* minimal API surface

### Extreme API

Use `onc::extreme` when you need direct control over:

* AES-256-GCM
* ChaCha20-Poly1305
* XChaCha20-Poly1305
* PBKDF2-HMAC-SHA256
* PBKDF2 iteration count
* key length
* nonce length
* explicit key material
* explicit salt
* explicit nonce
* metadata storage
* caller-provided output buffers
* one-shot low-level operations

`onc::extreme` is intended for applications and libraries that need deterministic control over the cryptographic parameters rather than automatic defaults.

---

# 2. Core Types

All modern APIs live under:

```cpp
namespace onc
```

## 2.1 `onc::Byte`

```cpp
using Byte = std::uint8_t;
```

Represents one byte of binary data.

---

## 2.2 `onc::ByteView`

```cpp
using ByteView = std::span<const std::uint8_t>;
```

A non-owning read-only view over binary data.

Example:

```cpp
std::vector<std::uint8_t> data = ...;

onc::ByteView view = data;
```

`ByteView` does not own the referenced memory.

---

## 2.3 `onc::MutableByteView`

```cpp
using MutableByteView = std::span<std::uint8_t>;
```

A non-owning mutable view used by APIs that write into caller-provided memory.

Example:

```cpp
std::vector<std::uint8_t> output(4096);

onc::MutableByteView buffer = output;
```

---

# 3. `onc` — Recommended API

## 3.1 `onc::Algorithm`

```cpp
enum class Algorithm {
    Auto,
    AES256_GCM,
    ChaCha20_Poly1305,
    XChaCha20_Poly1305
};
```

Defines the supported high-level encryption algorithms.

| Value                | Description                       |
| -------------------- | --------------------------------- |
| `Auto`               | Let OnCrypto select the algorithm |
| `AES256_GCM`         | AES-256-GCM                       |
| `ChaCha20_Poly1305`  | ChaCha20-Poly1305                 |
| `XChaCha20_Poly1305` | XChaCha20-Poly1305                |

The basic `onc::encrypt()` overload does not expose an algorithm parameter; algorithm selection is handled by the library.

For explicit algorithm selection, use `onc::extreme`.

---

## 3.2 `onc::OutputOwnership`

```cpp
enum class OutputOwnership {
    LibraryOwned,
    CallerOwned
};
```

Describes the intended ownership model for output data.

| Value          | Meaning                                   |
| -------------- | ----------------------------------------- |
| `LibraryOwned` | Output is returned/managed by the library |
| `CallerOwned`  | Caller supplies the destination storage   |

The caller-owned model is primarily exposed through the `onc::extreme` buffer APIs.

---

# 4. One-Shot Encryption

## 4.1 `onc::encrypt`

```cpp
std::vector<unsigned char> encrypt(
    ByteView data,
    std::string_view password
);
```

Encrypts binary data using a password and returns the resulting OnC payload.

### Parameters

| Parameter  | Description                      |
| ---------- | -------------------------------- |
| `data`     | Plaintext input                  |
| `password` | Password used for key derivation |

### Returns

A `std::vector<unsigned char>` containing the encrypted OnC payload.

Example:

```cpp
std::vector<std::uint8_t> plaintext = {
    'H', 'e', 'l', 'l', 'o'
};

auto encrypted = onc::encrypt(
    plaintext,
    "correct horse battery staple"
);
```

---

## 4.2 `onc::decrypt`

```cpp
std::vector<unsigned char> decrypt(
    ByteView data,
    std::string_view password
);
```

Decrypts an OnC payload.

```cpp
auto plaintext = onc::decrypt(
    encrypted,
    "correct horse battery staple"
);
```

If authentication or decryption fails, the operation reports failure through the library's exception mechanism.

---

## 4.3 Vector overloads

For applications already using `std::vector<unsigned char>`, equivalent overloads are available:

```cpp
std::vector<unsigned char> encrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
);

std::vector<unsigned char> decrypt(
    const std::vector<unsigned char>& data,
    const std::string& password
);
```

---

# 5. Streaming File API

The modern namespace also provides file-based streaming encryption.

## 5.1 `onc::encryptStream`

```cpp
bool encryptStream(
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& password,
    size_t chunkSize = 1024 * 1024,
    streaming::ProgressCallback callback = nullptr
);
```

Encrypts a file without requiring the entire input file to be loaded into memory at once.

### Parameters

| Parameter    | Description                |
| ------------ | -------------------------- |
| `inputFile`  | Input file path            |
| `outputFile` | Destination file path      |
| `password`   | Encryption password        |
| `chunkSize`  | Processing chunk size      |
| `callback`   | Optional progress callback |

Default chunk size:

```text
1 MiB
```

Example:

```cpp
bool ok = onc::encryptStream(
    "input.bin",
    "encrypted.onc",
    "my-password"
);
```

---

## 5.2 `onc::decryptStream`

```cpp
bool decryptStream(
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& password,
    size_t chunkSize = 1024 * 1024,
    streaming::ProgressCallback callback = nullptr
);
```

Decrypts an encrypted file using the streaming API.

```cpp
bool ok = onc::decryptStream(
    "encrypted.onc",
    "output.bin",
    "my-password"
);
```

---

# 6. `onc::extreme`

```cpp
namespace onc::extreme
```

The Extreme API is the explicit-control layer of OnCrypto.

Unlike the normal `onc` API, it exposes cryptographic parameters directly.

Use this API when application-level control over cryptographic configuration, binary layout, key material, or output buffers is required.

---

# 7. Extreme Algorithms

## 7.1 `onc::extreme::Algorithm`

```cpp
enum class Algorithm {
    AES256_GCM,
    ChaCha20_Poly1305,
    XChaCha20_Poly1305
};
```

Unlike `onc::Algorithm`, there is no `Auto` mode.

The caller explicitly selects the algorithm.

| Algorithm            |      Key | Default nonce |
| -------------------- | -------: | ------------: |
| `AES256_GCM`         | 32 bytes |      12 bytes |
| `ChaCha20_Poly1305`  | 32 bytes |      12 bytes |
| `XChaCha20_Poly1305` | 32 bytes |      24 bytes |

The implementation currently uses a 16-byte authentication tag.

---

# 8. Extreme KDF

## 8.1 `onc::extreme::Kdf`

```cpp
enum class Kdf {
    PBKDF2_SHA256
};
```

Current Extreme API KDF:

```text
PBKDF2-HMAC-SHA256
```

The default iteration count is:

```cpp
100000
```

The KDF is used when `use_raw_key` is `false`.

---

# 9. `EncryptOptions`

```cpp
struct EncryptOptions {
    Algorithm algorithm = Algorithm::AES256_GCM;
    Kdf kdf = Kdf::PBKDF2_SHA256;

    std::uint32_t iterations = 100000;

    std::size_t key_length = 0;
    std::size_t nonce_length = 0;

    std::vector<std::uint8_t> salt;
    std::vector<std::uint8_t> nonce;
    std::vector<std::uint8_t> key;

    bool use_raw_key = false;
    bool store_metadata = true;

    OutputOwnership ownership =
        OutputOwnership::LibraryOwned;
};
```

---

## 9.1 `algorithm`

Selects the AEAD algorithm.

```cpp
options.algorithm =
    onc::extreme::Algorithm::AES256_GCM;
```

No automatic selection is performed by the Extreme API.

---

## 9.2 `kdf`

Selects the key derivation function.

```cpp
options.kdf =
    onc::extreme::Kdf::PBKDF2_SHA256;
```

Currently only PBKDF2-SHA256 is supported.

---

## 9.3 `iterations`

Controls the PBKDF2 iteration count.

```cpp
options.iterations = 100000;
```

The default is:

```text
100000
```

Higher values increase password-derived key computation cost.

---

## 9.4 `key_length`

Controls the requested key length.

```cpp
options.key_length = 32;
```

A value of `0` selects the algorithm's default key length.

For the currently supported AEAD algorithms, the default key length is 32 bytes.

---

## 9.5 `nonce_length`

Controls the nonce length.

```cpp
options.nonce_length = 12;
```

If zero, the implementation selects the algorithm default:

```text
AES-256-GCM       → 12 bytes
ChaCha20-Poly1305 → 12 bytes
XChaCha20-Poly1305 → 24 bytes
```

---

## 9.6 `salt`

Optional explicit salt.

```cpp
options.salt = {
    /* 16 bytes */
};
```

The current implementation requires an explicit salt to be exactly 16 bytes when supplied.

If omitted during password-based encryption, a random 16-byte salt is generated.

---

## 9.7 `nonce`

Optional explicit nonce.

```cpp
options.nonce = {
    /* nonce bytes */
};
```

If omitted, a nonce is generated automatically according to the selected algorithm and `nonce_length`.

---

## 9.8 `key`

Explicit key material.

```cpp
options.key = {
    /* key bytes */
};
```

When `use_raw_key` is enabled, the supplied key is used directly rather than deriving a key from the password.

The implementation validates that the supplied key length matches the effective key length.

---

## 9.9 `use_raw_key`

```cpp
options.use_raw_key = true;
```

Enables raw-key mode.

In this mode:

* the caller must provide `key`
* password-based derivation is bypassed
* the supplied key must have the expected length

Example:

```cpp
onc::extreme::EncryptOptions options;

options.algorithm =
    onc::extreme::Algorithm::AES256_GCM;

options.use_raw_key = true;

options.key = {
    /* 32-byte key */
};
```

Raw-key mode is intended for applications that already manage cryptographic keys externally.

---

## 9.10 `store_metadata`

```cpp
options.store_metadata = true;
```

Controls whether the encrypted output contains the OnC metadata/header.

### `true`

The output is a complete OnC/ONC1 payload containing:

* format header
* algorithm identifier
* KDF identifier
* iteration count
* salt
* nonce
* authentication tag
* ciphertext

### `false`

The implementation returns a raw AEAD payload consisting of:

```text
ciphertext || authentication_tag
```

When metadata is disabled, the information required for decryption must be supplied separately.

---

# 10. `DecryptOptions`

```cpp
struct DecryptOptions {
    Algorithm algorithm = Algorithm::AES256_GCM;
    Kdf kdf = Kdf::PBKDF2_SHA256;

    std::uint32_t iterations = 100000;

    std::size_t key_length = 0;
    std::size_t nonce_length = 0;

    std::vector<std::uint8_t> salt;
    std::vector<std::uint8_t> nonce;
    std::vector<std::uint8_t> key;

    bool use_raw_key = false;
    bool verify_integrity = true;

    OutputOwnership ownership =
        OutputOwnership::LibraryOwned;
};
```

Most fields have the same meaning as `EncryptOptions`.

---

## 10.1 Metadata-based decryption

If the input contains a valid ONC1 header, the implementation extracts the required parameters from the payload.

The ONC1 header contains:

```text
magic
version
algorithm
KDF
iterations
salt
nonce length
```

The nonce and authentication tag follow the header.

Therefore a normal metadata-bearing payload can be decrypted without manually supplying its salt or nonce.

---

## 10.2 Raw payload decryption

If the input does not contain a valid ONC1 header, Extreme API treats it as a raw payload.

For password-based raw decryption, explicit salt and nonce values are required.

Example:

```cpp
onc::extreme::DecryptOptions options;

options.algorithm =
    onc::extreme::Algorithm::AES256_GCM;

options.salt = salt;
options.nonce = nonce;
```

For raw-key decryption:

```cpp
options.use_raw_key = true;
options.key = key;
options.nonce = nonce;
```

---

# 11. Extreme One-Shot Encryption

## 11.1 `onc::extreme::encrypt`

```cpp
std::vector<unsigned char> encrypt(
    ByteView data,
    std::string_view password,
    const EncryptOptions& options
);
```

Provides explicit control over encryption parameters.

Example:

```cpp
onc::extreme::EncryptOptions options;

options.algorithm =
    onc::extreme::Algorithm::XChaCha20_Poly1305;

options.kdf =
    onc::extreme::Kdf::PBKDF2_SHA256;

options.iterations = 150000;

auto encrypted = onc::extreme::encrypt(
    plaintext,
    "password",
    options
);
```

---

# 12. Extreme One-Shot Decryption

## 12.1 `onc::extreme::decrypt`

```cpp
std::vector<unsigned char> decrypt(
    ByteView data,
    std::string_view password,
    const DecryptOptions& options
);
```

Example:

```cpp
onc::extreme::DecryptOptions options;

options.algorithm =
    onc::extreme::Algorithm::XChaCha20_Poly1305;

options.kdf =
    onc::extreme::Kdf::PBKDF2_SHA256;

auto plaintext = onc::extreme::decrypt(
    encrypted,
    "password",
    options
);
```

For metadata-bearing ONC1 data, algorithm, salt, nonce and iteration information are read from the payload.

---

# 13. Caller-Provided Output Buffers

The Extreme API exposes buffer-oriented functions for applications that need to control destination memory.

## 13.1 `required_output_size`

```cpp
std::size_t required_output_size(
    std::size_t plaintext_size,
    bool include_metadata = true
);
```

Returns an output-size estimate based on the current implementation's format assumptions.

### Important

For the current 1.6.x implementation, this helper should **not** be treated as a universally exact size calculator for every Extreme configuration.

In particular, the implementation currently uses fixed assumptions for metadata sizing rather than deriving the result from arbitrary custom nonce lengths.

For production code requiring exact sizing for custom Extreme configurations, prefer sizing the destination based on the actual encrypted result or update this helper before relying on it for strict allocation guarantees.

---

# 14. `encrypt_into`

```cpp
bool encrypt_into(
    ByteView data,
    std::string_view password,
    const EncryptOptions& options,
    MutableByteView output,
    std::size_t* bytes_written = nullptr
);
```

Encrypts into a caller-provided destination buffer.

Example:

```cpp
std::vector<std::uint8_t> output(
    onc::extreme::required_output_size(
        plaintext.size()
    )
);

std::size_t written = 0;

bool ok = onc::extreme::encrypt_into(
    plaintext,
    "password",
    options,
    output,
    &written
);

if (ok) {
    output.resize(written);
}
```

If the supplied destination is too small, the operation returns `false`.

When `bytes_written` is provided, the implementation reports the required/produced size.

### Current implementation note

Although the API is designed around caller-provided buffers, the current implementation first creates a temporary `std::vector` through the one-shot `encrypt()` operation and then copies the result into the caller's buffer.

Therefore `encrypt_into()` currently provides a caller-controlled destination interface, but it is **not yet a zero-allocation encryption path**.

---

# 15. `decrypt_into`

```cpp
bool decrypt_into(
    ByteView data,
    std::string_view password,
    const DecryptOptions& options,
    MutableByteView output,
    std::size_t* bytes_written = nullptr
);
```

Decrypts into caller-provided memory.

Example:

```cpp
std::vector<std::uint8_t> plaintextBuffer(
    encrypted.size()
);

std::size_t written = 0;

bool ok = onc::extreme::decrypt_into(
    encrypted,
    "password",
    options,
    plaintextBuffer,
    &written
);

if (ok) {
    plaintextBuffer.resize(written);
}
```

As with `encrypt_into()`, the current implementation internally creates the one-shot result before copying it into the destination buffer.

---

# 16. Extreme Context API

The Extreme API also exposes stateful context classes:

```cpp
class EncryptContext;
class DecryptContext;
```

They provide:

```text
reset()
update()
final()
```

The intended model is:

```text
Create Context
      │
      ▼
   update()
      │
      ▼
   update()
      │
      ▼
    final()
```

---

## 16.1 `EncryptContext`

```cpp
explicit EncryptContext(
    EncryptOptions options = {}
);
```

Creates an encryption context.

---

## 16.2 `reset`

```cpp
void reset(
    EncryptOptions options
);
```

Resets the context and replaces its options.

---

## 16.3 `update`

```cpp
std::vector<unsigned char> update(
    ByteView chunk
);
```

Feeds a chunk of plaintext into the context.

---

## 16.4 `final`

```cpp
std::vector<unsigned char> final();
```

Finalizes the context and produces the remaining encrypted output.

### Current implementation status

The current `EncryptContext` implementation does not expose a password parameter in its constructor or methods. Internally, key derivation currently uses an empty password.

Consequently, the context API should currently be considered **experimental / incomplete for password-based encryption**.

Do not use `EncryptContext` as a stable replacement for `onc::extreme::encrypt()` until the context API receives an explicit key/password initialization mechanism.

---

# 17. `DecryptContext`

```cpp
explicit DecryptContext(
    DecryptOptions options = {}
);
```

Creates a decryption context.

---

## 17.1 `reset`

```cpp
void reset(
    DecryptOptions options
);
```

Resets the context.

---

## 17.2 `update`

```cpp
std::vector<unsigned char> update(
    ByteView chunk
);
```

Feeds encrypted data into the context.

---

## 17.3 `final`

```cpp
std::vector<unsigned char> final();
```

Finalizes authentication and decryption.

The current implementation buffers the supplied data and performs the final AEAD operation during `final()`.

Therefore the current context implementation should **not be interpreted as a fully incremental, constant-memory streaming AEAD implementation**.

---

# 18. ONC1 Format

Metadata-bearing Extreme encryption uses the ONC1 binary format.

The format begins with a 28-byte header:

```text
Offset   Size   Field
────────────────────────────────
0        4      Magic: "ONC1"
4        1      Format version
5        1      Algorithm ID
6        1      KDF ID
7        4      PBKDF2 iterations
11       16     Salt
27       1      Nonce length
```

The remaining payload is:

```text
┌───────────────────────┐
│ ONC1 Header (28 B)    │
├───────────────────────┤
│ Nonce                 │
├───────────────────────┤
│ Authentication Tag    │
├───────────────────────┤
│ Ciphertext            │
└───────────────────────┘
```

Current format identifiers:

```cpp
enum class AlgorithmID : uint8_t {
    AES256_GCM         = 1,
    ChaCha20_Poly1305  = 2,
    XChaCha20_Poly1305 = 3
};

enum class KDFID : uint8_t {
    PBKDF2_SHA256 = 1
};
```

The ONC1 header uses a packed 28-byte binary layout.

---

# 19. Extreme API Examples

## 19.1 AES-256-GCM with default parameters

```cpp
onc::extreme::EncryptOptions options;

options.algorithm =
    onc::extreme::Algorithm::AES256_GCM;

auto encrypted = onc::extreme::encrypt(
    plaintext,
    password,
    options
);
```

---

## 19.2 XChaCha20-Poly1305

```cpp
onc::extreme::EncryptOptions options;

options.algorithm =
    onc::extreme::Algorithm::XChaCha20_Poly1305;

options.iterations = 150000;

auto encrypted = onc::extreme::encrypt(
    plaintext,
    password,
    options
);
```

---

## 19.3 Explicit salt and nonce

```cpp
onc::extreme::EncryptOptions options;

options.algorithm =
    onc::extreme::Algorithm::AES256_GCM;

options.salt = salt;
options.nonce = nonce;

auto encrypted = onc::extreme::encrypt(
    plaintext,
    password,
    options
);
```

This is useful when the application has an external parameter-management scheme.

---

## 19.4 Raw-key encryption

```cpp
onc::extreme::EncryptOptions options;

options.algorithm =
    onc::extreme::Algorithm::ChaCha20_Poly1305;

options.use_raw_key = true;
options.key = key;
options.nonce = nonce;

auto encrypted = onc::extreme::encrypt(
    plaintext,
    {},
    options
);
```

In raw-key mode, the key is supplied directly.

---

## 19.5 Raw payload

```cpp
onc::extreme::EncryptOptions options;

options.store_metadata = false;

auto payload = onc::extreme::encrypt(
    plaintext,
    password,
    options
);
```

The resulting payload does not contain the normal ONC1 metadata header.

The application must retain the parameters necessary for decryption.

---

# 20. API Selection Guide

Use the simplest API that satisfies the application's requirements.

### Normal application

```cpp
onc::encrypt(...)
onc::decrypt(...)
```

Recommended for most applications.

### File encryption

```cpp
onc::encryptStream(...)
onc::decryptStream(...)
```

Recommended for large files.

### Explicit algorithm

```cpp
onc::extreme::encrypt(...)
onc::extreme::decrypt(...)
```

Use when algorithm selection must be controlled explicitly.

### Explicit key management

```cpp
onc::extreme::EncryptOptions
onc::extreme::DecryptOptions
```

Use when the application manages key material itself.

### Caller-provided destination

```cpp
onc::extreme::encrypt_into(...)
onc::extreme::decrypt_into(...)
```

Use when integrating with an existing memory/buffer architecture.

### Legacy compatibility

```cpp
crypto::*
```

Only for maintaining existing applications.

---

# 21. Deprecated Legacy API

The original API is still available for compatibility:

```cpp
namespace crypto
```

However, it is deprecated.

New code should **not** use:

```cpp
crypto::encrypt(...)
crypto::decrypt(...)
crypto::encryptFile(...)
crypto::decryptFile(...)
crypto::builder::*
crypto::advanced::*
```

Use:

```cpp
onc::encrypt(...)
onc::decrypt(...)
onc::encryptStream(...)
onc::decryptStream(...)
onc::extreme::*
```

instead.

---

# 22. Migration Guide

## Legacy

```cpp
auto encrypted =
    crypto::encrypt(data, password);
```

### Modern

```cpp
auto encrypted =
    onc::encrypt(data, password);
```

---

## Legacy file API

```cpp
crypto::encryptFile(
    input,
    output,
    password
);
```

### Modern

```cpp
onc::encryptStream(
    input,
    output,
    password
);
```

---

## Legacy algorithm configuration

Applications previously using the builder/advanced APIs should migrate to `onc::extreme` when explicit cryptographic configuration is required.

Example:

```cpp
onc::extreme::EncryptOptions options;

options.algorithm =
    onc::extreme::Algorithm::AES256_GCM;

options.iterations = 150000;

auto encrypted =
    onc::extreme::encrypt(
        data,
        password,
        options
    );
```

---

# 23. Error Handling

The modern APIs use normal C++ return values and exceptions depending on the operation.

For example:

```cpp
try {
    auto encrypted =
        onc::extreme::encrypt(
            data,
            password,
            options
        );
}
catch (const std::exception& e) {
    // Handle encryption error
}
```

Configuration errors can include:

* unsupported algorithm
* unsupported KDF
* invalid salt length
* invalid nonce length
* invalid raw-key configuration
* invalid key length
* malformed encrypted payload
* payload too short to contain an authentication tag
* authentication/decryption failure

---

# 24. Security Model

OnCrypto's encryption APIs use authenticated encryption.

The supported AEAD algorithms are:

```text
AES-256-GCM
ChaCha20-Poly1305
XChaCha20-Poly1305
```

Authentication tags are part of the encrypted representation.

For password-based encryption, the Extreme API currently supports:

```text
PBKDF2-HMAC-SHA256
```

with a configurable iteration count.

Applications should:

* use strong passwords or externally managed keys
* avoid reusing nonces with the same key
* avoid manually generating predictable salts
* use the generated ONC1 metadata when practical
* prefer the high-level API unless explicit cryptographic control is required

---

# 25. ABI / C API

OnCrypto also exposes a C-compatible API for language bindings and FFI integrations.

The C ABI is intended for:

* C applications
* language bindings
* FFI layers
* native integrations where C++ ABI compatibility is undesirable

The C API should be preferred over directly exposing C++ implementation types to foreign languages.

---

# 26. API Stability

### Stable / Recommended

```text
onc::encrypt
onc::decrypt
onc::encryptStream
onc::decryptStream
onc::extreme::encrypt
onc::extreme::decrypt
onc::extreme::encrypt_into
onc::extreme::decrypt_into
```

### Deprecated

```text
crypto::*
crypto::builder::*
crypto::advanced::*
```

### Experimental / incomplete

```text
onc::extreme::EncryptContext
onc::extreme::DecryptContext
```

The context APIs currently require further implementation work before they should be treated as a production-grade incremental encryption/decryption interface.

---

# 27. Design Philosophy

OnCrypto intentionally separates convenience from control.

### `onc`

```text
Simple
Safe defaults
Minimal configuration
Application-oriented
```

### `onc::extreme`

```text
Explicit
Configurable
Buffer-oriented
Key-management friendly
Low-level integration
```

### `crypto::*`

```text
Legacy
Compatibility only
Deprecated
```

This allows applications to start with a small API while still providing a low-level interface for advanced integrations.

---

# 28. Summary

For new projects:

```cpp
#include <oncrypto/oncrypto.hpp>

auto encrypted =
    onc::encrypt(data, password);

auto plaintext =
    onc::decrypt(encrypted, password);
```

For explicit cryptographic configuration:

```cpp
onc::extreme::EncryptOptions options;

options.algorithm =
    onc::extreme::Algorithm::XChaCha20_Poly1305;

options.iterations = 150000;

auto encrypted =
    onc::extreme::encrypt(
        data,
        password,
        options
    );
```

For large files:

```cpp
onc::encryptStream(
    input,
    output,
    password
);
```

For legacy applications, `crypto::*` remains available for compatibility but should not be used in new code.
