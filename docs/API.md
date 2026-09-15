# OnCrypto Public API

OnCrypto exposes a modern C++20 API centered around the `onc` namespace.

The public API is divided into two levels:

* **`onc`** — recommended high-level API for normal application integration.
* **`onc::extreme`** — low-level API for applications that require explicit control over cryptographic parameters, output buffers, ownership, and incremental processing.

The older `crypto` namespace and its sub-namespaces are retained for compatibility but are **deprecated**. New code should use `onc` APIs.

---

## API Status

| Namespace          | Status                                    | Intended use                    |
| ------------------ | ----------------------------------------- | ------------------------------- |
| `onc`              | **Recommended**                           | General application integration |
| `onc::extreme`     | **Recommended for advanced integrations** | Explicit low-level control      |
| `crypto`           | Deprecated                                | Legacy compatibility            |
| `crypto::builder`  | Deprecated                                | Legacy compatibility            |
| `crypto::advanced` | Deprecated                                | Legacy compatibility            |

The `crypto` APIs are preserved for existing applications but should not be used for new development.

---

# `onc` Namespace

The `onc` namespace is the primary public API.

It uses C++ standard-library types such as `std::span`, `std::string_view`, and `std::vector`, making it suitable for modern C++20 applications.

## Basic Types

### `onc::Byte`

```cpp
using Byte = std::uint8_t;
```

Represents a single byte.

### `onc::ByteView`

```cpp
using ByteView = std::span<const std::uint8_t>;
```

A non-owning read-only view over byte data.

### `onc::MutableByteView`

```cpp
using MutableByteView = std::span<std::uint8_t>;
```

A non-owning mutable view over caller-provided byte storage.

---

## `onc::Algorithm`

```cpp
enum class Algorithm {
    Auto,
    AES256_GCM,
    ChaCha20_Poly1305,
    XChaCha20_Poly1305
};
```

Specifies the encryption algorithm used by the high-level API.

| Value                | Description                       |
| -------------------- | --------------------------------- |
| `Auto`               | Let OnCrypto select the algorithm |
| `AES256_GCM`         | AES-256-GCM                       |
| `ChaCha20_Poly1305`  | ChaCha20-Poly1305                 |
| `XChaCha20_Poly1305` | XChaCha20-Poly1305                |

When `Auto` is used, the library applies its configured automatic algorithm-selection policy.

Applications that require deterministic algorithm selection should select an explicit algorithm.

---

## `onc::OutputOwnership`

```cpp
enum class OutputOwnership {
    LibraryOwned,
    CallerOwned
};
```

Describes ownership semantics for APIs that expose output-buffer control.

| Value          | Meaning                                            |
| -------------- | -------------------------------------------------- |
| `LibraryOwned` | Output storage is owned by OnCrypto                |
| `CallerOwned`  | Output storage is supplied and owned by the caller |

This type is primarily relevant to `onc::extreme`.

---

# Basic Encryption API

## `onc::encrypt`

Encrypts byte data using a password and returns an OnC-formatted ciphertext buffer.

```cpp
std::vector<unsigned char> encrypt(
    ByteView data,
    std::string_view password
);
```

### Example

```cpp
#include <oncrypto/oncrypto.hpp>

#include <string>

int main() {
    std::string message = "Hello, OnCrypto!";
    std::string password = "SecretPassword123";

    auto encrypted = onc::encrypt(message, password);
    auto decrypted = onc::decrypt(encrypted, password);

    return decrypted == std::vector<unsigned char>(
        message.begin(),
        message.end()
    ) ? 0 : 1;
}
```

The API accepts a `ByteView`, allowing contiguous byte-oriented data to be passed without requiring an intermediate container.

---

## `onc::decrypt`

Decrypts an OnC-formatted ciphertext buffer using a password.

```cpp
std::vector<unsigned char> decrypt(
    ByteView data,
    std::string_view password
);
```

The returned vector contains the original plaintext bytes.

### Example

```cpp
auto plaintext = onc::decrypt(ciphertext, password);
```

Decryption authenticates the ciphertext before accepting the recovered plaintext.

---

## `std::vector` overloads

For convenience, `onc` also exposes overloads accepting `std::vector<unsigned char>` and `std::string`.

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

These are useful when the application already stores its data in standard containers.

---

# File Streaming API

The `onc` namespace provides file-oriented streaming operations for processing data incrementally.

## `onc::encryptStream`

