#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// OnCrypto C ABI
// ============================================================================
//
// Stable C-compatible interface for OnCrypto.
//
// The API is intentionally independent from the internal C++ implementation.
// It is designed to be consumed by:
//
//   - Rust
//   - Python
//   - C
//   - C++
//   - Kotlin/JNI
//   - Java/JNA
//   - other FFI systems
//
// Memory returned through onc_buffer must always be released with
// onc_buffer_free().
//
// Error details can be retrieved with onc_get_last_error().
//
// ============================================================================


// ============================================================================
// Status codes
// ============================================================================

typedef enum onc_status {
    ONC_SUCCESS = 0,

    ONC_ERROR_INVALID_ARGUMENT = -1,
    ONC_ERROR_ENCRYPTION_FAILED = -2,
    ONC_ERROR_DECRYPTION_FAILED = -3,
    ONC_ERROR_AUTHENTICATION_FAILED = -4,
    ONC_ERROR_INVALID_FORMAT = -5,
    ONC_ERROR_IO = -6,
    ONC_ERROR_OUT_OF_MEMORY = -7,
    ONC_ERROR_INTERNAL = -8
} onc_status;


// ============================================================================
// Opaque handles
// ============================================================================
//
// The concrete structures are intentionally hidden from consumers.
// Bindings must treat these values as opaque handles.
//

typedef struct onc_builder_s* onc_builder_t;
typedef struct onc_stream_s* onc_stream_t;


// ============================================================================
// Output buffer
// ============================================================================
//
// On success:
//
//     data -> allocated byte buffer
//     size -> number of valid bytes
//
// The caller owns the returned buffer and must release it using
// onc_buffer_free().
//
// An empty successful result is represented by:
//
//     data = NULL
//     size = 0
//

typedef struct onc_buffer {
    uint8_t* data;
    size_t size;
} onc_buffer;


// ============================================================================
// String container
// ============================================================================
//
// Reserved for C ABI APIs that return allocated strings.
//

typedef struct onc_string {
    char* str;
    size_t length;
} onc_string;


// ============================================================================
// Metadata and diagnostics
// ============================================================================

// Returns the runtime OnCrypto version.
//
// The returned pointer is owned by OnCrypto and must not be freed.
const char* onc_version(void);


// Converts a status code into a human-readable constant string.
//
// The returned pointer is owned by OnCrypto and must not be freed.
const char* onc_status_to_string(onc_status status);


// Returns the last error generated on the calling thread.
//
// The returned pointer remains valid until another OnCrypto API call modifies
// the thread-local error state.
//
// The caller must not free the returned pointer.
const char* onc_get_last_error(void);


// ============================================================================
// Memory management
// ============================================================================

// Releases a buffer previously returned by an OnCrypto API.
//
// Passing NULL is safe.
//
// After the call:
//
//     buffer->data == NULL
//     buffer->size == 0
//
void onc_buffer_free(onc_buffer* buffer);


// Releases a dynamically allocated C string.
//
// Passing NULL is safe.
//
void onc_string_free(onc_string* string);


// ============================================================================
// One-shot buffer encryption and decryption
// ============================================================================
//
// These APIs operate on complete messages.
//
// For large or incremental data processing, use the streaming API instead.
//

onc_status onc_encrypt_buffer(
    const uint8_t* input,
    size_t input_len,
    const uint8_t* key,
    size_t key_len,
    onc_buffer* out_buf
);


onc_status onc_decrypt_buffer(
    const uint8_t* input,
    size_t input_len,
    const uint8_t* key,
    size_t key_len,
    onc_buffer* out_buf
);


// ============================================================================
// File encryption and decryption
// ============================================================================
//
// These APIs operate directly on filesystem paths.
//
// The caller remains responsible for path validity and filesystem
// permissions.
//

onc_status onc_encrypt_file(
    const char* src_path,
    const char* dst_path,
    const uint8_t* key,
    size_t key_len
);


