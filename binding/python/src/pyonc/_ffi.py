from __future__ import annotations

import ctypes
import os
import sys
from pathlib import Path


# ============================================================================
# Platform
# ============================================================================

def _library_filename() -> str:
    if sys.platform.startswith(("win32", "cygwin")):
        return "oncrypto.dll"

    if sys.platform == "darwin":
        return "liboncrypto.dylib"

    return "liboncrypto.so"


def _load_library() -> ctypes.CDLL:
    filename = _library_filename()

    pyonc_dir = Path(__file__).resolve().parent

    # Repository root:
    #
    # oncrypto_cli/
    # └── binding/
    #     └── python/
    #         └── src/
    #             └── pyonc/
    #                 └── _ffi.py
    #
    # _ffi.py -> pyonc -> src -> python -> binding -> repository
    repo_root = pyonc_dir.parents[4]

    candidates: list[Path] = []

    # ------------------------------------------------------------------------
    # Explicit user override
    # ------------------------------------------------------------------------

    env_path = os.environ.get("ONCRYPTO_LIB_PATH")

    if env_path:
        candidates.append(Path(env_path).expanduser())

    # ------------------------------------------------------------------------
    # Development builds
    # ------------------------------------------------------------------------

    build_directories = (
        "build-linux",
        "build",
        "build-debug",
        "build-release",
    )

    for build_dir in build_directories:
        candidates.append(repo_root / build_dir / filename)
        candidates.append(repo_root / build_dir / "Debug" / filename)
        candidates.append(repo_root / build_dir / "Release" / filename)

    # ------------------------------------------------------------------------
    # Library bundled next to Python package
    # ------------------------------------------------------------------------

    candidates.append(pyonc_dir / filename)
    candidates.append(pyonc_dir.parent / filename)

    # ------------------------------------------------------------------------
    # Common Linux/macOS installation locations
    # ------------------------------------------------------------------------

    if not sys.platform.startswith(("win32", "cygwin")):
        candidates.extend(
            [
                Path("/usr/local/lib") / filename,
                Path("/usr/lib") / filename,
                Path("/lib") / filename,
            ]
        )

    # ------------------------------------------------------------------------
    # Try explicit filesystem paths first
    # ------------------------------------------------------------------------

    errors: list[str] = []

    for candidate in candidates:
        if not candidate.is_file():
            continue

        try:
            if sys.platform.startswith(("win32", "cygwin")):
                return ctypes.WinDLL(str(candidate))

            return ctypes.CDLL(str(candidate))

        except OSError as exc:
            errors.append(f"{candidate}: {exc}")

    # ------------------------------------------------------------------------
    # Finally ask the system dynamic linker
    # ------------------------------------------------------------------------

    try:
        if sys.platform.startswith(("win32", "cygwin")):
            return ctypes.WinDLL(filename)

        return ctypes.CDLL(filename)

    except OSError as exc:
        details = "\n".join(errors)

        raise RuntimeError(
            f"Could not load native OnCrypto library ({filename}).\n"
            f"Repository root: {repo_root}\n"
            f"Set ONCRYPTO_LIB_PATH to the full path of the native library.\n"
            + (f"\nAttempted paths:\n{details}" if details else "")
        ) from exc


lib = _load_library()


# ============================================================================
# C structures
# ============================================================================

class OncBuffer(ctypes.Structure):
    _fields_ = [
        ("data", ctypes.POINTER(ctypes.c_uint8)),
        ("size", ctypes.c_size_t),
    ]


class OncString(ctypes.Structure):
    _fields_ = [
        ("str", ctypes.c_char_p),
        ("length", ctypes.c_size_t),
    ]


# ============================================================================
# Opaque handles
# ============================================================================

onc_builder_t = ctypes.c_void_p
onc_stream_t = ctypes.c_void_p


# ============================================================================
# Function signatures
# ============================================================================

# Metadata
lib.onc_version.argtypes = []
lib.onc_version.restype = ctypes.c_char_p

lib.onc_status_to_string.argtypes = [
    ctypes.c_int,
]
lib.onc_status_to_string.restype = ctypes.c_char_p

lib.onc_get_last_error.argtypes = []
lib.onc_get_last_error.restype = ctypes.c_char_p


# ============================================================================
# Memory
# ============================================================================

lib.onc_buffer_free.argtypes = [
    ctypes.POINTER(OncBuffer),
]
lib.onc_buffer_free.restype = None

lib.onc_string_free.argtypes = [
    ctypes.POINTER(OncString),
]
lib.onc_string_free.restype = None


# ============================================================================
# Buffer encryption/decryption
# ============================================================================

lib.onc_encrypt_buffer.argtypes = [
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_size_t,
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_size_t,
    ctypes.POINTER(OncBuffer),
]
lib.onc_encrypt_buffer.restype = ctypes.c_int


lib.onc_decrypt_buffer.argtypes = [
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_size_t,
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_size_t,
    ctypes.POINTER(OncBuffer),
]
lib.onc_decrypt_buffer.restype = ctypes.c_int


# ============================================================================
# File encryption/decryption
# ============================================================================

lib.onc_encrypt_file.argtypes = [
    ctypes.c_char_p,
    ctypes.c_char_p,
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_size_t,
]
lib.onc_encrypt_file.restype = ctypes.c_int


lib.onc_decrypt_file.argtypes = [
    ctypes.c_char_p,
    ctypes.c_char_p,
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_size_t,
]
lib.onc_decrypt_file.restype = ctypes.c_int


# ============================================================================
# Builder
# ============================================================================

lib.onc_builder_create.argtypes = []
lib.onc_builder_create.restype = onc_builder_t


lib.onc_builder_set_key.argtypes = [
    onc_builder_t,
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_size_t,
]
lib.onc_builder_set_key.restype = ctypes.c_int


lib.onc_builder_set_algorithm.argtypes = [
    onc_builder_t,
    ctypes.c_char_p,
]
lib.onc_builder_set_algorithm.restype = ctypes.c_int


lib.onc_builder_set_iterations.argtypes = [
    onc_builder_t,
    ctypes.c_uint32,
]
lib.onc_builder_set_iterations.restype = ctypes.c_int


lib.onc_builder_encrypt.argtypes = [
    onc_builder_t,
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_size_t,
    ctypes.POINTER(OncBuffer),
]
lib.onc_builder_encrypt.restype = ctypes.c_int


lib.onc_builder_destroy.argtypes = [
    onc_builder_t,
]
lib.onc_builder_destroy.restype = None


# ============================================================================
# Streaming
# ============================================================================

lib.onc_stream_create_encryptor.argtypes = [
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_size_t,
]
lib.onc_stream_create_encryptor.restype = onc_stream_t


lib.onc_stream_create_decryptor.argtypes = [
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_size_t,
]
lib.onc_stream_create_decryptor.restype = onc_stream_t


lib.onc_stream_update.argtypes = [
    onc_stream_t,
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_size_t,
    ctypes.POINTER(OncBuffer),
]
lib.onc_stream_update.restype = ctypes.c_int


lib.onc_stream_final.argtypes = [
    onc_stream_t,
    ctypes.POINTER(OncBuffer),
]
lib.onc_stream_final.restype = ctypes.c_int


lib.onc_stream_destroy.argtypes = [
    onc_stream_t,
]
lib.onc_stream_destroy.restype = None