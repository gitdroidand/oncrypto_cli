from enum import Enum
from pathlib import Path
from typing import Union, Optional
import ctypes

from ._ffi import lib, OncBuffer, onc_builder_t, onc_stream_t


class OnCryptoError(Exception):
    """Base exception class for OnCrypto operations."""

    def __init__(self, code: int, message: str = ""):
        self.code = code
        status_str = lib.onc_status_to_string(code).decode("utf-8")
        last_err = lib.onc_get_last_error().decode("utf-8")
        detail = message or last_err or status_str
        super().__init__(f"OnCrypto Error [{code} - {status_str}]: {detail}")


class Algorithm(str, Enum):
    AUTO = "Auto"
    AES256_GCM = "AES256_GCM"
    CHACHA20 = "ChaCha20"
    XCHACHA20 = "XChaCha20"


def version() -> str:
    """Returns the library version."""
    return lib.onc_version().decode("utf-8")


def _to_bytes(data: Union[str, bytes, bytearray]) -> bytes:
    if isinstance(data, str):
        return data.encode("utf-8")
    return bytes(data)


def _check_status(status_code: int):
    if status_code != 0:
        raise OnCryptoError(status_code)


def encrypt(data: Union[str, bytes], password: Union[str, bytes]) -> bytes:
    """Encrypts raw data/string with a password."""
    data_bytes = _to_bytes(data)
    pwd_bytes = _to_bytes(password)

    in_ptr = (ctypes.c_uint8 * len(data_bytes)).from_buffer_copy(data_bytes)
    pwd_ptr = (ctypes.c_uint8 * len(pwd_bytes)).from_buffer_copy(pwd_bytes)

    out_buf = OncBuffer()
    status = lib.onc_encrypt_buffer(
        in_ptr, len(data_bytes), pwd_ptr, len(pwd_bytes), ctypes.byref(out_buf)
    )
    _check_status(status)

    try:
        res = ctypes.string_at(out_buf.data, out_buf.size)
        return res
    finally:
        lib.onc_buffer_free(ctypes.byref(out_buf))


def decrypt(data: bytes, password: Union[str, bytes]) -> bytes:
    """Decrypts encrypted bytes back to original raw bytes."""
    pwd_bytes = _to_bytes(password)

    in_ptr = (ctypes.c_uint8 * len(data)).from_buffer_copy(data)
    pwd_ptr = (ctypes.c_uint8 * len(pwd_bytes)).from_buffer_copy(pwd_bytes)

    out_buf = OncBuffer()
    status = lib.onc_decrypt_buffer(
        in_ptr, len(data), pwd_ptr, len(pwd_bytes), ctypes.byref(out_buf)
    )
    _check_status(status)

    try:
        res = ctypes.string_at(out_buf.data, out_buf.size)
        return res
    finally:
        lib.onc_buffer_free(ctypes.byref(out_buf))


def encrypt_file(
    src_path: Union[str, Path],
    dst_path: Union[str, Path],
    password: Union[str, bytes],
) -> None:
    """Encrypts a file on disk."""
    pwd_bytes = _to_bytes(password)
    pwd_ptr = (ctypes.c_uint8 * len(pwd_bytes)).from_buffer_copy(pwd_bytes)

    status = lib.onc_encrypt_file(
        str(src_path).encode("utf-8"),
        str(dst_path).encode("utf-8"),
        pwd_ptr,
        len(pwd_bytes),
    )
    _check_status(status)


def decrypt_file(
    src_path: Union[str, Path],
    dst_path: Union[str, Path],
    password: Union[str, bytes],
) -> None:
    """Decrypts a file on disk."""
    pwd_bytes = _to_bytes(password)
    pwd_ptr = (ctypes.c_uint8 * len(pwd_bytes)).from_buffer_copy(pwd_bytes)

    status = lib.onc_decrypt_file(
        str(src_path).encode("utf-8"),
        str(dst_path).encode("utf-8"),
        pwd_ptr,
        len(pwd_bytes),
    )
    _check_status(status)


class EncryptorBuilder:
    """Fluent Builder Pattern for custom encryption setups."""

    def __init__(self):
        self._handle = lib.onc_builder_create()
        if not self._handle:
            raise OnCryptoError(7, "Failed to allocate builder context.")

    def password(self, pwd: Union[str, bytes]) -> "EncryptorBuilder":
        pwd_bytes = _to_bytes(pwd)
        pwd_ptr = (ctypes.c_uint8 * len(pwd_bytes)).from_buffer_copy(pwd_bytes)
        status = lib.onc_builder_set_key(self._handle, pwd_ptr, len(pwd_bytes))
        _check_status(status)
        return self

    def algorithm(self, algo: Union[Algorithm, str]) -> "EncryptorBuilder":
        algo_str = algo.value if isinstance(algo, Algorithm) else str(algo)
        status = lib.onc_builder_set_algorithm(
            self._handle, algo_str.encode("utf-8")
        )
        _check_status(status)
        return self

    def iterations(self, count: int) -> "EncryptorBuilder":
        status = lib.onc_builder_set_iterations(self._handle, count)
        _check_status(status)
        return self

    def encrypt(self, data: Union[str, bytes]) -> bytes:
        data_bytes = _to_bytes(data)
        in_ptr = (ctypes.c_uint8 * len(data_bytes)).from_buffer_copy(data_bytes)

        out_buf = OncBuffer()
        status = lib.onc_builder_encrypt(
            self._handle, in_ptr, len(data_bytes), ctypes.byref(out_buf)
        )
        _check_status(status)

        try:
            return ctypes.string_at(out_buf.data, out_buf.size)
        finally:
            lib.onc_buffer_free(ctypes.byref(out_buf))

    def __del__(self):
        if hasattr(self, "_handle") and self._handle:
            lib.onc_builder_destroy(self._handle)
            self._handle = None


class StreamSession:
    """Context-Managed Streaming Session for large payloads."""

    def __init__(self, password: Union[str, bytes], is_encrypt: bool = True):
        pwd_bytes = _to_bytes(password)
        pwd_ptr = (ctypes.c_uint8 * len(pwd_bytes)).from_buffer_copy(pwd_bytes)

        if is_encrypt:
            self._handle = lib.onc_stream_create_encryptor(
                pwd_ptr, len(pwd_bytes)
            )
        else:
            self._handle = lib.onc_stream_create_decryptor(
                pwd_ptr, len(pwd_bytes)
            )

        if not self._handle:
            raise OnCryptoError(7, "Failed to create stream session.")

    def update(self, chunk: bytes) -> bytes:
        in_ptr = (ctypes.c_uint8 * len(chunk)).from_buffer_copy(chunk)
        out_buf = OncBuffer()

        status = lib.onc_stream_update(
            self._handle, in_ptr, len(chunk), ctypes.byref(out_buf)
        )
        _check_status(status)

        try:
            if out_buf.size > 0 and out_buf.data:
                return ctypes.string_at(out_buf.data, out_buf.size)
            return b""
        finally:
            lib.onc_buffer_free(ctypes.byref(out_buf))

    def final(self) -> bytes:
        out_buf = OncBuffer()
        status = lib.onc_stream_final(self._handle, ctypes.byref(out_buf))
        _check_status(status)

        try:
            if out_buf.size > 0 and out_buf.data:
                return ctypes.string_at(out_buf.data, out_buf.size)
            return b""
        finally:
            lib.onc_buffer_free(ctypes.byref(out_buf))

    def close(self):
        if hasattr(self, "_handle") and self._handle:
            lib.onc_stream_destroy(self._handle)
            self._handle = None

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def __del__(self):
        self.close()