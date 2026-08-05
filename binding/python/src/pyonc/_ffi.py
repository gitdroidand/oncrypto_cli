import ctypes
import os
import sys
from typing import Optional


# --- Cross-Platform Library Loader ---
def _get_library_name() -> str:
    """Returns the platform-specific library name."""
    if sys.platform.startswith("win32") or sys.platform.startswith("cygwin"):
        return "oncrypto.dll"
    elif sys.platform.startswith("darwin"):
        return "liboncrypto.dylib"
    else:
        return "liboncrypto.so"


def _load_library() -> ctypes.CDLL:
    lib_name = _get_library_name()

    # Base directory where _ffi.py is located (.../binding/python/lib/pyonc)
    pyonc_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Root directory of the repository (.../oncrypto_cli)
    repo_root = os.path.abspath(os.path.join(pyonc_dir, "..", "..", "..", ".."))

    possible_paths = [
        os.environ.get("ONCRYPTO_LIB_PATH", ""),
        # Direct paths to the build folder in root
        os.path.join(repo_root, "build", lib_name),
        os.path.join(repo_root, "build", "Release", lib_name),
        os.path.join(repo_root, "build", "Debug", lib_name),
        # Bundled inside pyonc package directory
        os.path.join(pyonc_dir, lib_name),
        # System installation paths
        os.path.join("/usr/local/lib", lib_name),
        os.path.join("/usr/lib", lib_name),
        lib_name,
    ]

    for path in possible_paths:
        if path and os.path.exists(path):
            try:
                if sys.platform.startswith("win32"):
                    return ctypes.WinDLL(path)
                return ctypes.CDLL(path)
            except OSError:
                continue

    try:
        if sys.platform.startswith("win32"):
            return ctypes.WinDLL(lib_name)
        return ctypes.CDLL(lib_name)
    except OSError as e:
        raise RuntimeError(
            f"Could not load native OnCrypto library ('{lib_name}'). "
            "Set ONCRYPTO_LIB_PATH or ensure it is installed in system paths."
        ) from e


lib = _load_library()


# --- C Struct Definitions ---
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


# Opaque Handles
onc_builder_t = ctypes.c_void_p
onc_stream_t = ctypes.c_void_p

# --- C Function Signatures ---
lib.onc_version.argtypes = []
lib.onc_version.restype = ctypes.c_char_p

lib.onc_status_to_string.argtypes = [ctypes.c_int]
lib.onc_status_to_string.restype = ctypes.c_char_p

lib.onc_get_last_error.argtypes = []
lib.onc_get_last_error.restype = ctypes.c_char_p

lib.onc_buffer_free.argtypes = [ctypes.POINTER(OncBuffer)]
lib.onc_buffer_free.restype = None

lib.onc_string_free.argtypes = [ctypes.POINTER(OncString)]
lib.onc_string_free.restype = None

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

lib.onc_builder_create.argtypes = []
lib.onc_builder_create.restype = onc_builder_t

lib.onc_builder_set_key.argtypes = [
    onc_builder_t,
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_size_t,
]
lib.onc_builder_set_key.restype = ctypes.c_int

lib.onc_builder_set_algorithm.argtypes = [onc_builder_t, ctypes.c_char_p]
lib.onc_builder_set_algorithm.restype = ctypes.c_int

lib.onc_builder_set_iterations.argtypes = [onc_builder_t, ctypes.c_uint32]
lib.onc_builder_set_iterations.restype = ctypes.c_int

lib.onc_builder_encrypt.argtypes = [
    onc_builder_t,
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_size_t,
    ctypes.POINTER(OncBuffer),
]
lib.onc_builder_encrypt.restype = ctypes.c_int

lib.onc_builder_destroy.argtypes = [onc_builder_t]
lib.onc_builder_destroy.restype = None

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

lib.onc_stream_final.argtypes = [onc_stream_t, ctypes.POINTER(OncBuffer)]
lib.onc_stream_final.restype = ctypes.c_int

lib.onc_stream_destroy.argtypes = [onc_stream_t]
lib.onc_stream_destroy.restype = None