```cpp
bool encryptStream(
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& password,
    size_t chunkSize = 1024 * 1024,
    streaming::ProgressCallback callback = nullptr
);
```

Encrypts an input file incrementally and writes the resulting encrypted data to the output file.

### Parameters

| Parameter    | Description                |
| ------------ | -------------------------- |
| `inputFile`  | Source file path           |
| `outputFile` | Destination file path      |
| `password`   | Encryption password        |
| `chunkSize`  | Processing chunk size      |
| `callback`   | Optional progress callback |

The default chunk size is 1 MiB.

### Example

```cpp
bool success = onc::encryptStream(
    "plain.bin",
    "encrypted.onc",
    "SecretPassword123"
);
```

A progress callback can be supplied when the application needs progress reporting.

---

## `onc::decryptStream`

```cpp
bool decryptStream(
    const std::string& inputFile,
    const std::string& outputFile,
    const std::string& password,
    size_t chunkSize = 1024 * 1024,
    streaming::ProgressCallback callback = nullptr
);
```

Decrypts an encrypted file incrementally.

```cpp
bool success = onc::decryptStream(
    "encrypted.onc",
    "restored.bin",
    "SecretPassword123"
);
```

---

# `onc::extreme`

The `onc::extreme` namespace provides explicit, low-level control over encryption and decryption.

It is intended for applications that need capabilities beyond the normal `onc` API, including:

* explicit algorithm selection
* explicit KDF selection
* iteration control
* explicit key length
* explicit nonce length
* caller-supplied salt
* caller-supplied nonce
* caller-supplied key material
* raw-key operation
* metadata control
* caller-provided output buffers
* incremental encryption/decryption contexts

For ordinary application encryption, prefer `onc::encrypt` and `onc::decrypt`.

---

## `onc::extreme::Algorithm`

```cpp
enum class Algorithm {
    AES256_GCM,
    ChaCha20_Poly1305,
    XChaCha20_Poly1305
};
```

Unlike `onc::Algorithm`, the Extreme API does not provide an `Auto` algorithm.

The caller must explicitly select one of the supported algorithms.

---

## `onc::extreme::Kdf`

```cpp
enum class Kdf {
    PBKDF2_SHA256
};
```

Specifies the key-derivation function used when password-based encryption is selected.

Currently available:

```cpp
onc::extreme::Kdf::PBKDF2_SHA256
```

---

# `EncryptOptions`

`onc::extreme::EncryptOptions` controls an encryption operation at a low level.

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
    OutputOwnership ownership = OutputOwnership::LibraryOwned;
};
```

## Options

### `algorithm`

Selects the AEAD algorithm.

```cpp
options.algorithm = onc::extreme::Algorithm::AES256_GCM;
```

### `kdf`

Selects the password-based key derivation mechanism.

```cpp
options.kdf = onc::extreme::Kdf::PBKDF2_SHA256;
```

### `iterations`

Controls the KDF iteration count.

```cpp
options.iterations = 100000;
```

### `key_length`

Specifies the requested key length when applicable.

A value of `0` leaves the selection to the implementation.

### `nonce_length`

Specifies the requested nonce length when applicable.

A value of `0` leaves the selection to the implementation.

### `salt`

Allows explicit salt material to be supplied.

```cpp
options.salt = salt;
```

### `nonce`

Allows explicit nonce material to be supplied.

```cpp
options.nonce = nonce;
```

Applications should only supply nonce values when they can guarantee correct nonce management for the selected algorithm.

### `key`

Allows explicit key material to be supplied.

```cpp
options.key = key;
```

### `use_raw_key`

Enables raw-key operation.

```cpp
options.use_raw_key = true;
```

When raw-key operation is enabled, applications are responsible for supplying appropriate key material.

### `store_metadata`

Controls whether encryption metadata is stored with the resulting output.

```cpp
options.store_metadata = true;
```

### `ownership`

Controls output ownership semantics.

```cpp
options.ownership =
    onc::OutputOwnership::LibraryOwned;
```

---

# `DecryptOptions`

`onc::extreme::DecryptOptions` provides the corresponding low-level decryption controls.

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
    OutputOwnership ownership = OutputOwnership::LibraryOwned;
};
```

### `verify_integrity`

Controls integrity verification during decryption.

```cpp
options.verify_integrity = true;
```

Applications handling authenticated ciphertext should normally keep integrity verification enabled.

---

# `required_output_size`

Calculates the output buffer size required for encryption.

