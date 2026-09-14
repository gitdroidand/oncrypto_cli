// unit.rs
//
// Unit and integration tests for the OnCrypto Rust binding.
//
// These tests verify the safe Rust API against the native OnCrypto library.
// The Rust crate must not perform cryptographic operations itself; therefore,
// successful results also validate the complete Rust -> FFI -> C ABI -> C++
// execution path.

use std::fs;

use oncrypto::{
    decrypt,
    decrypt_file,
    encrypt,
    encrypt_file,
    Algorithm,
    EncryptorBuilder,
};

// ============================================================================
// Helpers
// ============================================================================


fn create_test_directory(name: &str) -> std::path::PathBuf {
    let root = std::env::temp_dir()
        .join(format!(
            "oncrypto-rust-{}-{}",
            name,
            std::process::id()
        ));

    if root.exists() {
        let _ = fs::remove_dir_all(&root);
    }

    fs::create_dir_all(&root)
        .expect("temporary test directory should be created");

    root
}

// ============================================================================
// Version
// ============================================================================

#[test]
fn version_is_available() {
    let version = oncrypto::version();

    assert!(
        !version.is_empty(),
        "native library returned an empty version"
    );

    assert_ne!(
        version,
        "unknown",
        "native library failed to provide a valid version"
    );
}

// ============================================================================
// Basic Buffer Encryption
// ============================================================================

#[test]
fn basic_round_trip() {
    let plaintext = b"hello from rust";
    let key = b"correct horse battery staple";

    let encrypted = encrypt(plaintext, key)
        .expect("encryption should succeed");

    assert!(
        !encrypted.is_empty(),
        "ciphertext should not be empty"
    );

    let decrypted = decrypt(&encrypted, key)
        .expect("decryption should succeed");

    assert_eq!(
        decrypted,
        plaintext,
        "decrypted plaintext must equal original plaintext"
    );
}

#[test]
fn binary_data_round_trip() {
    let plaintext: Vec<u8> = (0..=255).collect();
    let key = b"binary-test-key";

    let encrypted = encrypt(&plaintext, key)
        .expect("binary encryption should succeed");

    let decrypted = decrypt(&encrypted, key)
        .expect("binary decryption should succeed");

    assert_eq!(
        decrypted,
        plaintext,
        "binary data must survive encryption and decryption unchanged"
    );
}

#[test]
fn empty_input_round_trip() {
    let plaintext: &[u8] = &[];
    let key = b"empty-input-key";

    let encrypted = encrypt(plaintext, key)
        .expect("empty input should be accepted");

    let decrypted = decrypt(&encrypted, key)
        .expect("empty input should decrypt");

    assert_eq!(
        decrypted,
        plaintext,
        "empty plaintext must remain empty after round-trip"
    );
}

#[test]
fn large_input_round_trip() {
    let plaintext: Vec<u8> = (0..=255)
        .cycle()
        .take(8 * 1024 * 1024)
        .collect();

    let key = b"large-input-test-key";

    let encrypted = encrypt(&plaintext, key)
        .expect("large input encryption should succeed");

    assert!(
        encrypted.len() > plaintext.len(),
        "authenticated ciphertext should contain encryption metadata"
    );

    let decrypted = decrypt(&encrypted, key)
        .expect("large input decryption should succeed");

    assert_eq!(
        decrypted,
        plaintext,
        "large binary input must survive round-trip"
    );
}

// ============================================================================
// Ciphertext Uniqueness / Nonce Randomness
// ============================================================================

#[test]
fn different_encryptions_produce_different_ciphertexts() {
    let plaintext = b"same plaintext";
    let key = b"same key";

    let encrypted_a = encrypt(plaintext, key)
        .expect("first encryption should succeed");

    let encrypted_b = encrypt(plaintext, key)
        .expect("second encryption should succeed");

    assert_ne!(
        encrypted_a,
        encrypted_b,
        "encrypting identical plaintext with identical key should normally \
         produce different ciphertexts because fresh randomness/nonces are used"
    );

    let decrypted_a = decrypt(&encrypted_a, key)
        .expect("first ciphertext should decrypt");

    let decrypted_b = decrypt(&encrypted_b, key)
        .expect("second ciphertext should decrypt");

    assert_eq!(decrypted_a, plaintext);
    assert_eq!(decrypted_b, plaintext);
}

// ============================================================================
// Authentication / Integrity
// ============================================================================

