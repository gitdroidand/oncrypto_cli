use std::ffi::{CStr, CString};
use std::fmt;
use std::path::Path;

use crate::ffi;

const ONC_SUCCESS: i32 = 0;

const ONC_ERROR_INVALID_ARGUMENT: i32 = -1;
const ONC_ERROR_OUT_OF_MEMORY: i32 = -7;
const ONC_ERROR_INTERNAL: i32 = -8;

/// Algorithms supported by liboncrypto's builder API.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Algorithm {
    /// Let liboncrypto select the appropriate algorithm.
    Auto,

    /// AES-256-GCM.
    Aes256Gcm,

    /// ChaCha20-Poly1305.
    ChaCha20,

    /// XChaCha20-Poly1305.
    XChaCha20,
}

impl Algorithm {
    fn as_c_str(self) -> &'static CStr {
        match self {
            Self::Auto => c"Auto",
            Self::Aes256Gcm => c"AES256_GCM",
            Self::ChaCha20 => c"ChaCha20",
            Self::XChaCha20 => c"XChaCha20",
        }
    }
}

/// Error returned by the safe Rust API.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct OnCryptoError {
    /// Native status code returned by liboncrypto.
    pub status: i32,

    /// Human-readable error message.
    pub message: String,
}

impl fmt::Display for OnCryptoError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{} (status {})", self.message, self.status)
    }
}

impl std::error::Error for OnCryptoError {}

/// Converts a native OnCrypto status code to its human-readable name.
fn native_status_name(status: i32) -> String {
    unsafe {
        let ptr = ffi::onc_status_to_string(status);

        if ptr.is_null() {
            return "Unknown error".to_owned();
        }

        CStr::from_ptr(ptr)
            .to_string_lossy()
            .into_owned()
    }
}

/// Retrieves the most recent native error and converts it into a safe
/// Rust error value.
///
/// The native error message is preferred. If the native library does not
/// provide a message, the status code is converted using
/// `onc_status_to_string`.
fn last_error(status: i32) -> OnCryptoError {
    unsafe {
        let message_ptr = ffi::onc_get_last_error();

        let message = if message_ptr.is_null() {
            native_status_name(status)
        } else {
            let message = CStr::from_ptr(message_ptr)
                .to_string_lossy()
                .into_owned();

            if message.is_empty() {
                native_status_name(status)
            } else {
                message
            }
        };

        OnCryptoError { status, message }
    }
}

/// Converts a native status into a Rust `Result`.
fn check_status(status: i32) -> Result<(), OnCryptoError> {
    if status == ONC_SUCCESS {
        Ok(())
    } else {
        Err(last_error(status))
    }
}

/// Executes a native operation that writes its result into an `OncBuffer`
/// and safely transfers the resulting bytes into a Rust `Vec<u8>`.
fn output_buffer(
    operation: impl FnOnce(*mut ffi::OncBuffer) -> i32,
) -> Result<Vec<u8>, OnCryptoError> {
    let mut buffer = ffi::OncBuffer {
        data: std::ptr::null_mut(),
        size: 0,
    };

    let status = operation(&mut buffer);

    if status != ONC_SUCCESS {
        unsafe {
            if !buffer.data.is_null() {
                ffi::onc_buffer_free(&mut buffer);
            }
        }

        return Err(last_error(status));
    }

    if buffer.data.is_null() && buffer.size != 0 {
        return Err(OnCryptoError {
            status: ONC_ERROR_INTERNAL,
            message: "liboncrypto returned an invalid output buffer.".into(),
        });
    }

    let result = if buffer.size == 0 {
        Vec::new()
    } else {
        unsafe {
            std::slice::from_raw_parts(buffer.data, buffer.size).to_vec()
        }
    };

    unsafe {
        ffi::onc_buffer_free(&mut buffer);
    }

    Ok(result)
}

/// Returns the version reported by liboncrypto.
///
/// # Examples
///
/// ```
/// let version = oncrypto::version();
///
/// assert!(!version.is_empty());
/// ```
pub fn version() -> String {
    unsafe {
        let ptr = ffi::onc_version();

        if ptr.is_null() {
            return "unknown".into();
        }

        CStr::from_ptr(ptr)
            .to_string_lossy()
            .into_owned()
    }
}

/// Encrypts arbitrary bytes using liboncrypto's default encryption API.
///
/// This is the simplest API and is usually the right choice when the
/// caller does not need builder configuration.
///
/// # Examples
///
/// ```
/// use oncrypto::{decrypt, encrypt};
///
/// let plaintext = b"hello from Rust";
/// let password = b"correct horse battery staple";
///
/// let encrypted = encrypt(plaintext, password).unwrap();
/// let decrypted = decrypt(&encrypted, password).unwrap();
///
/// assert_eq!(decrypted, plaintext);
/// ```
pub fn encrypt(
    input: &[u8],
    key: &[u8],
) -> Result<Vec<u8>, OnCryptoError> {
    output_buffer(|output| unsafe {
        ffi::onc_encrypt_buffer(
            input.as_ptr(),
            input.len(),
            key.as_ptr(),
            key.len(),
            output,
        )
    })
}