```cpp
std::size_t required_output_size(
    std::size_t plaintext_size,
    bool include_metadata = true
);
```

### Example

```cpp
std::size_t outputSize =
    onc::extreme::required_output_size(
        plaintext.size(),
        true
    );

std::vector<std::uint8_t> output(outputSize);
```

This function is intended primarily for buffer-oriented APIs such as `encrypt_into`.

---

# Buffer-Oriented Encryption

## `encrypt_into`

Encrypts data directly into caller-provided output storage.

```cpp
bool encrypt_into(
    ByteView data,
    std::string_view password,
    const EncryptOptions& options,
    MutableByteView output,
    std::size_t* bytes_written = nullptr
);
```

### Parameters

| Parameter       | Description                                 |
| --------------- | ------------------------------------------- |
| `data`          | Plaintext input                             |
| `password`      | Password used for password-based encryption |
| `options`       | Encryption configuration                    |
| `output`        | Caller-provided destination buffer          |
| `bytes_written` | Optional number of bytes produced           |

### Example

```cpp
onc::extreme::EncryptOptions options;
options.algorithm =
    onc::extreme::Algorithm::XChaCha20_Poly1305;

std::size_t required =
    onc::extreme::required_output_size(
        plaintext.size()
    );

std::vector<std::uint8_t> output(required);

std::size_t written = 0;

bool success = onc::extreme::encrypt_into(
    plaintext,
    password,
    options,
    output,
    &written
);

if (success) {
    output.resize(written);
}
```

This interface is useful when the application wants to control allocation and output storage.

---

# Buffer-Oriented Decryption

## `decrypt_into`

Decrypts ciphertext directly into caller-provided output storage.

```cpp
bool decrypt_into(
    ByteView data,
    std::string_view password,
    const DecryptOptions& options,
    MutableByteView output,
    std::size_t* bytes_written = nullptr
);
```

The caller supplies the destination buffer and may receive the number of bytes written through `bytes_written`.

---

# Explicit Encryption

## `onc::extreme::encrypt`

Encrypts data using an explicit `EncryptOptions` configuration.

```cpp
std::vector<unsigned char> encrypt(
    ByteView data,
    std::string_view password,
    const EncryptOptions& options
);
```

### Example

```cpp
onc::extreme::EncryptOptions options;

options.algorithm =
    onc::extreme::Algorithm::AES256_GCM;

options.kdf =
    onc::extreme::Kdf::PBKDF2_SHA256;

options.iterations = 100000;

auto ciphertext =
    onc::extreme::encrypt(
        plaintext,
        password,
        options
    );
```

---

# Explicit Decryption

## `onc::extreme::decrypt`

Decrypts data using an explicit `DecryptOptions` configuration.

```cpp
std::vector<unsigned char> decrypt(
    ByteView data,
    std::string_view password,
    const DecryptOptions& options
);
```

### Example

```cpp
onc::extreme::DecryptOptions options;

options.algorithm =
    onc::extreme::Algorithm::AES256_GCM;

options.verify_integrity = true;

auto plaintext =
    onc::extreme::decrypt(
        ciphertext,
        password,
        options
    );
```

---

# Incremental Encryption

## `onc::extreme::EncryptContext`

`EncryptContext` provides incremental encryption for applications that process data in chunks.

```cpp
class EncryptContext {
public:
    explicit EncryptContext(EncryptOptions options = {});

    void reset(EncryptOptions options);

    std::vector<unsigned char> update(ByteView chunk);

    std::vector<unsigned char> final();
};
```

### Lifecycle

The typical lifecycle is:

```text
Create
  ↓
update(chunk)
  ↓
update(chunk)
  ↓
update(chunk)
  ↓
final()
```

### Example

```cpp
onc::extreme::EncryptOptions options;

onc::extreme::EncryptContext context(options);

auto part1 = context.update(chunk1);
auto part2 = context.update(chunk2);
auto part3 = context.update(chunk3);

auto final = context.final();
```

`final()` completes the operation and produces the final output.

---

## `EncryptContext::reset`

Reinitializes an existing encryption context with new options.

```cpp
void reset(EncryptOptions options);
```

Example:

```cpp
context.reset(options);
```

This allows a context object to be reused for another encryption operation.

---

# Incremental Decryption

## `onc::extreme::DecryptContext`

`DecryptContext` provides incremental decryption.

```cpp
class DecryptContext {
public:
    explicit DecryptContext(DecryptOptions options = {});

    void reset(DecryptOptions options);

    std::vector<unsigned char> update(ByteView chunk);

    std::vector<unsigned char> final();
};
```