#[test]
fn wrong_key_is_rejected() {
    let plaintext = b"secret data";

    let encrypted = encrypt(
        plaintext,
        b"correct-key",
    )
    .expect("encryption should succeed");

    let error = decrypt(
        &encrypted,
        b"wrong-key",
    )
    .expect_err("decrypting with a wrong key must fail");

    assert!(
        error.status != 0,
        "wrong-key failure must return a non-success status"
    );
}

#[test]
fn corrupted_ciphertext_is_rejected() {
    let plaintext = b"integrity test";

    let mut encrypted = encrypt(
        plaintext,
        b"integrity-key",
    )
    .expect("encryption should succeed");

    assert!(
        !encrypted.is_empty(),
        "ciphertext should not be empty"
    );

    let middle_index = encrypted.len() / 2;

    encrypted[middle_index] ^= 0xFF;

    let error = decrypt(
        &encrypted,
        b"integrity-key",
    )
    .expect_err(
        "modified ciphertext must fail authentication or format validation"
    );

    assert!(
        error.status != 0,
        "corrupted ciphertext must return an error status"
    );
}

#[test]
fn truncated_ciphertext_is_rejected() {
    let plaintext = b"truncation integrity test";

    let encrypted = encrypt(
        plaintext,
        b"truncation-key",
    )
    .expect("encryption should succeed");

    assert!(
        encrypted.len() > 1,
        "ciphertext must contain enough data for truncation testing"
    );

    let truncated = &encrypted[..encrypted.len() - 1];

    let error = decrypt(
        truncated,
        b"truncation-key",
    )
    .expect_err(
        "truncated ciphertext must be rejected"
    );

    assert!(
        error.status != 0,
        "truncated ciphertext must return an error status"
    );
}

// ============================================================================
// Builder API
// ============================================================================

#[test]
fn builder_aes256_gcm_round_trip() {
    let plaintext = b"builder AES-256-GCM test";
    let key = b"builder-password";

    let builder = EncryptorBuilder::new()
        .expect("builder creation should succeed")
        .key(key)
        .expect("setting key should succeed")
        .algorithm(Algorithm::Aes256Gcm)
        .expect("AES-256-GCM should be accepted")
        .iterations(100_000)
        .expect("PBKDF2 iterations should be accepted");

    let encrypted = builder
        .encrypt(plaintext)
        .expect("builder encryption should succeed");

    let decrypted = decrypt(&encrypted, key)
        .expect("builder ciphertext should decrypt");

    assert_eq!(decrypted, plaintext);
}

#[test]
fn builder_chacha20_round_trip() {
    let plaintext = b"builder ChaCha20 test";
    let key = b"builder-password";

    let builder = EncryptorBuilder::new()
        .expect("builder creation should succeed")
        .key(key)
        .expect("setting key should succeed")
        .algorithm(Algorithm::ChaCha20)
        .expect("ChaCha20 should be accepted");

    let encrypted = builder
        .encrypt(plaintext)
        .expect("builder encryption should succeed");

    let decrypted = decrypt(&encrypted, key)
        .expect("builder ciphertext should decrypt");

    assert_eq!(decrypted, plaintext);
}

#[test]
fn builder_xchacha20_round_trip() {
    let plaintext = b"builder XChaCha20 test";
    let key = b"builder-password";

    let builder = EncryptorBuilder::new()
        .expect("builder creation should succeed")
        .key(key)
        .expect("setting key should succeed")
        .algorithm(Algorithm::XChaCha20)
        .expect("XChaCha20 should be accepted");

    let encrypted = builder
        .encrypt(plaintext)
        .expect("builder encryption should succeed");

    let decrypted = decrypt(&encrypted, key)
        .expect("builder ciphertext should decrypt");

    assert_eq!(decrypted, plaintext);
}

#[test]
fn builder_ciphertext_is_decryptable_by_default_api() {
    let plaintext = b"builder interoperability test";
    let key = b"interop-key";

    let builder = EncryptorBuilder::new()
        .expect("builder creation should succeed")
        .key(key)
        .expect("setting key should succeed")
        .algorithm(Algorithm::Aes256Gcm)
        .expect("AES-256-GCM should be accepted");

    let encrypted = builder
        .encrypt(plaintext)
        .expect("builder encryption should succeed");

    let decrypted = decrypt(
        &encrypted,
        key,
    )
    .expect(
        "ciphertext produced by builder must be compatible with default decrypt API"
    );

    assert_eq!(decrypted, plaintext);
}

// ============================================================================
// File Encryption
// ============================================================================

