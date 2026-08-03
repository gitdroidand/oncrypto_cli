# OnCrypto Architecture

OnCrypto is designed as a clean SDK boundary between applications and secure cryptographic implementations.

## Core Layers

```
Application / CLI
    ↓
liboncrypto.so / liboncrypto.a
    ↓
OnCrypto Core
    ↓
Engine C ABI
    ↓
Backend implementation
```

- **Application / CLI**: Uses the OnCrypto public API through headers and library import.
- **Public SDK**: `liboncrypto.so` and `liboncrypto.a` expose only OnCrypto symbols.
- **OnCrypto Core**: Orchestrates format handling, algorithm selection, key derivation, and AEAD operations.
- **Engine C ABI**: A stable C entrypoint that decouples the core from any backend provider.
- **Backend implementation**: Internal provider that supplies random bytes, PBKDF2, AEAD, and HMAC.

## Why this design

- The engine C ABI keeps the public SDK stable while allowing backend replacement.
- The core does not expose backend implementation details to applications.
- Shared library visibility is restricted so only intended public symbols are exported.

## Components

### `CryptoRepository`

- Coordinates algorithm selection and encryption/decryption workflows.
- Produces and consumes the OnC binary format.

### Algorithms

Supported algorithms implement authenticated encryption with integrity protection:

- `AES-256-GCM`
- `ChaCha20-Poly1305`
- `XChaCha20-Poly1305`

### Backend abstraction

- The backend exposes low-level primitives through a minimal C ABI.
- This separation prevents the core from depending directly on a specific crypto library.
- The internal backend provider is an implementation detail and is hidden from SDK users.
