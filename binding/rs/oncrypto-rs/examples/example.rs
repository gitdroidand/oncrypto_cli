//! OnCrypto Rust Binding — API Example
//!
//! This example demonstrates the safe public API exposed by the OnCrypto
//! Rust binding.
//!
//! The consumer of this crate does not need to know anything about:
//! - C or C++
//! - FFI
//! - raw pointers
//! - native buffers
//! - manual memory management
//!
//! The Rust binding provides an idiomatic Rust interface over the native
//! `liboncrypto` implementation.
//!
//! The example demonstrates:
//!
//! - Library version information
//! - Basic encryption and decryption
//! - Binary data encryption
//! - Wrong-key rejection
//! - Ciphertext integrity validation
//! - Configurable encryption through `EncryptorBuilder`
//! - AES-256-GCM
//! - ChaCha20-Poly1305
//! - XChaCha20-Poly1305
//!
//! Run with:
//!
//!     cargo run --example example
//!
//! The native OnCrypto library must be available through the crate's
//! configured build environment.

use oncrypto::{
    decrypt,
    encrypt,
    version,
    Algorithm,
    EncryptorBuilder,
};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("OnCrypto Rust Binding");
    println!("=====================");
    println!("Version: {}", version());

    // =====================================================================
    // Basic encryption and decryption
    // =====================================================================

    let plaintext = b"Hello from Rust!";
    let password = b"correct horse battery staple";

    let encrypted = encrypt(plaintext, password)?;

    println!();
    println!("Basic Encryption");
    println!("----------------");
    println!(
        "Plaintext : {}",
        String::from_utf8_lossy(plaintext)
    );
    println!("Ciphertext: {} bytes", encrypted.len());

    let decrypted = decrypt(&encrypted, password)?;

    println!(
        "Decrypted : {}",
        String::from_utf8_lossy(&decrypted)
    );

    assert_eq!(
        decrypted,
        plaintext,
        "decrypted data must match the original plaintext"
    );

    println!("✓ Basic round-trip succeeded");

    // =====================================================================
    // Binary data
    // =====================================================================
    //
    // The API operates on arbitrary bytes rather than UTF-8 strings.
    // Therefore it can safely be used with:
    //
    // - Binary files
    // - Serialized data
    // - Images
    // - Compressed data
    // - Protocol payloads
    // - Arbitrary byte sequences
    // =====================================================================

    let binary_data: Vec<u8> = (0..=255).collect();
    let binary_password = b"binary-password";

    let encrypted_binary = encrypt(
        &binary_data,
        binary_password,
    )?;

    let decrypted_binary = decrypt(
        &encrypted_binary,
        binary_password,
    )?;

    assert_eq!(
        decrypted_binary,
        binary_data,
        "binary round-trip must preserve every byte"
    );

    println!("✓ Binary data round-trip succeeded");

    // =====================================================================
    // Empty input
    // =====================================================================
    //
    // Empty input is still a valid byte sequence and must remain
    // distinguishable from an encryption failure.
    // =====================================================================

    let empty: &[u8] = &[];
    let empty_password = b"empty-password";

    let encrypted_empty = encrypt(
        empty,
        empty_password,
    )?;

    let decrypted_empty = decrypt(
        &encrypted_empty,
        empty_password,
    )?;

    assert_eq!(
        decrypted_empty,
        empty,
        "empty input must survive a round-trip"
    );

    println!("✓ Empty input round-trip succeeded");

    // =====================================================================
    // Wrong password
    // =====================================================================
    //
    // OnCrypto uses authenticated encryption. A ciphertext encrypted with
    // one key must not successfully decrypt with another key.
    // =====================================================================

    let wrong_password = b"wrong-password";

    match decrypt(&encrypted, wrong_password) {
        Ok(_) => {
            return Err(
                "unexpected success while decrypting with a wrong password"
                    .into(),
            );
        }
        Err(error) => {
            println!("✓ Wrong password rejected");
            println!("  Error: {error}");
        }
    }

    // =====================================================================
    // Ciphertext integrity
    // =====================================================================
    //
    // Modifying the ciphertext must invalidate authentication.
    // =====================================================================

    let mut corrupted = encrypted.clone();

    assert!(
        !corrupted.is_empty(),
        "ciphertext should not be empty"
    );

    let middle = corrupted.len() / 2;
    corrupted[middle] ^= 0xFF;

    match decrypt(&corrupted, password) {
        Ok(_) => {
            return Err(
                "unexpected success while decrypting corrupted ciphertext"
                    .into(),
            );
        }
        Err(error) => {
            println!("✓ Corrupted ciphertext rejected");
            println!("  Error: {error}");
        }
    }

    // =====================================================================
    // Builder API
    // =====================================================================
    //
    // The builder provides explicit control over the native encryption
    // configuration while keeping the same safe Rust interface.
    // =====================================================================

    println!();
    println!("Builder API");
    println!("-----------");

    // ---------------------------------------------------------------------
    // AES-256-GCM
    // ---------------------------------------------------------------------

    let builder = EncryptorBuilder::new()?
        .key(password)?
        .algorithm(Algorithm::Aes256Gcm)?
        .iterations(100_000)?;

    let aes_ciphertext = builder.encrypt(plaintext)?;
    let aes_plaintext = decrypt(
        &aes_ciphertext,
        password,
    )?;

    assert_eq!(
        aes_plaintext,
        plaintext,
        "AES-256-GCM round-trip must succeed"
    );

    println!("✓ AES-256-GCM round-trip succeeded");
    println!(
        "  Ciphertext size: {} bytes",
        aes_ciphertext.len()
    );

    // ---------------------------------------------------------------------
    // ChaCha20-Poly1305
    // ---------------------------------------------------------------------

    let builder = EncryptorBuilder::new()?
        .key(password)?
        .algorithm(Algorithm::ChaCha20)?;

    let chacha_ciphertext = builder.encrypt(plaintext)?;
    let chacha_plaintext = decrypt(
        &chacha_ciphertext,
        password,
    )?;

    assert_eq!(
        chacha_plaintext,
        plaintext,
        "ChaCha20-Poly1305 round-trip must succeed"
    );

    println!("✓ ChaCha20-Poly1305 round-trip succeeded");
    println!(
        "  Ciphertext size: {} bytes",
        chacha_ciphertext.len()
    );

    // ---------------------------------------------------------------------
    // XChaCha20-Poly1305
    // ---------------------------------------------------------------------

    let builder = EncryptorBuilder::new()?
        .key(password)?
        .algorithm(Algorithm::XChaCha20)?;

    let xchacha_ciphertext = builder.encrypt(plaintext)?;
    let xchacha_plaintext = decrypt(
        &xchacha_ciphertext,
        password,
    )?;

    assert_eq!(
        xchacha_plaintext,
        plaintext,
        "XChaCha20-Poly1305 round-trip must succeed"
    );

    println!("✓ XChaCha20-Poly1305 round-trip succeeded");
    println!(
        "  Ciphertext size: {} bytes",
        xchacha_ciphertext.len()
    );

    // =====================================================================
    // Algorithm isolation
    // =====================================================================
    //
    // Different algorithms should produce independently valid ciphertexts.
    // Each ciphertext must decrypt correctly with the same password.
    // =====================================================================

    assert_eq!(
        decrypt(&aes_ciphertext, password)?,
        plaintext
    );

    assert_eq!(
        decrypt(&chacha_ciphertext, password)?,
        plaintext
    );

    assert_eq!(
        decrypt(&xchacha_ciphertext, password)?,
        plaintext
    );

    println!("✓ All configured algorithms decrypt successfully");

    // =====================================================================
    // Final result
    // =====================================================================

    println!();
    println!("=====================");
    println!("All examples passed.");
    println!("OnCrypto Rust binding is working correctly.");

    Ok(())
}