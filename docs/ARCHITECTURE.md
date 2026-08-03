# OnCrypto Architecture

OnCrypto is designed as a cryptographic SDK with a clear separation between the public API, core processing, and backend implementations.

## Architecture overview

```text
Application / CLI
    |
OnCrypto Public API
    |
Core
    |
Engine C ABI
    |
Backend Provider
    |
OpenSSL implementation
```

- **Application / CLI**: Consumes the OnCrypto public API.
- **OnCrypto Public API**: Exposes library interfaces, data structures, and high-level operations.
- **Core**: Handles OnC format, algorithm selection, key derivation, encryption, and decryption workflows.
- **Engine C ABI**: Provides a stable internal boundary between core logic and backend implementations.
- **Backend Provider**: Implements low-level cryptographic primitives used by the core.
- **OpenSSL implementation**: One backend provider currently used by the SDK, treated as an internal detail.

## Design goals

- Keep the public SDK stable and backend-agnostic.
- Hide implementation details behind the public API.
- Allow backend replacement without changing the core or public interface.
- Restrict symbol visibility so only intended API symbols are exported.

## Components

### `CryptoRepository`

- Coordinates encryption and decryption workflows.
- Produces and consumes the OnC binary format.
- Selects the appropriate algorithm and manages metadata.

### Supported algorithms

- `AES-256-GCM`
- `ChaCha20-Poly1305`
- `XChaCha20-Poly1305`

### Backend abstraction

- The core interacts with backend primitives through a minimal C ABI.
- This separation prevents the core from depending on a specific library.
- The internal backend provider is not exposed to SDK consumers.
