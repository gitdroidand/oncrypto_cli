# OnCrypto SDK Integration

## Include path

Add the installed include directory to your compiler search path:

```bash
-I/usr/local/include
```

Typically the public header file is:

```cpp
#include <oncrypto/oncrypto.hpp>
```

## Linker options

Link the public library only:

```bash
-loncrypto
```
```

### Example with `g++`

```bash
g++ -std=c++20 main.cpp -I/usr/local/include -L/usr/local/lib -loncrypto -o myapp
```

### Example with CMake

```cmake
find_package(OnCrypto REQUIRED)
add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE OnCrypto::oncrypto)
```

## What is hidden from SDK users

- OpenSSL headers
- OpenSSL libraries
- OpenSSL symbols
- `EVP_*` APIs

SDK consumers only depend on `liboncrypto.so` or `liboncrypto.a` and the public `oncrypto/` headers.
