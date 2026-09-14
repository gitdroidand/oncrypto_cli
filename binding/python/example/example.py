#!/usr/bin/env python3
"""
OnCrypto Python Binding Examples
================================

Small, practical examples for using the ``pyonc`` Python binding.

This file is intentionally written as documentation-by-example rather than
as a full application. Each command demonstrates one part of the public API.

Quick start:

    python3 binding/python/example/example.py

Individual examples:

    python3 binding/python/example/example.py version
    python3 binding/python/example/example.py roundtrip
    python3 binding/python/example/example.py binary
    python3 binding/python/example/example.py builder
    python3 binding/python/example/example.py algorithms
    python3 binding/python/example/example.py file-demo
    python3 binding/python/example/example.py errors
    python3 binding/python/example/example.py stream

Integration tests:

    python3 binding/python/example/example.py test

The native library can be selected explicitly:

    ONCRYPTO_LIB_PATH="$PWD/build-linux/liboncrypto.so" \
    python3 binding/python/example/example.py

When the binding is used directly from the repository, PYTHONPATH may also
be required:

    PYTHONPATH="$PWD/binding/python/src" \
    python3 binding/python/example/example.py

For convenience, this example also adds the repository's Python source
directory to sys.path automatically when executed directly.
"""

from __future__ import annotations

import argparse
import ctypes
import os
import sys
import tempfile
from pathlib import Path
from typing import Callable


# ============================================================================
# Repository / development environment bootstrap
# ============================================================================

EXAMPLE_FILE = Path(__file__).resolve()
PYTHON_BINDING_ROOT = EXAMPLE_FILE.parents[1]
PYTHON_SRC = PYTHON_BINDING_ROOT / "src"
REPOSITORY_ROOT = EXAMPLE_FILE.parents[3]

if str(PYTHON_SRC) not in sys.path:
    sys.path.insert(0, str(PYTHON_SRC))


# If the user did not explicitly provide ONCRYPTO_LIB_PATH, try the normal
# repository build locations. This keeps the example convenient while still
# allowing a user to override the library explicitly.
def _configure_native_library_path() -> None:
    if os.environ.get("ONCRYPTO_LIB_PATH"):
        return

    candidates = [
        REPOSITORY_ROOT / "build-linux" / "liboncrypto.so",
        REPOSITORY_ROOT / "build" / "liboncrypto.so",
        REPOSITORY_ROOT / "build" / "Release" / "liboncrypto.so",
        REPOSITORY_ROOT / "build" / "Debug" / "liboncrypto.so",
    ]

    for candidate in candidates:
        if candidate.is_file():
            os.environ["ONCRYPTO_LIB_PATH"] = str(candidate)
            return


_configure_native_library_path()


import pyonc  # noqa: E402


# ============================================================================
# Formatting helpers
# ============================================================================

WIDTH = 72


def title(text: str) -> None:
    print()
    print("=" * WIDTH)
    print(text)
    print("=" * WIDTH)


def section(text: str) -> None:
    print()
    print(f"--- {text} ---")


def success(text: str) -> None:
    print(f"  ✓ {text}")


def info(text: str) -> None:
    print(f"  • {text}")


def fail(text: str) -> None:
    print(f"  ✗ {text}")


def show_bytes(label: str, data: bytes, preview: int = 48) -> None:
    if len(data) <= preview:
        print(f"  {label}: {data.hex()}")
        return

    print(
        f"  {label}: "
        f"{data[:preview].hex()}..."
        f" ({len(data)} bytes)"
    )


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


# ============================================================================
# 1. Version
# ============================================================================

def example_version() -> None:
    """
    Demonstrates:

        pyonc.version()
    """

    title("Example: Library Version")

    version = pyonc.version()

    print(f"OnCrypto version: {version}")

    require(bool(version), "Version string is empty.")

    success("pyonc.version() works.")


# ============================================================================
# 2. Basic encrypt/decrypt
# ============================================================================