/// Decrypts bytes previously encrypted by liboncrypto.
///
/// Authentication, format validation, and decryption are performed
/// entirely by the native OnCrypto library.
pub fn decrypt(
    input: &[u8],
    key: &[u8],
) -> Result<Vec<u8>, OnCryptoError> {
    output_buffer(|output| unsafe {
        ffi::onc_decrypt_buffer(
            input.as_ptr(),
            input.len(),
            key.as_ptr(),
            key.len(),
            output,
        )
    })
}

/// Encrypts a file using liboncrypto's native file API.
///
/// The file is processed by the native library. The Rust binding does not
/// load the complete file into memory.
pub fn encrypt_file(
    source: impl AsRef<Path>,
    destination: impl AsRef<Path>,
    key: &[u8],
) -> Result<(), OnCryptoError> {
    let source = path_to_cstring(source.as_ref())?;
    let destination = path_to_cstring(destination.as_ref())?;

    let status = unsafe {
        ffi::onc_encrypt_file(
            source.as_ptr(),
            destination.as_ptr(),
            key.as_ptr(),
            key.len(),
        )
    };

    check_status(status)
}

/// Decrypts a file using liboncrypto's native file API.
///
/// The file is processed by the native library. The Rust binding does not
/// load the complete file into memory.
pub fn decrypt_file(
    source: impl AsRef<Path>,
    destination: impl AsRef<Path>,
    key: &[u8],
) -> Result<(), OnCryptoError> {
    let source = path_to_cstring(source.as_ref())?;
    let destination = path_to_cstring(destination.as_ref())?;

    let status = unsafe {
        ffi::onc_decrypt_file(
            source.as_ptr(),
            destination.as_ptr(),
            key.as_ptr(),
            key.len(),
        )
    };

    check_status(status)
}

/// Converts a Rust filesystem path into a C-compatible string.
///
/// The native C ABI expects NUL-terminated paths, therefore paths
/// containing an embedded NUL byte are rejected.
fn path_to_cstring(
    path: &Path,
) -> Result<CString, OnCryptoError> {
    let text = path.to_string_lossy();

    CString::new(text.as_bytes()).map_err(|_| OnCryptoError {
        status: ONC_ERROR_INVALID_ARGUMENT,
        message: "Path contains an embedded NUL byte.".into(),
    })
}

/// Configurable encryption builder backed by liboncrypto.
///
/// The builder does not implement cryptography itself. Every operation
/// is forwarded to the native OnCrypto library through the C ABI.
pub struct EncryptorBuilder {
    raw: ffi::OncBuilder,
}

impl EncryptorBuilder {
    /// Creates a new native encryptor builder.
    pub fn new() -> Result<Self, OnCryptoError> {
        let raw = unsafe { ffi::onc_builder_create() };

        if raw.is_null() {
            Err(last_error(ONC_ERROR_OUT_OF_MEMORY))
        } else {
            Ok(Self { raw })
        }
    }

    /// Sets the password/key used by the native encryptor.
    pub fn key(
        self,
        key: &[u8],
    ) -> Result<Self, OnCryptoError> {
        let status = unsafe {
            ffi::onc_builder_set_key(
                self.raw,
                key.as_ptr(),
                key.len(),
            )
        };

        check_status(status)?;

        Ok(self)
    }

    /// Selects the encryption algorithm.
    pub fn algorithm(
        self,
        algorithm: Algorithm,
    ) -> Result<Self, OnCryptoError> {
        let status = unsafe {
            ffi::onc_builder_set_algorithm(
                self.raw,
                algorithm.as_c_str().as_ptr(),
            )
        };

        check_status(status)?;

        Ok(self)
    }

    /// Sets the PBKDF2 iteration count used by the native builder.
    pub fn iterations(
        self,
        iterations: u32,
    ) -> Result<Self, OnCryptoError> {
        let status = unsafe {
            ffi::onc_builder_set_iterations(
                self.raw,
                iterations,
            )
        };

        check_status(status)?;

        Ok(self)
    }

    /// Encrypts bytes using the current builder configuration.
    pub fn encrypt(
        &self,
        input: &[u8],
    ) -> Result<Vec<u8>, OnCryptoError> {
        output_buffer(|output| unsafe {
            ffi::onc_builder_encrypt(
                self.raw,
                input.as_ptr(),
                input.len(),
                output,
            )
        })
    }
}

impl Drop for EncryptorBuilder {
    fn drop(&mut self) {
        unsafe {
            ffi::onc_builder_destroy(self.raw);
        }
    }
}

/// Creates a new configurable native encryptor builder.
///
/// This is a convenience wrapper around [`EncryptorBuilder::new`].
pub fn builder() -> Result<EncryptorBuilder, OnCryptoError> {
    EncryptorBuilder::new()
}