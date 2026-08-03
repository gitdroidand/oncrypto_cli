# OnCrypto Public API

## Namespaces

- `crypto`
- `crypto::builder`
- `crypto::advanced`
- `onc`
- `onc::streaming`

## Core SDK functions

### `crypto::encrypt`

Encrypt raw data with a password and return OnC-formatted ciphertext.

### `crypto::decrypt`

Decrypt OnC-formatted ciphertext using a password and return the original plaintext.

### `crypto::getAlgorithmName`

Retrieve the algorithm name currently used by the SDK.

### `crypto::getVersion`

Retrieve the library version string.

### `crypto::encryptFile` / `crypto::decryptFile`

Convenience APIs for encrypting and decrypting file contents.

## Builder API

### `crypto::builder::Encryptor`

Use fluent configuration for password, algorithm, and iteration count.

### `crypto::builder::Decryptor`

Use fluent configuration for decrypting with explicit algorithm selection.

## Advanced API

### `crypto::advanced::encrypt`

Encrypt with advanced options such as KDF selection and output format.

### `crypto::advanced::decrypt`

Decrypt data with advanced validation and algorithm controls.

## Streaming API

### `onc::encryptStream`

Encrypt a file stream in chunks with a progress callback.

### `onc::decryptStream`

Decrypt a file stream in chunks with progress reporting.

### `onc::streaming::ProgressCallback`

Callback invoked with processed and total bytes during streaming.

## Engine C ABI

The engine exposes a stable C API to the internal backend.
This lets the core use a backend adapter without depending on backend implementation details.

### Exposed C ABI functions

- `oncrypto_engine_version_major`
- `oncrypto_engine_version_minor`
- `oncrypto_engine_version_string`
- `oncrypto_engine_random_bytes`
- `oncrypto_engine_pbkdf2_hmac_sha256`
- `oncrypto_engine_aead_encrypt`
- `oncrypto_engine_aead_decrypt`
- `oncrypto_engine_hmac_sha256`

## Example

```cpp
#include <oncrypto/oncrypto.hpp>

int main() {
    std::vector<unsigned char> data = {'H','e','l','l','o'};
    std::string password = "secret";

    auto encrypted = crypto::encrypt(data, password);
    auto decrypted = crypto::decrypt(encrypted, password);

    return decrypted == data ? 0 : 1;
}
```