def example_roundtrip(
    text: str = "Hello from OnCrypto Python!",
    password: str = "example-password",
) -> None:
    """
    Demonstrates the simplest pyonc usage:

        encrypted = pyonc.encrypt(data, password)
        plaintext = pyonc.decrypt(encrypted, password)
    """

    title("Example: Basic Encryption / Decryption")

    plaintext = text.encode("utf-8")

    print(f"Plaintext: {text}")
    print(f"Plaintext size: {len(plaintext)} bytes")

    encrypted = pyonc.encrypt(text, password)

    print(f"Encrypted size: {len(encrypted)} bytes")
    show_bytes("Ciphertext", encrypted)

    decrypted = pyonc.decrypt(encrypted, password)

    print(f"Decrypted: {decrypted.decode('utf-8')}")

    require(
        decrypted == plaintext,
        "Decrypted plaintext does not match original plaintext.",
    )

    success("encrypt() → decrypt() round trip succeeded.")


# ============================================================================
# 3. Binary-safe encryption
# ============================================================================

def example_binary(
    password: str = "binary-example-password",
) -> None:
    """
    Demonstrates that encrypt()/decrypt() operate on bytes and therefore do
    not depend on text encoding.

    This is important when encrypting files, serialized objects, compressed
    data, images, or arbitrary binary payloads.
    """

    title("Example: Binary-Safe Encryption")

    plaintext = bytes(
        [
            0x00,
            0x01,
            0x02,
            0x7F,
            0x80,
            0xFE,
            0xFF,
            0x00,
            0xAA,
            0x55,
            0x13,
            0x37,
        ]
    )

    print(f"Input size: {len(plaintext)} bytes")
    show_bytes("Input", plaintext)

    encrypted = pyonc.encrypt(plaintext, password)

    print(f"Encrypted size: {len(encrypted)} bytes")
    show_bytes("Ciphertext", encrypted)

    decrypted = pyonc.decrypt(encrypted, password)

    show_bytes("Decrypted", decrypted)

    require(
        decrypted == plaintext,
        "Binary round trip failed.",
    )

    success("Binary data survived the round trip unchanged.")


# ============================================================================
# 4. Builder API
# ============================================================================

def example_builder(
    text: str = "Builder API example",
    password: str = "builder-password",
) -> None:
    """
    Demonstrates the fluent EncryptorBuilder API.

    Example:

        encrypted = (
            pyonc.EncryptorBuilder()
            .password(password)
            .algorithm(pyonc.Algorithm.AES256_GCM)
            .iterations(100_000)
            .encrypt(data)
        )
    """

    title("Example: EncryptorBuilder")

    plaintext = text.encode("utf-8")

    builder = (
        pyonc.EncryptorBuilder()
        .password(password)
        .algorithm(pyonc.Algorithm.AES256_GCM)
        .iterations(100_000)
    )

    encrypted = builder.encrypt(plaintext)

    print(f"Plaintext: {text}")
    print(f"Encrypted size: {len(encrypted)} bytes")
    show_bytes("Ciphertext", encrypted)

    decrypted = pyonc.decrypt(encrypted, password)

    require(
        decrypted == plaintext,
        "Builder encryption round trip failed.",
    )

    success("Builder encryption succeeded.")


# ============================================================================
# 5. Algorithm selection
# ============================================================================

def example_algorithms(
    password: str = "algorithm-password",
) -> None:
    """
    Demonstrates the algorithms exposed by the Python binding.

    Note:
        This example intentionally tests the public enum/API rather than
        attempting to make claims about which algorithm is "best".
    """

    title("Example: Algorithm Selection")

    plaintext = b"Algorithm selection example."

    algorithms = [
        pyonc.Algorithm.AES256_GCM,
        pyonc.Algorithm.CHACHA20,
        pyonc.Algorithm.XCHACHA20,
    ]

    for algorithm in algorithms:
        print(f"\nAlgorithm: {algorithm.value}")

        encrypted = (
            pyonc.EncryptorBuilder()
            .password(password)
            .algorithm(algorithm)
            .iterations(100_000)
            .encrypt(plaintext)
        )

        decrypted = pyonc.decrypt(encrypted, password)

        require(
            decrypted == plaintext,
            f"{algorithm.value} round trip failed.",
        )

        print(f"  Ciphertext size: {len(encrypted)} bytes")
        success(f"{algorithm.value} round trip succeeded.")


