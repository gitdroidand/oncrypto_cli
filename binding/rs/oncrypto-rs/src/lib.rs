//! Safe Rust bindings for the OnCrypto native library.
//!
//! This crate is a thin wrapper around `liboncrypto`.
//! It does not implement cryptography itself.
//!
//! The public API intentionally uses idiomatic Rust types such as
//! `&[u8]`, `Vec<u8>`, `Path`, `Result` and typed configuration enums.
//!
//! ```
//! use oncrypto::{decrypt, encrypt};
//!
//! let key = b"my secret";
//! let message = b"Hello OnCrypto!";
//!
//! let encrypted = encrypt(message, key).unwrap();
//! let decrypted = decrypt(&encrypted, key).unwrap();
//!
//! assert_eq!(decrypted, message);
//! ```
// lib.rs

mod ffi;
pub mod oncrypto;
pub mod streaming;

pub use oncrypto::{
    decrypt,
    decrypt_file,
    encrypt,
    encrypt_file,
    builder,
    version,
    Algorithm,
    EncryptorBuilder,
    OnCryptoError,
};

pub use streaming::{
    EncryptStream,
    DecryptStream,
};