#[test]
fn file_round_trip() {
    let root = create_test_directory("file-round-trip");

    let input = root.join("input.bin");
    let encrypted = root.join("encrypted.bin");
    let decrypted = root.join("decrypted.bin");

    let plaintext: Vec<u8> = (0..=255)
        .cycle()
        .take(1024 * 1024)
        .collect();

    fs::write(&input, &plaintext)
        .expect("input file should be written");

    encrypt_file(
        &input,
        &encrypted,
        b"file-password",
    )
    .expect("file encryption should succeed");

    assert!(
        encrypted.exists(),
        "encrypted output file should exist"
    );

    decrypt_file(
        &encrypted,
        &decrypted,
        b"file-password",
    )
    .expect("file decryption should succeed");

    let restored = fs::read(&decrypted)
        .expect("decrypted file should be readable");

    assert_eq!(
        restored,
        plaintext,
        "decrypted file must match original file"
    );

    let _ = fs::remove_dir_all(root);
}

#[test]
fn file_wrong_key_is_rejected() {
    let root = create_test_directory("file-wrong-key");

    let input = root.join("input.bin");
    let encrypted = root.join("encrypted.bin");
    let decrypted = root.join("decrypted.bin");

    fs::write(
        &input,
        b"protected file",
    )
    .expect("input file should be written");

    encrypt_file(
        &input,
        &encrypted,
        b"correct-key",
    )
    .expect("file encryption should succeed");

    let error = decrypt_file(
        &encrypted,
        &decrypted,
        b"wrong-key",
    )
    .expect_err(
        "wrong file decryption key must fail"
    );

    assert!(
        error.status != 0,
        "wrong file key must return an error status"
    );

    let _ = fs::remove_dir_all(root);
}

#[test]
fn missing_input_file_is_rejected() {
    let root = create_test_directory("missing-input");

    let input = root.join("does-not-exist.bin");
    let encrypted = root.join("encrypted.bin");

    let error = encrypt_file(
        &input,
        &encrypted,
        b"file-password",
    )
    .expect_err(
        "encrypting a missing input file must fail"
    );

    assert!(
        error.status != 0,
        "missing input file must return an error status"
    );

    let _ = fs::remove_dir_all(root);
}

#[test]
fn missing_encrypted_file_is_rejected() {
    let root = create_test_directory("missing-encrypted");

    let encrypted = root.join("does-not-exist.bin");
    let decrypted = root.join("decrypted.bin");

    let error = decrypt_file(
        &encrypted,
        &decrypted,
        b"file-password",
    )
    .expect_err(
        "decrypting a missing encrypted file must fail"
    );

    assert!(
        error.status != 0,
        "missing encrypted file must return an error status"
    );

    let _ = fs::remove_dir_all(root);
}

// ============================================================================
// Test Error API
// ============================================================================

#[test]
fn wrong_key_error_contains_useful_information() {
    let plaintext = b"error reporting test";
    let key = b"correct-key";

    let encrypted = encrypt(
        plaintext,
        key,
    )
    .expect("encryption should succeed");

    let error = decrypt(
        &encrypted,
        b"wrong-key",
    )
    .expect_err(
        "wrong key must produce an error"
    );

    assert!(
        !error.message.is_empty(),
        "native error message must not be empty"
    );
}

#[test]
fn error_status_is_non_zero() {
    let plaintext = b"status test";

    let encrypted = encrypt(
        plaintext,
        b"correct-key",
    )
    .expect("encryption should succeed");

    let error = decrypt(
        &encrypted,
        b"wrong-key",
    )
    .expect_err(
        "wrong key must produce an error"
    );

    assert_ne!(
        error.status,
        0,
        "failed operations must never report ONC_SUCCESS"
    );
}

// ============================================================================
// Algorithm Coverage
// ============================================================================

#[test]
fn all_supported_algorithms_round_trip() {
    let plaintext = b"algorithm compatibility test";
    let key = b"algorithm-test-password";

    let algorithms = [
        Algorithm::Aes256Gcm,
        Algorithm::ChaCha20,
        Algorithm::XChaCha20,
    ];

    for algorithm in algorithms {
        let builder = EncryptorBuilder::new()
            .expect("builder creation should succeed")
            .key(key)
            .expect("setting key should succeed")
            .algorithm(algorithm)
            .expect("algorithm should be accepted");

        let encrypted = builder
            .encrypt(plaintext)
            .expect("algorithm encryption should succeed");

        let decrypted = decrypt(
            &encrypted,
            key,
        )
        .expect("algorithm ciphertext should decrypt");

        assert_eq!(
            decrypted,
            plaintext,
            "algorithm round-trip failed for {:?}",
            algorithm
        );
    }
}