# ============================================================================
# 6. File API
# ============================================================================

def example_file_api(
    password: str = "file-password",
) -> None:
    """
    Demonstrates:

        pyonc.encrypt_file(...)
        pyonc.decrypt_file(...)

    The paths are created inside a temporary directory so running this
    example does not leave files in the repository.
    """

    title("Example: File Encryption / Decryption")

    plaintext = (
        b"OnCrypto file API example.\n"
        b"This file contains binary-safe data.\n"
        b"\x00\x01\x02\xff"
    )

    with tempfile.TemporaryDirectory(prefix="oncrypto-example-") as temp:
        directory = Path(temp)

        source = directory / "input.bin"
        encrypted = directory / "input.bin.onc"
        decrypted = directory / "output.bin"

        source.write_bytes(plaintext)

        print(f"Source:    {source}")
        print(f"Encrypted: {encrypted}")
        print(f"Decrypted: {decrypted}")

        pyonc.encrypt_file(
            source,
            encrypted,
            password,
        )

        require(
            encrypted.exists(),
            "Encrypted file was not created.",
        )

        print(
            f"Encrypted file size: "
            f"{encrypted.stat().st_size} bytes"
        )

        pyonc.decrypt_file(
            encrypted,
            decrypted,
            password,
        )

        require(
            decrypted.exists(),
            "Decrypted file was not created.",
        )

        restored = decrypted.read_bytes()

        require(
            restored == plaintext,
            "Decrypted file does not match original.",
        )

        success("File encryption/decryption succeeded.")


# ============================================================================
# 7. Error handling
# ============================================================================

def example_errors() -> None:
    """
    Demonstrates handling OnCryptoError.

    The most important rule for applications using pyonc is to catch
    pyonc.OnCryptoError around operations that can fail.
    """

    title("Example: Error Handling")

    encrypted = pyonc.encrypt(
        b"secret payload",
        "correct-password",
    )

    print("Attempting decryption with the wrong password...")

    try:
        pyonc.decrypt(
            encrypted,
            "wrong-password",
        )

    except pyonc.OnCryptoError as error:
        print(f"  Exception type: {type(error).__name__}")
        print(f"  Error: {error}")

        require(
            error.code != 0,
            "Expected a non-success error code.",
        )

        success("Expected OnCryptoError was caught.")

    else:
        raise AssertionError(
            "Wrong password unexpectedly decrypted the payload."
        )


# ============================================================================
# 8. Stream API
# ============================================================================

def example_stream(
    password: str = "stream-password",
) -> None:
    """
    Demonstrates the current StreamSession API.

    Important:
        The current OnCrypto 1.6.0 C ABI reports that stream update/final
        operations are not implemented. Therefore this example does NOT
        pretend that real streaming encryption works.

    It demonstrates creation and safe cleanup, then explicitly shows the
    expected unsupported-operation error.
    """

    title("Example: StreamSession")

    print(
        "The StreamSession handles exist in the public API, "
        "but update()/final() are currently unsupported."
    )

    stream = pyonc.StreamSession(
        password,
        is_encrypt=True,
    )

    try:
        print("Stream session created successfully.")

        try:
            stream.update(b"stream chunk")

        except pyonc.OnCryptoError as error:
            print(f"Expected update() error: {error}")

            require(
                error.code != 0,
                "Expected stream.update() to fail.",
            )

            success(
                "Stream update correctly reported the current "
                "unsupported state."
            )

    finally:
        stream.close()

    success("Stream session was cleaned up safely.")


# ============================================================================
# 9. Context manager
# ============================================================================

