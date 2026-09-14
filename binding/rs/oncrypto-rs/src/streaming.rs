
use crate::ffi;
use crate::oncrypto::OnCryptoError;

const ONC_SUCCESS: i32 = 0;

const ONC_ERROR_INVALID_ARGUMENT: i32 = -1;
const ONC_ERROR_ENCRYPTION_FAILED: i32 = -2;
const ONC_ERROR_DECRYPTION_FAILED: i32 = -3;
const ONC_ERROR_AUTHENTICATION_FAILED: i32 = -4;
const ONC_ERROR_INVALID_FORMAT: i32 = -5;
const ONC_ERROR_IO: i32 = -6;
const ONC_ERROR_OUT_OF_MEMORY: i32 = -7;
const ONC_ERROR_INTERNAL: i32 = -8;

/// Converts a native streaming status into a safe Rust error.
///
/// The actual human-readable error is retrieved from the native
/// thread-local error storage.
fn last_error(status: i32) -> OnCryptoError {
    unsafe {
        let message = ffi::onc_get_last_error();

        let message = if message.is_null() {
            match status {
                ONC_ERROR_INVALID_ARGUMENT => "Invalid argument",
                ONC_ERROR_ENCRYPTION_FAILED => "Encryption failed",
                ONC_ERROR_DECRYPTION_FAILED => "Decryption failed",
                ONC_ERROR_AUTHENTICATION_FAILED => "Authentication failed",
                ONC_ERROR_INVALID_FORMAT => "Invalid format",
                ONC_ERROR_IO => "I/O error",
                ONC_ERROR_OUT_OF_MEMORY => "Out of memory",
                ONC_ERROR_INTERNAL => "Internal error",
                _ => "Unknown error",
            }
            .to_owned()
        } else {
            std::ffi::CStr::from_ptr(message)
                .to_string_lossy()
                .into_owned()
        };

        OnCryptoError { status, message }
    }
}

/// Converts a native output buffer into an owned Rust `Vec<u8>`.
///
/// The native buffer is always released through `onc_buffer_free`,
/// regardless of whether the buffer contains data.
///
/// # Safety
///
/// The caller must provide a valid native `OncBuffer` returned by one
/// of the OnCrypto C ABI functions.
unsafe fn take_buffer(
    buffer: &mut ffi::OncBuffer,
) -> Result<Vec<u8>, OnCryptoError> {
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
            std::slice::from_raw_parts(
                buffer.data,
                buffer.size,
            )
            .to_vec()
        }
    };

    unsafe {
        ffi::onc_buffer_free(buffer);
    }

    Ok(result)
}

/// Executes a native streaming operation and takes ownership of its output.
fn execute_stream_operation(
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

    unsafe { take_buffer(&mut buffer) }
}

// ============================================================================
// EncryptStream
// ============================================================================

/// Stateful streaming encryptor backed by the native OnCrypto C ABI.
///
/// `EncryptStream` does not implement cryptography itself. It owns an opaque
/// native streaming context and forwards input chunks to liboncrypto.
///
/// The stream must be finalized with [`EncryptStream::finalize`] after the
/// final input chunk has been processed.
///
/// # Example
///
/// ```
/// use oncrypto::EncryptStream;
///
/// let mut stream = EncryptStream::new(b"secret-password")?;
///
/// let mut encrypted = Vec::new();
///
/// encrypted.extend_from_slice(stream.update(b"Hello ")? .as_slice());
/// encrypted.extend_from_slice(stream.update(b"OnCrypto")?.as_slice());
/// encrypted.extend_from_slice(stream.finalize()?.as_slice());
///
/// # Ok::<(), oncrypto::OnCryptoError>(())
/// ```
pub struct EncryptStream {
    raw: ffi::OncStream,
    finalized: bool,
}

impl EncryptStream {
    /// Creates a new streaming encryptor.
    ///
    /// The key is copied into the native streaming context by the C ABI.
    pub fn new(key: &[u8]) -> Result<Self, OnCryptoError> {
        let raw = unsafe {
            ffi::onc_stream_create_encryptor(
                key.as_ptr(),
                key.len(),
            )
        };

        if raw.is_null() {
            return Err(last_error(ONC_ERROR_OUT_OF_MEMORY));
        }

        Ok(Self {
            raw,
            finalized: false,
        })
    }