onc_status onc_decrypt_file(
    const char* src_path,
    const char* dst_path,
    const uint8_t* key,
    size_t key_len
);


// ============================================================================
// Builder API
// ============================================================================
//
// Builder objects provide configurable one-shot encryption/decryption.
//
// Supported algorithm names:
//
//     Auto
//     AES256_GCM
//     ChaCha20
//     XChaCha20
//
// The builder handle must be destroyed with onc_builder_destroy().
//

onc_builder_t onc_builder_create(void);


void onc_builder_destroy(
    onc_builder_t builder
);


onc_status onc_builder_set_key(
    onc_builder_t builder,
    const uint8_t* key,
    size_t key_len
);


onc_status onc_builder_set_algorithm(
    onc_builder_t builder,
    const char* algo_name
);


onc_status onc_builder_set_iterations(
    onc_builder_t builder,
    uint32_t iterations
);


onc_status onc_builder_encrypt(
    onc_builder_t builder,
    const uint8_t* input,
    size_t input_len,
    onc_buffer* out_buf
);


// ============================================================================
// Streaming API
// ============================================================================
//
// The streaming API processes authenticated frames instead of requiring the
// complete plaintext/ciphertext to be held in memory.
//
// Lifecycle:
//
//     onc_stream_create_encryptor()
//                 |
//                 v
//         onc_stream_update()
//                 |
//                 v
//         onc_stream_update()
//                 |
//                 v
//         onc_stream_final()
//                 |
//                 v
//         onc_stream_destroy()
//
// The decryptor follows the same lifecycle.
//
// IMPORTANT:
// Encryptor update() boundaries define encrypted frame boundaries.
//
// Decryptor update() boundaries DO NOT define frame boundaries. Arbitrary
// fragments are accepted, including fragments that split a frame header,
// authentication tag, or ciphertext.
//
// ============================================================================


// Creates a ChaCha20-Poly1305 streaming encryptor.
//
// The key is copied into the internal stream state and therefore does not
// need to remain alive after this function returns.
//
// Returns NULL on failure.
//
// The returned handle must be destroyed with onc_stream_destroy().
onc_stream_t onc_stream_create_encryptor(
    const uint8_t* key,
    size_t key_len
);


// Creates a ChaCha20-Poly1305 streaming decryptor.
//
// The key is copied into the internal stream state and therefore does not
// need to remain alive after this function returns.
//
// Returns NULL on failure.
//
// The returned handle must be destroyed with onc_stream_destroy().
onc_stream_t onc_stream_create_decryptor(
    const uint8_t* key,
    size_t key_len
);


// Processes a chunk of streaming data.
//
// Encryptor:
//
//     Each successful update() creates one authenticated encrypted frame.
//     The first update() also emits the stream header.
//
// Decryptor:
//
//     The input may contain arbitrary fragments of the encrypted stream.
//     Complete frames are authenticated and decrypted immediately.
//     Incomplete data is retained internally until the next update().
//
// On success, output data is returned through out_buf.
//
// The caller owns out_buf and must release it with onc_buffer_free().
onc_status onc_stream_update(
    onc_stream_t stream,
    const uint8_t* chunk,
    size_t chunk_len,
    onc_buffer* out_buf
);


// Finalizes the stream.
//
// Encryptor:
//
//     If no update() call has occurred, final() emits a valid empty-stream
//     header. Otherwise it produces no additional bytes.
//
// Decryptor:
//
//     final() verifies that the stream header has been received and that
//     there is no incomplete encrypted frame remaining.
//
// Calling update() after final() is invalid.
//
// Calling final() more than once is invalid.
onc_status onc_stream_final(
    onc_stream_t stream,
    onc_buffer* out_buf
);


// Destroys a streaming handle and releases all associated resources.
//
// Passing NULL is safe.
//
// After destruction, the handle must not be used again.
void onc_stream_destroy(
    onc_stream_t stream
);


#ifdef __cplusplus
}
#endif