def example_stream_context_manager(
    password: str = "context-password",
) -> None:
    """
    Demonstrates automatic StreamSession cleanup:

        with pyonc.StreamSession(...) as stream:
            ...
    """

    title("Example: StreamSession Context Manager")

    try:
        with pyonc.StreamSession(
            password,
            is_encrypt=True,
        ) as stream:
            print("Stream session entered.")

            try:
                stream.update(b"example")

            except pyonc.OnCryptoError as error:
                print(f"Expected update() error: {error}")

    finally:
        print("Context manager exited and session was released.")

    success("StreamSession context-manager usage works.")


# ============================================================================
# 10. Practical file workflow
# ============================================================================

def example_file_workflow(
    password: str = "workflow-password",
) -> None:
    """
    A slightly more realistic example showing how an application can use
    the file API without manually managing native buffers.
    """

    title("Example: Practical File Workflow")

    original = (
        b"Configuration data\n"
        b"database.host=localhost\n"
        b"database.port=5432\n"
        b"\x00binary-marker\xff"
    )

    with tempfile.TemporaryDirectory(prefix="oncrypto-workflow-") as temp:
        directory = Path(temp)

        source = directory / "config.dat"
        vault = directory / "config.dat.onc"
        restored = directory / "config.restored"

        source.write_bytes(original)

        print("Encrypting file...")
        pyonc.encrypt_file(
            source,
            vault,
            password,
        )

        print("Decrypting file...")
        pyonc.decrypt_file(
            vault,
            restored,
            password,
        )

        result = restored.read_bytes()

        require(
            result == original,
            "File workflow round trip failed.",
        )

        print(f"Original size:  {len(original)} bytes")
        print(f"Encrypted size: {vault.stat().st_size} bytes")
        print(f"Restored size:  {len(result)} bytes")

        success("Complete file workflow succeeded.")


# ============================================================================
# 11. Quick Start
# ============================================================================

def run_quick_start() -> None:
    """
    The default example.

    This is deliberately short. A developer should be able to run the file
    with no arguments and immediately see the most important pyonc usage.
    """

    title("OnCrypto Python Binding — Quick Start")

    print(f"pyonc version: {pyonc.version()}")

    section("Encrypt")

    plaintext = b"Hello from pyonc!"
    password = "my-secret-password"

    print(f"Plaintext: {plaintext!r}")

    encrypted = pyonc.encrypt(
        plaintext,
        password,
    )

    print(f"Ciphertext size: {len(encrypted)} bytes")
    show_bytes("Ciphertext", encrypted)

    section("Decrypt")

    decrypted = pyonc.decrypt(
        encrypted,
        password,
    )

    print(f"Decrypted: {decrypted!r}")

    require(
        decrypted == plaintext,
        "Quick-start round trip failed.",
    )

    success("Basic pyonc encryption/decryption works.")

    print()
    print("Next examples:")
    print("  version")
    print("  roundtrip")
    print("  binary")
    print("  builder")
    print("  algorithms")
    print("  file-demo")
    print("  errors")
    print("  stream")
    print("  stream-context")
    print("  workflow")
    print("  test")


# ============================================================================
# 12. Integration test suite
# ============================================================================

def run_tests() -> None:
    """
    Runs the examples as a small integration test suite.

    This is intentionally not a replacement for the native C/C++ test suite.
    It verifies that the Python binding correctly reaches the C ABI and that
    the high-level Python API behaves as documented.
    """

    title("pyonc Python Binding Integration Tests")

    tests: list[tuple[str, Callable[[], None]]] = [
        (
            "version",
            example_version,
        ),
        (
            "basic round trip",
            lambda: example_roundtrip(),
        ),
        (
            "binary data",
            lambda: example_binary(),
        ),
        (
            "builder",
            lambda: example_builder(),
        ),
        (
            "algorithms",
            lambda: example_algorithms(),
        ),
        (
            "file API",
            lambda: example_file_api(),
        ),
        (
            "error handling",
            example_errors,
        ),
        (
            "stream session",
            example_stream,
        ),
        (
            "stream context manager",
            example_stream_context_manager,
        ),
        (
            "file workflow",
            example_file_workflow,
        ),
    ]

    passed = 0

    for name, test in tests:
        print()
        print(f"[TEST] {name}")

        try:
            test()

        except Exception as error:
            fail(f"{name}: {error}")
            raise

        else:
            passed += 1
            success(f"{name} passed.")

    print()
    print("=" * WIDTH)
    print(f"Python binding tests: {passed}/{len(tests)} passed")
    print("=" * WIDTH)

    success("ALL PYONC INTEGRATION TESTS PASSED.")


