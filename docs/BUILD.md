# Build Instructions

## Dependencies

- CMake 3.16 or newer
- C++20-capable compiler (GCC, Clang)
- System OpenSSL development libraries with static archives
  - Example on Ubuntu: `sudo apt install libssl-dev`
- Ninja (recommended)

## Linux Build

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build --prefix /usr/local
```

The build produces:

- `build/liboncrypto_core.a` (internal static implementation library)
- `build/liboncrypto.so` (public SDK shared library)
- `build/liboncrypto.a` (public SDK static library)
- `build/oncrypto_cli` (command line tool)

## Android / Termux Notes

On mobile targets, the same SDK layering applies, but you must provide a compatible OpenSSL static library and configure CMake for the target ABI.

- Use `-DOpenSSL_USE_STATIC_LIBS=ON`
- Supply the OpenSSL static library path if needed via `-DOPENSSL_ROOT_DIR`

## Internal backend implementation

The SDK links an internal backend provider into `liboncrypto.so`, so application developers use only the OnCrypto public API and do not need to manage backend symbols directly.

The public SDK exports only OnCrypto symbols and hides backend implementation details behind a clean API boundary.
