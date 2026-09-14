use std::ffi::{c_char, c_int, c_uchar, c_void};

/// Raw buffer allocated by the native OnCrypto library.
///
/// # Safety
///
/// The memory referenced by `data` is owned by the native library and
/// must be released with `onc_buffer_free`.
///
/// Rust code must never deallocate this memory directly.
#[repr(C)]
pub struct OncBuffer {
    pub data: *mut c_uchar,
    pub size: usize,
}

/// Opaque handle to a native OnCrypto builder.
///
/// The underlying object is owned by the native library and must be
/// released with `onc_builder_destroy`.
pub type OncBuilder = *mut c_void;

/// Raw status code returned by the native OnCrypto C ABI.
pub type OncStatus = c_int;

/// Opaque handle to a native OnCrypto streaming context.
///
/// The underlying context is owned by the native library and must be
/// released with `onc_stream_destroy`.
pub type OncStream = *mut c_void;

unsafe extern "C" {
    // ========================================================================
    // Library metadata and error handling
    // ========================================================================

    /// Returns the native OnCrypto version string.
    ///
    /// The returned pointer is owned by the native library and remains valid
    /// for the lifetime of the process.
    pub fn onc_version() -> *const c_char;

    /// Converts a native status code to a human-readable string.
    ///
    /// The returned pointer is owned by the native library.
    pub fn onc_status_to_string(status: OncStatus) -> *const c_char;

    /// Returns the last error message for the calling thread.
    ///
    /// The returned pointer is owned by the native library and must not be
    /// freed by the caller.
    pub fn onc_get_last_error() -> *const c_char;

    // ========================================================================
    // Native memory management
    // ========================================================================

    /// Releases a buffer previously allocated by the native library.
    pub fn onc_buffer_free(buffer: *mut OncBuffer);

    // ========================================================================
    // One-shot buffer encryption/decryption
    // ========================================================================

    /// Encrypts a complete input buffer.
    pub fn onc_encrypt_buffer(
        input: *const c_uchar,
        input_len: usize,
        key: *const c_uchar,
        key_len: usize,
        out_buf: *mut OncBuffer,
    ) -> OncStatus;

    /// Decrypts a complete encrypted buffer.
    pub fn onc_decrypt_buffer(
        input: *const c_uchar,
        input_len: usize,
        key: *const c_uchar,
        key_len: usize,
        out_buf: *mut OncBuffer,
    ) -> OncStatus;

    // ========================================================================
    // File encryption/decryption
    // ========================================================================

    /// Encrypts a file using the native OnCrypto file API.
    pub fn onc_encrypt_file(
        src_path: *const c_char,
        dst_path: *const c_char,
        key: *const c_uchar,
        key_len: usize,
    ) -> OncStatus;

    /// Decrypts a file using the native OnCrypto file API.
    pub fn onc_decrypt_file(
        src_path: *const c_char,
        dst_path: *const c_char,
        key: *const c_uchar,
        key_len: usize,
    ) -> OncStatus;

    // ========================================================================
    // Builder API
    // ========================================================================

    /// Creates a native encryption builder.
    ///
    /// The returned handle must be released with `onc_builder_destroy`.
    pub fn onc_builder_create() -> OncBuilder;

    /// Sets the password/key used by the builder.
    pub fn onc_builder_set_key(
        builder: OncBuilder,
        key: *const c_uchar,
        key_len: usize,
    ) -> OncStatus;

    /// Sets the encryption algorithm used by the builder.
    pub fn onc_builder_set_algorithm(
        builder: OncBuilder,
        algorithm: *const c_char,
    ) -> OncStatus;

    /// Sets the PBKDF iteration count used by the builder.
    pub fn onc_builder_set_iterations(
        builder: OncBuilder,
        iterations: u32,
    ) -> OncStatus;

    /// Encrypts a complete buffer using the configured builder.
    pub fn onc_builder_encrypt(
        builder: OncBuilder,
        input: *const c_uchar,
        input_len: usize,
        out_buf: *mut OncBuffer,
    ) -> OncStatus;

    /// Destroys a native encryption builder.
    pub fn onc_builder_destroy(builder: OncBuilder);

    // ========================================================================
    // Streaming API
    // ========================================================================

    /// Creates a native streaming encryption context.
    ///
    /// The returned handle must be released with `onc_stream_destroy`.
    pub fn onc_stream_create_encryptor(
        key: *const c_uchar,
        key_len: usize,
    ) -> OncStream;

    /// Creates a native streaming decryption context.
    ///
    /// The returned handle must be released with `onc_stream_destroy`.
    pub fn onc_stream_create_decryptor(
        key: *const c_uchar,
        key_len: usize,
    ) -> OncStream;

    /// Processes one streaming input chunk.
    ///
    /// The native library allocates the output buffer. The caller owns
    /// the returned buffer and must release it with `onc_buffer_free`.
    pub fn onc_stream_update(
        stream: OncStream,
        chunk: *const c_uchar,
        chunk_len: usize,
        out_buf: *mut OncBuffer,
    ) -> OncStatus;

    /// Finalizes a streaming encryption or decryption context.
    ///
    /// Any final output produced by the native implementation is returned
    /// through `out_buf`. The caller owns the returned buffer and must
    /// release it with `onc_buffer_free`.
    pub fn onc_stream_final(
        stream: OncStream,
        out_buf: *mut OncBuffer,
    ) -> OncStatus;

    /// Destroys a native streaming context.
    ///
    /// Passing a null handle is permitted by the C ABI.
    pub fn onc_stream_destroy(stream: OncStream);
}