### Example

```cpp
onc::extreme::DecryptOptions options;

options.verify_integrity = true;

onc::extreme::DecryptContext context(options);

auto part1 = context.update(ciphertextChunk1);
auto part2 = context.update(ciphertextChunk2);

auto final = context.final();
```

Integrity verification is controlled by `DecryptOptions::verify_integrity`.

---

# Legacy API

The following namespaces are retained for source compatibility but are deprecated:

```text
crypto
crypto::builder
crypto::advanced
```

New applications should migrate to `onc`.

---

## `crypto::encrypt` / `crypto::decrypt`

The legacy functions:

```cpp
crypto::encrypt(...)
crypto::decrypt(...)
```

are deprecated.

The header explicitly directs users toward:

```cpp
onc::encrypt(...)
onc::decrypt(...)
```

### Migration

```diff
- auto encrypted = crypto::encrypt(data, password);
- auto decrypted = crypto::decrypt(encrypted, password);
+ auto encrypted = onc::encrypt(data, password);
+ auto decrypted = onc::decrypt(encrypted, password);
```

---

## `crypto::builder`

The legacy builder API:

```text
crypto::builder::Encryptor
crypto::builder::Decryptor
```

is deprecated.

New code should use the `onc` API or `onc::extreme` where explicit low-level configuration is required.

---

## `crypto::advanced`

The legacy:

```text
crypto::advanced::encrypt
crypto::advanced::decrypt
```

API is deprecated.

New integrations should use:

```text
onc::encrypt
onc::decrypt
onc::extreme::encrypt
onc::extreme::decrypt
```

depending on the required level of control.

---

# Choosing an API

| Requirement                      | Recommended API                                   |
| -------------------------------- | ------------------------------------------------- |
| Normal encryption/decryption     | `onc::encrypt` / `onc::decrypt`                   |
| Automatic algorithm selection    | `onc::Algorithm::Auto`                            |
| Explicit algorithm selection     | `onc::Algorithm`                                  |
| Large file processing            | `onc::encryptStream` / `onc::decryptStream`       |
| Caller-provided output buffer    | `onc::extreme::encrypt_into` / `decrypt_into`     |
| Explicit salt/nonce/key          | `onc::extreme`                                    |
| Raw-key operation                | `onc::extreme`                                    |
| Explicit KDF configuration       | `onc::extreme`                                    |
| Incremental low-level processing | `onc::extreme::EncryptContext` / `DecryptContext` |
| Existing legacy application      | `crypto::*` with migration planned                |

---

# C ABI

OnCrypto also exposes a C ABI for language interoperability and foreign-function interfaces.

The C ABI provides a language-independent boundary without requiring consumers to depend on the C++ object model.

The C ABI includes engine-level operations such as:

```text
oncrypto_engine_version_major
oncrypto_engine_version_minor
oncrypto_engine_version_string
oncrypto_engine_random_bytes
oncrypto_engine_pbkdf2_hmac_sha256
oncrypto_engine_aead_encrypt
oncrypto_engine_aead_decrypt
oncrypto_engine_hmac_sha256
```

Language bindings can build on this boundary without exposing the internal C++ implementation details.

---

# API Design Principles

OnCrypto's current public API follows several principles:

### Modern C++20

The recommended API uses modern C++ facilities including:

* `std::span`
* `std::string_view`
* strongly typed `enum class`
* standard containers
* explicit ownership semantics

### Layered abstraction

The API provides progressively lower levels of control:

```text
onc
 │
 ├── Basic encryption/decryption
 ├── File streaming
 │
 └── extreme
      ├── Explicit cryptographic parameters
      ├── Caller-owned buffers
      ├── Raw keys
      └── Incremental contexts
```

### Backward compatibility

The legacy `crypto::*` API remains available for compatibility but is no longer the preferred interface.

New functionality should be added to the `onc` API rather than expanding the deprecated namespace.

---

# Recommended Include

Applications using the public C++ API should include:

```cpp
#include <oncrypto/oncrypto.hpp>
```

The public header contains the primary `onc` and `onc::extreme` interfaces.

---

# Versioning

The API documented here corresponds to the current OnCrypto 1.6.x development series.

Because OnCrypto is still under active development, public interfaces may evolve before the first stable release. Applications requiring long-term compatibility should pin their OnCrypto version and review API changes when upgrading.
