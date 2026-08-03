# OnCrypto Security Model

OnCrypto is designed to provide strong confidentiality and integrity for encrypted payloads.

## Authentication and integrity

All supported algorithms are authenticated encryption schemes.
OnCrypto verifies the authentication tag during decryption, and failures result in decryption errors.

## Password-based key derivation

Passwords are converted into encryption keys using PBKDF2 with SHA-256.
This protects against weak passwords by making key derivation intentionally expensive.

## Randomness and nonces

- Secure random bytes are generated for salt and nonce values.
- Each encryption operation uses fresh nonces.
- The internal backend makes nonce generation part of the engine implementation.

## Format-level protection

OnCrypto stores metadata and ciphertext together in the OnC format.
The format is validated before decryption to avoid accidental misuse of invalid data.

## Implementation isolation

The public API does not expose backend implementation details.
The backend provider is an internal component that may be replaced without changing the public SDK interface.

## Threat model

OnCrypto is intended to protect data-at-rest and file encryption workflows.
It assumes that the execution environment is otherwise trusted and focuses on:

- protecting plaintext confidentiality
- preventing ciphertext tampering
- avoiding key reuse for nonces

OnCrypto does not attempt to protect against:

- compromised runtime environments
- side-channel attacks in untrusted hardware
- leaked passwords
