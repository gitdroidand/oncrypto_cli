# OnCrypto File Format (OnC)

OnCrypto stores encrypted data in a compact binary format called **OnC**.

## Format Goals

- Provide a self-describing container for ciphertext, nonce, tag, and metadata.
- Allow consumers to verify the format before decryption.
- Keep the SDK public API stable while allowing internal format evolution.

## Key elements

- **Header**: identifies the file as OnCrypto data.
- **Algorithm**: records the authenticated encryption algorithm used.
- **KDF**: records the key derivation function and iteration count.
- **Salt**: securely derives the encryption key from a password.
- **Nonce**: used for AEAD operations.
- **Tag**: authentication tag that protects integrity.
- **Ciphertext**: the encrypted payload.

## Validation

The public API exposes `onc::format::validateHeader()` to confirm the data begins with a valid OnC header.

### Supported metadata fields

- `algorithmName`
- `kdfName`
- `iterations`
- `salt`
- `nonce`
- `tag`

## Corruption handling

If the data is corrupted or the tag does not validate, decryption should fail. The OnCrypto API surfaces decryption failures as exceptions from the public `crypto::decrypt()` and `CryptoRepository::decrypt()` calls.