# ============================================================================
# CLI
# ============================================================================

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="example.py",
        description=(
            "Practical documentation examples for the OnCrypto "
            "Python binding (pyonc)."
        ),
        epilog=(
            "Run without a command to execute the Quick Start example."
        ),
    )

    parser.add_argument(
        "--key",
        default="example-password",
        help="Password used by examples that accept a password.",
    )

    subparsers = parser.add_subparsers(
        dest="command",
    )

    subparsers.add_parser(
        "version",
        help="Show the OnCrypto library version.",
    )

    roundtrip_parser = subparsers.add_parser(
        "roundtrip",
        help="Demonstrate encrypt() and decrypt().",
    )

    roundtrip_parser.add_argument(
        "text",
        nargs="?",
        default="Hello from OnCrypto Python!",
        help="Text to encrypt.",
    )

    subparsers.add_parser(
        "binary",
        help="Demonstrate binary-safe encryption.",
    )

    builder_parser = subparsers.add_parser(
        "builder",
        help="Demonstrate EncryptorBuilder.",
    )

    builder_parser.add_argument(
        "text",
        nargs="?",
        default="Builder API example",
        help="Text to encrypt.",
    )

    subparsers.add_parser(
        "algorithms",
        help="Demonstrate supported algorithm selection.",
    )

    subparsers.add_parser(
        "file-demo",
        help="Demonstrate encrypt_file() and decrypt_file().",
    )

    subparsers.add_parser(
        "errors",
        help="Demonstrate OnCryptoError handling.",
    )

    subparsers.add_parser(
        "stream",
        help="Demonstrate the current StreamSession API.",
    )

    subparsers.add_parser(
        "stream-context",
        help="Demonstrate StreamSession as a context manager.",
    )

    subparsers.add_parser(
        "workflow",
        help="Demonstrate a practical file workflow.",
    )

    subparsers.add_parser(
        "test",
        help="Run all Python binding integration tests.",
    )

    subparsers.add_parser(
        "demo",
        help="Run the Quick Start demonstration.",
    )

    return parser


# ============================================================================
# Main
# ============================================================================

def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    try:
        # No command means Quick Start.
        if args.command is None:
            run_quick_start()
            return 0

        if args.command == "version":
            example_version()

        elif args.command == "roundtrip":
            example_roundtrip(
                text=args.text,
                password=args.key,
            )

        elif args.command == "binary":
            example_binary(
                password=args.key,
            )

        elif args.command == "builder":
            example_builder(
                text=args.text,
                password=args.key,
            )

        elif args.command == "algorithms":
            example_algorithms(
                password=args.key,
            )

        elif args.command == "file-demo":
            example_file_api(
                password=args.key,
            )

        elif args.command == "errors":
            example_errors()

        elif args.command == "stream":
            example_stream(
                password=args.key,
            )

        elif args.command == "stream-context":
            example_stream_context_manager(
                password=args.key,
            )

        elif args.command == "workflow":
            example_file_workflow(
                password=args.key,
            )

        elif args.command == "test":
            run_tests()

        elif args.command == "demo":
            run_quick_start()

        else:
            parser.error(f"Unknown command: {args.command}")

        return 0

    except pyonc.OnCryptoError as error:
        print()
        fail("OnCrypto operation failed.")
        print(f"  {error}")
        return 1

    except AssertionError as error:
        print()
        fail(f"Example assertion failed: {error}")
        return 1

    except OSError as error:
        print()
        fail(f"Filesystem/native-library error: {error}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())