#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Stable public engine C ABI. 
   This API is internal – it is compiled into the same library as the core.
   On Windows we do not use dllimport/dllexport; the symbols are visible internally.
   On ELF platforms we mark them with default visibility so they are not hidden
   when -fvisibility=hidden is used. */

/* Visibility macro – empty on Windows, default visibility on Unix */
#if defined(_WIN32) || defined(__CYGWIN__)
    #define ONCRYPTO_ENGINE_API
#else
    #if defined(ONCRYPTO_ENGINE_BUILD) && (defined(__ELF__) || defined(__MACH__))
        #define ONCRYPTO_ENGINE_API __attribute__((visibility("default")))
    #else
        #define ONCRYPTO_ENGINE_API
    #endif
#endif

#define ONCRYPTO_ENGINE_VERSION_MAJOR 1
#define ONCRYPTO_ENGINE_VERSION_MINOR 0

ONCRYPTO_ENGINE_API unsigned int oncrypto_engine_version_major(void);
ONCRYPTO_ENGINE_API unsigned int oncrypto_engine_version_minor(void);
ONCRYPTO_ENGINE_API const char* oncrypto_engine_version_string(void);

/* Simple C ABI for engine primitives. Return 0 on success, non-zero on failure. */

ONCRYPTO_ENGINE_API int oncrypto_engine_random_bytes(unsigned char* out, size_t out_len);

ONCRYPTO_ENGINE_API int oncrypto_engine_pbkdf2_hmac_sha256(
    const char* password,
    const unsigned char* salt,
    size_t salt_len,
    unsigned int iterations,
    unsigned char* out,
    size_t out_len
);

ONCRYPTO_ENGINE_API int oncrypto_engine_aead_encrypt(
    const char* algorithm,
    const unsigned char* key,
    size_t key_len,
    const unsigned char* nonce,
    size_t nonce_len,
    const unsigned char* plaintext,
    size_t plaintext_len,
    unsigned char* ciphertext,
    size_t* ciphertext_len,
    unsigned char* tag,
    size_t* tag_len
);

ONCRYPTO_ENGINE_API int oncrypto_engine_aead_decrypt(
    const char* algorithm,
    const unsigned char* key,
    size_t key_len,
    const unsigned char* nonce,
    size_t nonce_len,
    const unsigned char* ciphertext,
    size_t ciphertext_len,
    const unsigned char* tag,
    size_t tag_len,
    unsigned char* plaintext,
    size_t* plaintext_len
);

ONCRYPTO_ENGINE_API int oncrypto_engine_hmac_sha256(
    const unsigned char* key,
    size_t key_len,
    const unsigned char* data,
    size_t data_len,
    unsigned char* out,
    size_t* out_len
);

#ifdef __cplusplus
}
#endif