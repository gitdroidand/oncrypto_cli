// core/include/oncrypto/Export.hpp
#pragma once

// Export/import macros for the public C++ API
#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
    #ifdef ONCRYPTO_BUILD_SHARED
        #define ONCRYPTO_API __declspec(dllexport)
    #else
        #define ONCRYPTO_API __declspec(dllimport)
    #endif
#else
    #if defined(ONCRYPTO_BUILD_SHARED) && (defined(__ELF__) || defined(__MACH__))
        #define ONCRYPTO_API __attribute__((visibility("default")))
    #else
        #define ONCRYPTO_API
    #endif
#endif

// For static build, force empty
#ifdef ONCRYPTO_STATIC
    #undef ONCRYPTO_API
    #define ONCRYPTO_API
#endif