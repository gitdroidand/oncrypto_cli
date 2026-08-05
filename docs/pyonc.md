# pyonc — Python Binding

**OnCrypto Python Binding**

`pyonc` is the Python binding for the native [OnCrypto](../README.md) encryption library.

It provides an idiomatic Python interface over the OnCrypto **C ABI**, allowing Python applications to use native authenticated encryption without directly interacting with the underlying C++ implementation or cryptographic backend.

---

## Table of Contents

* [Overview](#overview)
* [Architecture](#architecture)
* [Package Structure](#package-structure)
* [Requirements](#requirements)
* [Native Library Loading](#native-library-loading)
* [Quick Start](#quick-start)
* [Version](#version)
* [Data Encryption](#data-encryption)
* [File Encryption](#file-encryption)
* [EncryptorBuilder](#encryptorbuilder)
* [Algorithms](#algorithms)
* [Streaming API](#streaming-api)
* [Error Handling](#error-handling)
* [Binary and Text Data](#binary-and-text-data)
* [API Reference](#api-reference)
* [Integration Testing](#integration-testing)
* [Security Notes](#security-notes)
* [Architecture for Binding Developers](#architecture-for-binding-developers)
* [Current Status](#current-status)

---

# Overview

`pyonc` exposes the core functionality of OnCrypto through Python.

The binding hides the native implementation behind a stable C ABI:

```text
Python Application
       │
       ▼
    pyonc
       │
       ▼
  OnCrypto C ABI
       │
       ▼
 OnCrypto Core
       │
       ▼
 Crypto Backend
```

A Python developer does not need to know:

* C++
* OpenSSL internals
* native memory management
* C ABI function signatures
* native status codes
* native buffer allocation and deallocation

The intended usage is simple:

```python
import pyonc

encrypted = pyonc.encrypt(
    b"Hello OnCrypto",
    "my-password"
)

decrypted = pyonc.decrypt(
    encrypted,
    "my-password"
)

assert decrypted == b"Hello OnCrypto"
```

---

# Architecture

The Python binding does **not** directly bind the internal C++ API.

Instead, OnCrypto exposes a dedicated C ABI layer.

```text
┌─────────────────────────────┐
│      Python Application     │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│            pyonc            │
│      Python API Layer       │
└──────────────┬──────────────┘
               │ ctypes
               ▼
┌─────────────────────────────┐
│       OnCrypto C ABI        │
│    Stable FFI Boundary      │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│       OnCrypto Core         │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│      Crypto Backend         │
└─────────────────────────────┘
```

This architecture is intentional.

The same C ABI can later be consumed by other language bindings:

```text
                 OnCrypto C ABI
                       │
       ┌───────────────┼────────────────┐
       │               │                │
       ▼               ▼                ▼
     pyonc           oncgo        oncrypto_rs
       │
       ├──────────── onc_zig
       ├──────────── onckt
       └──────────── Swift / C# / ...
```

The main C++ API is **not** converted into `extern "C"`.

The dedicated C ABI is the FFI boundary.

---

# Package Structure

Current Python binding structure:

```text
binding/python
├── example
│   └── example.py
├── lib
│   └── pyonc
│       ├── __init__.py
│       ├── _ffi.py
│       └── pyonc.py
└── setup.py
```

The responsibilities are separated as follows:

### `pyonc.py`

High-level Python API.

Contains:

* `encrypt()`
* `decrypt()`
* `encrypt_file()`
* `decrypt_file()`
* `version()`
* `EncryptorBuilder`
* `StreamSession`
* `Algorithm`
* `OnCryptoError`

### `_ffi.py`

Low-level `ctypes` bridge.

Responsible for:

* loading the native library
* declaring C structures
* declaring opaque native handles
* defining C function signatures
* exposing the native library to the Python layer

### `__init__.py`

Public package entry point.

### `example/example.py`

Practical application and integration test suite.

### `setup.py`

Python package build/install configuration.

---

# Requirements

The binding requires:

* Python
* the compiled OnCrypto native library
* the matching OnCrypto C ABI

The Python layer uses the standard-library `ctypes` module and therefore does not require a third-party FFI package.

---

# Native Library Loading

`pyonc` automatically determines the native library filename from the current platform.

| Platform     | Library             |
| ------------ | ------------------- |
| Windows      | `oncrypto.dll`      |
| macOS        | `liboncrypto.dylib` |
| Linux / Unix | `liboncrypto.so`    |

The loader searches several locations.

The most important development override is:

```text
ONCRYPTO_LIB_PATH
```

Example:

```bash
export ONCRYPTO_LIB_PATH=/path/to/liboncrypto.so
```

On Windows, the corresponding environment variable can be set to the full DLL path.

---

## Library Search Order

The binding attempts to locate the library through:

1. `ONCRYPTO_LIB_PATH`
2. repository `build/`
3. `build/Release/`
4. `build/Debug/`
5. the `pyonc` package directory
6. `/usr/local/lib`
7. `/usr/lib`
8. system dynamic-loader search paths

If the library cannot be loaded, `pyonc` raises a `RuntimeError` explaining that the native library could not be found.

---

# Quick Start

## Encrypt and Decrypt

```python
import pyonc

plaintext = b"Hello OnCrypto"
password = "MyPassword"

encrypted = pyonc.encrypt(
    plaintext,
    password
)

decrypted = pyonc.decrypt(
    encrypted,
    password
)

assert decrypted == plaintext
```

`encrypt()` returns `bytes`.

`decrypt()` also returns `bytes`.

---

# Version

Use `version()` to obtain the native OnCrypto version:

```python
import pyonc

print(pyonc.version())
```

Example:

```text
1.5.0
```

The version is obtained directly from the native library through the C ABI.

---

# Data Encryption

## `encrypt()`

Encrypts data in memory.

```python
pyonc.encrypt(data, password)
```

### Parameters

| Parameter  | Accepted types | Description |
| ---------- | -------------- | ----------- |
| `data`     | `str`, `bytes` | Plaintext   |
| `password` | `str`, `bytes` | Password    |

### Returns

```text
bytes
```

Example:

```python
encrypted = pyonc.encrypt(
    b"Sensitive information",
    "StrongPassword"
)
```

Strings are encoded using UTF-8.

---

## `decrypt()`

Decrypts an OnCrypto payload.

```python
pyonc.decrypt(data, password)
```

### Parameters

| Parameter  | Accepted types | Description       |
| ---------- | -------------- | ----------------- |
| `data`     | `bytes`        | Encrypted payload |
| `password` | `str`, `bytes` | Password          |

### Returns

```text
bytes
```

Example:

```python
plaintext = pyonc.decrypt(
    encrypted,
    "StrongPassword"
)
```

For text:

```python
text = plaintext.decode("utf-8")
```

---

# File Encryption

For file-oriented workloads, use the dedicated file APIs.

## `encrypt_file()`

```python
pyonc.encrypt_file(
    src_path,
    dst_path,
    password
)
```

Example:

```python
import pyonc

pyonc.encrypt_file(
    "secret.txt",
    "secret.txt.onc",
    "StrongPassword"
)
```

`src_path` and `dst_path` accept strings and `pathlib.Path` objects.

---

## `decrypt_file()`

```python
pyonc.decrypt_file(
    src_path,
    dst_path,
    password
)
```

Example:

```python
pyonc.decrypt_file(
    "secret.txt.onc",
    "secret-restored.txt",
    "StrongPassword"
)
```

For large files, the file APIs should be preferred over reading the complete file into Python memory.

---

# EncryptorBuilder

`EncryptorBuilder` provides a fluent API for configuring encryption before performing an in-memory encryption operation.

Example:

```python
import pyonc

encrypted = (
    pyonc.EncryptorBuilder()
    .password("StrongPassword")
    .algorithm(pyonc.Algorithm.AES256_GCM)
    .iterations(100_000)
    .encrypt(b"Sensitive data")
)
```

---

## `password()`

Sets the password/key material.

```python
builder.password("StrongPassword")
```

Returns the same builder instance.

---

## `algorithm()`

Selects the encryption algorithm.

```python
builder.algorithm(
    pyonc.Algorithm.AES256_GCM
)
```

The method accepts either an `Algorithm` value or a string.

---

## `iterations()`

Sets the KDF iteration count.

```python
builder.iterations(100_000)
```

Returns the same builder instance.

---

## `encrypt()`

Encrypts the supplied payload using the current builder configuration.

```python
encrypted = builder.encrypt(data)
```

---

## Complete Example

```python
import pyonc

builder = (
    pyonc.EncryptorBuilder()
    .password("VaultPassword")
    .algorithm(pyonc.Algorithm.AES256_GCM)
    .iterations(100_000)
)

encrypted = builder.encrypt(
    b"Confidential information"
)

restored = pyonc.decrypt(
    encrypted,
    "VaultPassword"
)

assert restored == b"Confidential information"
```

---

# Algorithms

The `Algorithm` enumeration currently exposes:

```python
pyonc.Algorithm.AUTO
pyonc.Algorithm.AES256_GCM
pyonc.Algorithm.CHACHA20
pyonc.Algorithm.XCHACHA20
```

## `AUTO`

```python
pyonc.Algorithm.AUTO
```

Allows OnCrypto to select the appropriate algorithm according to its internal policy.

This is the recommended choice when the application does not have a specific algorithm requirement.

---

## `AES256_GCM`

```python
pyonc.Algorithm.AES256_GCM
```

AES-256-GCM authenticated encryption.

---

## `CHACHA20`

```python
pyonc.Algorithm.CHACHA20
```

OnCrypto's ChaCha20-based authenticated encryption mode.

---

## `XCHACHA20`

```python
pyonc.Algorithm.XCHACHA20
```

OnCrypto's XChaCha20-based authenticated encryption mode.

---

# Streaming API

`pyonc` provides a native streaming interface through `StreamSession`.

Unlike manually encrypting independent chunks with repeated calls to `encrypt()`, `StreamSession` represents a persistent native encryption/decryption context.

```text
StreamSession
      │
      ├── update()
      ├── update()
      ├── update()
      │
      └── final()
```

This is the API intended for streaming workloads.

---

## Creating an Encryption Session

```python
session = pyonc.StreamSession(
    "StrongPassword",
    is_encrypt=True
)
```

Data is then processed incrementally:

```python
encrypted_parts = []

encrypted_parts.append(
    session.update(b"chunk 1")
)

encrypted_parts.append(
    session.update(b"chunk 2")
)

encrypted_parts.append(
    session.final()
)

session.close()

encrypted = b"".join(encrypted_parts)
```

---

## Creating a Decryption Session

```python
session = pyonc.StreamSession(
    "StrongPassword",
    is_encrypt=False
)
```

Then:

```python
plaintext_parts = []

plaintext_parts.append(
    session.update(encrypted_chunk)
)

plaintext_parts.append(
    session.update(next_chunk)
)

plaintext_parts.append(
    session.final()
)

session.close()

plaintext = b"".join(plaintext_parts)
```

---

# Context Manager

`StreamSession` implements the Python context-manager protocol.

The recommended usage is:

```python
with pyonc.StreamSession(
    "StrongPassword",
    is_encrypt=True
) as session:

    output = session.update(data)
    final = session.final()
```

The native stream handle is automatically destroyed when leaving the context.

---

# `StreamSession.update()`

Processes a chunk of data.

```python
output = session.update(chunk)
```

Parameters:

```text
chunk: bytes
```

Returns:

```text
bytes
```

A call may return an empty byte string:

```python
b""
```

if no output is produced for that update operation.

---

# `StreamSession.final()`

Finalizes the native streaming operation.

```python
output = session.final()
```

The final output must be included in the resulting stream.

Example:

```python
with pyonc.StreamSession(
    "StrongPassword",
    is_encrypt=True
) as session:

    encrypted = b""

    encrypted += session.update(b"first chunk")
    encrypted += session.update(b"second chunk")
    encrypted += session.final()
```

---

# `StreamSession.close()`

Explicitly destroys the native stream handle.

```python
session.close()
```

It is normally unnecessary when using:

```python
with pyonc.StreamSession(...) as session:
    ...
```

---

# Streaming vs Chunked Encryption

These are different concepts.

### Persistent streaming session

```python
with pyonc.StreamSession(password, True) as session:
    output1 = session.update(chunk1)
    output2 = session.update(chunk2)
    output3 = session.final()
```

One native stream context processes multiple updates.

### Independent encryption operations

```python
encrypted1 = pyonc.encrypt(chunk1, password)
encrypted2 = pyonc.encrypt(chunk2, password)
```

These are separate encryption operations.

They must not be described as equivalent to a streaming session.

For large sequential data, prefer `StreamSession` or the native file APIs.

---

# Error Handling

Native errors are converted into the Python exception:

```python
pyonc.OnCryptoError
```

Example:

```python
import pyonc

try:
    plaintext = pyonc.decrypt(
        encrypted,
        "WrongPassword"
    )
except pyonc.OnCryptoError as error:
    print(error)
```

A native error can be presented as:

```text
OnCrypto Error [3 - Decryption Failed]:
EngineBackend: aead_decrypt failed
```

The binding preserves the native status code:

```python
try:
    pyonc.decrypt(data, "wrong-password")
except pyonc.OnCryptoError as error:
    print(error.code)
```

---

## Error Flow

```text
Native Operation
      │
      ▼
C ABI Status Code
      │
      ▼
_check_status()
      │
      ▼
OnCryptoError
      │
      ▼
Python Application
```

Python applications therefore do not need to manually inspect C ABI status codes.

---

# Binary and Text Data

The encryption API operates on binary data.

`pyonc` accepts:

```python
bytes
bytearray
str
```

where appropriate.

Strings are converted to UTF-8.

Example:

```python
text = "سلام از OnCrypto"

encrypted = pyonc.encrypt(
    text,
    "password"
)

restored = pyonc.decrypt(
    encrypted,
    "password"
).decode("utf-8")

assert restored == text
```

Ciphertext must always be treated as binary data.

Do not assume that encrypted data is valid UTF-8.

---

# API Reference

## Module Functions

### `version()`

```python
version() -> str
```

Returns the native OnCrypto version.

---

### `encrypt()`

```python
encrypt(
    data: str | bytes,
    password: str | bytes
) -> bytes
```

Encrypts data in memory.

---

### `decrypt()`

```python
decrypt(
    data: bytes,
    password: str | bytes
) -> bytes
```

Decrypts an encrypted payload.

---

### `encrypt_file()`

```python
encrypt_file(
    src_path: str | Path,
    dst_path: str | Path,
    password: str | bytes
) -> None
```

Encrypts a file.

---

### `decrypt_file()`

```python
decrypt_file(
    src_path: str | Path,
    dst_path: str | Path,
    password: str | bytes
) -> None
```

Decrypts a file.

---

# `Algorithm`

```python
class Algorithm(str, Enum):
    AUTO = "Auto"
    AES256_GCM = "AES256_GCM"
    CHACHA20 = "ChaCha20"
    XCHACHA20 = "XChaCha20"
```

---

# `EncryptorBuilder`

```python
class EncryptorBuilder:
    password(...)
    algorithm(...)
    iterations(...)
    encrypt(...)
```

Creates a configurable encryption operation.

---

# `StreamSession`

```python
class StreamSession:
    update(...)
    final(...)
    close(...)
```

Constructor:

```python
StreamSession(
    password,
    is_encrypt=True
)
```

The session implements:

```python
__enter__()
__exit__()
```

and can therefore be used with `with`.

---

# Integration Testing

The binding contains a practical integration application:

```text
binding/python/example/example.py
```

Run the integration test suite with:

```bash
python3 binding/python/example/example.py -t
```

The suite currently validates:

```text
Test 1
  In-memory encryption/decryption
        │
        └── encrypt()
            decrypt()

Test 2
  Builder + File API
        │
        ├── EncryptorBuilder
        ├── encrypt_file()
        └── decrypt_file()

Test 3
  Streaming / large-data processing
        │
        ├── file-based processing
        ├── builder chunk processing
        └── standard chunk processing

Test 4
  Error handling
        │
        └── OnCryptoError
```

A successful run reports:

```text
ALL PYONC INTEGRATION TESTS PASSED!
```

The integration suite is an end-to-end test of the Python binding against the native OnCrypto implementation.

---

# Security Notes

## Passwords

Do not hard-code production passwords.

Avoid:

```python
password = "DefaultSecretKey"
```

Use an appropriate secret-management mechanism for production applications.

---

## Ciphertext

Ciphertext is arbitrary binary data.

Store it as `bytes` or use a binary-safe storage/transport mechanism.

If a text-only transport is required, encode the ciphertext using an appropriate encoding such as Base64.

---

## Authentication

OnCrypto uses authenticated encryption.

Applications should treat authentication/decryption failures as security-relevant failures and should not attempt to bypass them.

---

## Do Not Modify Encrypted Payloads

The encrypted payload should be treated as opaque data.

Do not manually modify:

* headers
* nonces
* authentication tags
* encrypted chunks
* format metadata

unless you are implementing the OnCrypto format itself.

---

## Do Not Build a New Cryptographic Protocol Around Raw Operations

The recommended abstraction is:

```python
pyonc.encrypt(...)
pyonc.decrypt(...)
```

or:

```python
pyonc.StreamSession(...)
```

Applications should not independently implement cryptographic nonce management, authentication, key derivation, or custom encryption constructions.

---

# Architecture for Binding Developers

The Python binding intentionally has two layers.

## Python API

Located in:

```text
binding/python/lib/pyonc/pyonc.py
```

This layer provides:

* Python types
* enums
* exceptions
* builder abstraction
* streaming abstraction
* conversion between Python objects and native buffers

---

## FFI Layer

Located in:

```text
binding/python/lib/pyonc/_ffi.py
```

This layer defines the native ABI interface using `ctypes`.

For example:

```python
lib.onc_encrypt_buffer.argtypes = [
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_size_t,
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_size_t,
    ctypes.POINTER(OncBuffer),
]
```

The FFI layer also defines native structures:

```python
class OncBuffer(ctypes.Structure):
    _fields_ = [
        ("data", ctypes.POINTER(ctypes.c_uint8)),
        ("size", ctypes.c_size_t),
    ]
```

and opaque handles:

```python
onc_builder_t = ctypes.c_void_p
onc_stream_t = ctypes.c_void_p
```

---

# Native Memory Management

Native output buffers are owned by the OnCrypto C ABI until explicitly released.

`pyonc` copies the native buffer into Python-owned `bytes`:

```python
res = ctypes.string_at(
    out_buf.data,
    out_buf.size
)
```

and then releases the native buffer:

```python
lib.onc_buffer_free(
    ctypes.byref(out_buf)
)
```

This pattern is used for encryption, decryption, builder operations, and streaming operations.

The Python application therefore receives normal Python `bytes` objects and does not need to manually manage native output memory.

---

# Native Handle Management

`EncryptorBuilder` owns a native builder handle:

```text
onc_builder_t
```

and destroys it when the Python object is finalized.

`StreamSession` owns:

```text
onc_stream_t
```

and provides explicit:

```python
session.close()
```

as well as context-manager support.

Recommended:

```python
with pyonc.StreamSession(
    password,
    is_encrypt=True
) as session:
    ...
```

---

# Current Status

The current `pyonc` implementation has passed end-to-end integration testing.

Validated functionality includes:

* `version()`
* in-memory encryption
* in-memory decryption
* file encryption
* file decryption
* `EncryptorBuilder`
* algorithm selection
* KDF iteration configuration
* `StreamSession`
* native error propagation
* `OnCryptoError`
* native buffer management
* Python/native FFI communication

The binding is therefore **functionally usable**.

The remaining work for a mature distributable Python package is primarily packaging and distribution:

* package metadata refinement
* platform-specific wheels
* native library distribution
* CI builds
* expanded Python test coverage
* type annotation refinement
* PyPI publication

These are distribution concerns rather than fundamental binding architecture problems.

---

# Example

A complete minimal application:

```python
import pyonc

password = "VaultPassword2026!"
message = "Confidential message"

encrypted = pyonc.encrypt(
    message,
    password
)

try:
    decrypted = pyonc.decrypt(
        encrypted,
        password
    ).decode("utf-8")

    print(decrypted)

except pyonc.OnCryptoError as error:
    print(f"Encryption error: {error}")
```

For configurable encryption:

```python
encrypted = (
    pyonc.EncryptorBuilder()
    .password(password)
    .algorithm(pyonc.Algorithm.AES256_GCM)
    .iterations(100_000)
    .encrypt(message)
)
```

For streaming:

```python
with pyonc.StreamSession(
    password,
    is_encrypt=True
) as session:

    output = b""
    output += session.update(b"chunk 1")
    output += session.update(b"chunk 2")
    output += session.final()
```

---

# Summary

`pyonc` provides a Python-native interface to OnCrypto through a dedicated C ABI:

```text
Python
  │
  ▼
pyonc
  │
  ▼
C ABI
  │
  ▼
OnCrypto Core
  │
  ▼
Crypto Backend
```

The main API is intentionally small:

```python
pyonc.encrypt(...)
pyonc.decrypt(...)
pyonc.encrypt_file(...)
pyonc.decrypt_file(...)
pyonc.EncryptorBuilder(...)
pyonc.StreamSession(...)
```

This keeps Python applications independent from the underlying C++ implementation while allowing OnCrypto to expose the same native cryptographic engine to multiple programming languages.

`pyonc` is currently a **functional and tested Python binding** and forms the first consumer of the OnCrypto C ABI architecture.