    /// Encrypts one chunk of input.
    ///
    /// The returned `Vec<u8>` is owned entirely by Rust after this method
    /// returns. The caller may therefore store, modify, or forward it
    /// without keeping the native output buffer alive.
    ///
    /// Calling `update` after [`Self::finalize`] returns an error.
    pub fn update(&mut self, chunk: &[u8]) -> Result<Vec<u8>, OnCryptoError> {
        if self.finalized {
            return Err(OnCryptoError {
                status: ONC_ERROR_INVALID_ARGUMENT,
                message: "Streaming encryptor has already been finalized.".into(),
            });
        }

        execute_stream_operation(|output| unsafe {
            ffi::onc_stream_update(
                self.raw,
                chunk.as_ptr(),
                chunk.len(),
                output,
            )
        })
    }

    /// Finalizes the encryption stream.
    ///
    /// Any final output produced by the native implementation is returned.
    ///
    /// After this method succeeds, no additional calls to [`Self::update`]
    /// are permitted.
    pub fn finalize(&mut self) -> Result<Vec<u8>, OnCryptoError> {
        if self.finalized {
            return Err(OnCryptoError {
                status: ONC_ERROR_INVALID_ARGUMENT,
                message: "Streaming encryptor has already been finalized.".into(),
            });
        }

        let result = execute_stream_operation(|output| unsafe {
            ffi::onc_stream_final(
                self.raw,
                output,
            )
        });

        if result.is_ok() {
            self.finalized = true;
        }

        result
    }

    /// Returns whether the stream has already been finalized.
    pub fn is_finalized(&self) -> bool {
        self.finalized
    }
}

impl Drop for EncryptStream {
    fn drop(&mut self) {
        if !self.raw.is_null() {
            unsafe {
                ffi::onc_stream_destroy(self.raw);
            }

            self.raw = std::ptr::null_mut();
        }
    }
}

// ============================================================================
// DecryptStream
// ============================================================================

/// Stateful streaming decryptor backed by the native OnCrypto C ABI.
///
/// `DecryptStream` owns an opaque native streaming context and forwards
/// encrypted chunks to liboncrypto.
///
/// Authentication and decryption are performed by the native library.
///
/// The stream must be finalized with [`DecryptStream::finalize`] after the
/// final encrypted chunk has been processed.
pub struct DecryptStream {
    raw: ffi::OncStream,
    finalized: bool,
}

impl DecryptStream {
    /// Creates a new streaming decryptor.
    ///
    /// The key is copied into the native streaming context by the C ABI.
    pub fn new(key: &[u8]) -> Result<Self, OnCryptoError> {
        let raw = unsafe {
            ffi::onc_stream_create_decryptor(
                key.as_ptr(),
                key.len(),
            )
        };

        if raw.is_null() {
            return Err(last_error(ONC_ERROR_OUT_OF_MEMORY));
        }

        Ok(Self {
            raw,
            finalized: false,
        })
    }

    /// Decrypts one encrypted chunk.
    ///
    /// The returned bytes are owned by Rust.
    ///
    /// Calling `update` after [`Self::finalize`] returns an error.
    pub fn update(
        &mut self,
        chunk: &[u8],
    ) -> Result<Vec<u8>, OnCryptoError> {
        if self.finalized {
            return Err(OnCryptoError {
                status: ONC_ERROR_INVALID_ARGUMENT,
                message: "Streaming decryptor has already been finalized.".into(),
            });
        }

        execute_stream_operation(|output| unsafe {
            ffi::onc_stream_update(
                self.raw,
                chunk.as_ptr(),
                chunk.len(),
                output,
            )
        })
    }

    /// Finalizes the decryption stream.
    ///
    /// Any final plaintext produced by the native implementation is returned.
    ///
    /// Authentication failures are reported through [`OnCryptoError`].
    pub fn finalize(&mut self) -> Result<Vec<u8>, OnCryptoError> {
        if self.finalized {
            return Err(OnCryptoError {
                status: ONC_ERROR_INVALID_ARGUMENT,
                message: "Streaming decryptor has already been finalized.".into(),
            });
        }

        let result = execute_stream_operation(|output| unsafe {
            ffi::onc_stream_final(
                self.raw,
                output,
            )
        });

        if result.is_ok() {
            self.finalized = true;
        }

        result
    }

    /// Returns whether the stream has already been finalized.
    pub fn is_finalized(&self) -> bool {
        self.finalized
    }
}

impl Drop for DecryptStream {
    fn drop(&mut self) {
        if !self.raw.is_null() {
            unsafe {
                ffi::onc_stream_destroy(self.raw);
            }

            self.raw = std::ptr::null_mut();
        }
    }
}