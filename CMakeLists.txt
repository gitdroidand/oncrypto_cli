cmake_minimum_required(VERSION 3.16)

# ============================================================
# CRITICAL: Force correct platform detection BEFORE project()
# ============================================================

# If vcpkg triplet indicates Windows, force CMAKE_SYSTEM_NAME
if(DEFINED ENV{VCPKG_TARGET_TRIPLET})
    if($ENV{VCPKG_TARGET_TRIPLET} MATCHES "mingw" OR $ENV{VCPKG_TARGET_TRIPLET} MATCHES "windows" OR $ENV{VCPKG_TARGET_TRIPLET} MATCHES "msvc")
        set(CMAKE_SYSTEM_NAME Windows CACHE STRING "Target system" FORCE)
        message(STATUS "Vcpkg triplet detected: $ENV{VCPKG_TARGET_TRIPLET} -> forcing Windows target")
    endif()
endif()

# If user passed -DCMAKE_SYSTEM_NAME, use it
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "Target platform set to: Windows (from cache/user)")
endif()

# If CMake is invoked with -DCMAKE_TOOLCHAIN_FILE that sets system name,
# it will already be set. This is a fallback.
if(NOT CMAKE_SYSTEM_NAME)
    set(CMAKE_SYSTEM_NAME ${CMAKE_HOST_SYSTEM_NAME})
    message(STATUS "No CMAKE_SYSTEM_NAME set, using host: ${CMAKE_SYSTEM_NAME}")
endif()

project(OnCrypto VERSION 1.6.0 LANGUAGES CXX C)

# ============================================================
# Platform detection variables
# ============================================================
if(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR 
   CMAKE_SYSTEM_NAME STREQUAL "CYGWIN" OR 
   CMAKE_SYSTEM_NAME STREQUAL "MSYS" OR
   CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    set(ONCRYPTO_IS_WINDOWS TRUE)
else()
    set(ONCRYPTO_IS_WINDOWS FALSE)
endif()

# Detect MinGW vs MSVC on Windows
if(ONCRYPTO_IS_WINDOWS)
    if(MINGW)
        set(ONCRYPTO_IS_MINGW TRUE)
        message(STATUS "Windows target: MinGW toolchain")
    elseif(MSVC)
        set(ONCRYPTO_IS_MSVC TRUE)
        message(STATUS "Windows target: MSVC toolchain")
    else()
        message(STATUS "Windows target: unknown toolchain")
    endif()
endif()

# ============================================================
# Options
# ============================================================
option(BUILD_CLI          "Build the oncrypto command line application" ON)
option(BUILD_TESTS        "Build unit tests" ON)
option(BUILD_EXAMPLES     "Build example programs" ON)
option(BUILD_STATIC_SDK   "Build public static SDK library" ON)
option(ENABLE_VISIBILITY  "Hide internal symbols in shared library" OFF)

# ============================================================
# C++ standard
# ============================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# Find OpenSSL - Direct path for MinGW
# ============================================================
if(ONCRYPTO_IS_WINDOWS AND MINGW)
    # Use the vcpkg_installed directory inside the build tree
    set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-mingw-static")
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
    set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_ROOT_DIR}/lib/libcrypto.a")
    set(OPENSSL_SSL_LIBRARY "${OPENSSL_ROOT_DIR}/lib/libssl.a")
    
    # Check if files exist
    if(NOT EXISTS "${OPENSSL_CRYPTO_LIBRARY}")
        message(FATAL_ERROR "OpenSSL crypto library not found at: ${OPENSSL_CRYPTO_LIBRARY}")
    endif()
    if(NOT EXISTS "${OPENSSL_INCLUDE_DIR}/openssl/ssl.h")
        message(FATAL_ERROR "OpenSSL header not found at: ${OPENSSL_INCLUDE_DIR}/openssl/ssl.h")
    endif()
    
    # Create imported targets
    add_library(OpenSSL::Crypto STATIC IMPORTED)
    set_target_properties(OpenSSL::Crypto PROPERTIES
        IMPORTED_LOCATION "${OPENSSL_CRYPTO_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}"
    )
    
    add_library(OpenSSL::SSL STATIC IMPORTED)
    set_target_properties(OpenSSL::SSL PROPERTIES
        IMPORTED_LOCATION "${OPENSSL_SSL_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}"
    )
    
    set(ONCRYPTO_OPENSSL_LIB OpenSSL::Crypto)
    message(STATUS "OpenSSL found: ${OPENSSL_INCLUDE_DIR}, ${OPENSSL_CRYPTO_LIBRARY}")
else()
    # For Linux and MSVC, use find_package
    find_package(OpenSSL 3 REQUIRED COMPONENTS Crypto)
    if(TARGET OpenSSL::Crypto)
        set(ONCRYPTO_OPENSSL_LIB OpenSSL::Crypto)
    elseif(TARGET OpenSSL::CryptoStatic)
        set(ONCRYPTO_OPENSSL_LIB OpenSSL::CryptoStatic)
    else()
        message(FATAL_ERROR "OpenSSL Crypto target not found")
    endif()
endif()
message(STATUS "OnCrypto: using OpenSSL target ${ONCRYPTO_OPENSSL_LIB}")

# ============================================================
# Windows system libraries for MinGW
# ============================================================
if(ONCRYPTO_IS_WINDOWS AND MINGW)
    set(WINDOWS_SYSTEM_LIBS
        ws2_32     # Winsock2 - for OpenSSL sockets
        crypt32    # Crypto API - for OpenSSL certificate store
        gdi32      # GDI - for OpenSSL
        bcrypt     # BCrypt - for OpenSSL 3.x
    )
else()
    set(WINDOWS_SYSTEM_LIBS "")
endif()

# ============================================================
# Source files
# ============================================================
set(CORE_PUBLIC_SOURCES
    core/src/oncrypto.cpp
    core/src/Builder.cpp
    core/src/Advanced.cpp
    core/src/repo/CryptoRepository.cpp
    core/src/c_abi/c_abi.cpp
    core/src/cryption/algo/AES256GCM.cpp
    core/src/cryption/algo/ChaCha20.cpp
    core/src/cryption/algo/XChaCha20.cpp
    core/src/utils/KeyDerivation.cpp
    core/src/utils/FileUtils.cpp
    core/src/format/OnCFormat.cpp
    core/src/backend/EngineBackend.cpp
    core/src/streaming/Streaming.cpp
)

set(ENGINE_SOURCES
    core/src/backend/OpenSSLBackend.cpp
)

# ============================================================
# Object libraries - compile once, reused for shared & static
# ============================================================
add_library(oncrypto_core_objs OBJECT ${CORE_PUBLIC_SOURCES})
add_library(oncrypto_engine_objs OBJECT ${ENGINE_SOURCES})

# Include directories for both
target_include_directories(oncrypto_core_objs
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(oncrypto_engine_objs
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Compile definitions - CRITICAL: Add ONCRYPTO_BUILD_SHARED for proper dllexport
target_compile_definitions(oncrypto_core_objs PRIVATE 
    ONCRYPTO_BUILD_SHARED=1
    ONCRYPTO_BUILD 
    ONCRYPTO_C_ABI_BUILD
)

target_compile_definitions(oncrypto_engine_objs PRIVATE 
    OPENSSL_NO_DLL          # Static OpenSSL
    ONCRYPTO_BUILD_SHARED=1
    ONCRYPTO_BUILD 
    ONCRYPTO_ENGINE_BUILD
)

# Make OpenSSL headers available to the engine object library
target_link_libraries(oncrypto_engine_objs PRIVATE ${ONCRYPTO_OPENSSL_LIB})

# Position independent code - required for shared library
set_target_properties(oncrypto_core_objs PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(oncrypto_engine_objs PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ============================================================
# Compile options
# ============================================================
if(ENABLE_VISIBILITY AND NOT ONCRYPTO_IS_WINDOWS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(VISIBILITY_FLAGS -fvisibility=hidden -fvisibility-inlines-hidden)
    endif()
endif()

set(DEBUG_FLAGS -g -O0 -Wall -Wextra -pedantic)
set(RELEASE_FLAGS -O3 -DNDEBUG)

foreach(TARGET IN ITEMS oncrypto_core_objs oncrypto_engine_objs)
    target_compile_options(${TARGET} PRIVATE
        $<$<CONFIG:Debug>:${DEBUG_FLAGS}>
        $<$<CONFIG:Release>:${RELEASE_FLAGS}>
        $<$<AND:$<BOOL:${ENABLE_VISIBILITY}>,$<NOT:$<BOOL:${ONCRYPTO_IS_WINDOWS}>>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:${VISIBILITY_FLAGS}>
    )
endforeach()

# ============================================================
# Static library "oncrypto_core" (without engine) - internal use
# ============================================================
add_library(oncrypto_core STATIC $<TARGET_OBJECTS:oncrypto_core_objs>)
set_target_properties(oncrypto_core PROPERTIES 
    OUTPUT_NAME oncrypto_core 
    POSITION_INDEPENDENT_CODE ON
)

# ============================================================
# Static library "oncrypto_engine" (engine only) - link OpenSSL
# ============================================================
add_library(oncrypto_engine STATIC $<TARGET_OBJECTS:oncrypto_engine_objs>)
target_link_libraries(oncrypto_engine 
    PUBLIC 
        ${ONCRYPTO_OPENSSL_LIB}
        ${WINDOWS_SYSTEM_LIBS}
)
set_target_properties(oncrypto_engine PROPERTIES 
    OUTPUT_NAME oncrypto_engine 
    POSITION_INDEPENDENT_CODE ON
)

# ============================================================
# Shared library "oncrypto" - combines core + engine
# ============================================================
add_library(oncrypto SHARED
    $<TARGET_OBJECTS:oncrypto_core_objs>
    $<TARGET_OBJECTS:oncrypto_engine_objs>
)

# Link OpenSSL and Windows system libraries
target_link_libraries(oncrypto PRIVATE 
    ${ONCRYPTO_OPENSSL_LIB}
    ${WINDOWS_SYSTEM_LIBS}
)

# Export all symbols for MinGW
if(MINGW)
    target_link_options(oncrypto PRIVATE
        -Wl,--export-all-symbols
        -Wl,--enable-auto-import
    )
endif()

# ============================================================
# CRITICAL: Static runtime for DLL ONLY (MinGW)
# This embeds libgcc and libstdc++ into oncrypto.dll
# ============================================================
if(MINGW)
    target_link_options(oncrypto PRIVATE
        -static-libgcc
        -static-libstdc++
    )
endif()

# Define ONCRYPTO_BUILD_SHARED for the shared library target
target_compile_definitions(oncrypto PRIVATE 
    ONCRYPTO_BUILD_SHARED=1
    ONCRYPTO_BUILD 
    ONCRYPTO_C_ABI_BUILD
)

set_target_properties(oncrypto PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME oncrypto
    CLEAN_DIRECT_OUTPUT 1
)

# For MinGW, remove 'lib' prefix from DLL (makes oncrypto.dll instead of liboncrypto.dll)
if(MINGW OR ONCRYPTO_IS_WINDOWS)
    set_target_properties(oncrypto PROPERTIES PREFIX "")
endif()

# ============================================================
# Static SDK library
# ============================================================
if(BUILD_STATIC_SDK)
    add_library(oncrypto_static STATIC
        $<TARGET_OBJECTS:oncrypto_core_objs>
        $<TARGET_OBJECTS:oncrypto_engine_objs>
    )
    target_link_libraries(oncrypto_static PUBLIC 
        ${ONCRYPTO_OPENSSL_LIB}
        ${WINDOWS_SYSTEM_LIBS}
    )
    target_compile_definitions(oncrypto_static PRIVATE ONCRYPTO_STATIC=1)
    set_target_properties(oncrypto_static PROPERTIES
        OUTPUT_NAME oncrypto
        CLEAN_DIRECT_OUTPUT 1
    )
endif()

# ============================================================
# CLI application - NO static runtime (uses oncrypto.dll's runtime)
# ============================================================
if(BUILD_CLI)
    add_executable(oncrypto_cli cli/src/main.cpp)
    target_include_directories(oncrypto_cli PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(oncrypto_cli PRIVATE oncrypto)
    
    if(ONCRYPTO_IS_WINDOWS)
        set_target_properties(oncrypto_cli PROPERTIES
            SUFFIX ".exe"
        )
    endif()
endif()

# ============================================================
# Tests
# ============================================================
if(BUILD_TESTS)
    add_executable(oncrypto_test tests/test.cpp)
    target_include_directories(oncrypto_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_compile_definitions(oncrypto_test PRIVATE 
        ONCRYPTO_STATIC=1
        OPENSSL_NO_DLL
    )
    target_link_libraries(oncrypto_test PRIVATE 
        oncrypto_core 
        oncrypto_engine
    )

    add_executable(oncrypto_cli_test tests/test_cli.cpp)
    target_include_directories(oncrypto_cli_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(oncrypto_cli_test PRIVATE oncrypto)
endif()

# ============================================================
# Examples
# ============================================================
if(BUILD_EXAMPLES)
    add_executable(example_simple examples/example.cpp)
    target_include_directories(example_simple PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core/include
    )
    target_link_libraries(example_simple PRIVATE oncrypto)
endif()

# ============================================================
# Installation
# ============================================================
include(GNUInstallDirs)

install(TARGETS oncrypto
    EXPORT OnCryptoTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(BUILD_STATIC_SDK)
    install(TARGETS oncrypto_static
        EXPORT OnCryptoTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()

if(BUILD_CLI)
    install(TARGETS oncrypto_cli
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

install(DIRECTORY core/include/oncrypto
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
    PATTERN "backend" EXCLUDE
)

install(EXPORT OnCryptoTargets
    FILE OnCryptoConfig.cmake
    NAMESPACE OnCrypto::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OnCrypto
)

# ============================================================
# RPATH handling
# ============================================================
if(ONCRYPTO_IS_WINDOWS)
    set(CMAKE_SKIP_RPATH TRUE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "")
    message(STATUS "RPATH disabled for Windows target")
else()
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    message(STATUS "RPATH enabled for ELF target")
endif()

# ============================================================
# Final status messages
# ============================================================
message(STATUS "")
message(STATUS "OnCrypto ${PROJECT_VERSION} Configuration:")
message(STATUS "  Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  Target platform:   ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Is Windows:        ${ONCRYPTO_IS_WINDOWS}")
message(STATUS "  Compiler:          ${CMAKE_CXX_COMPILER}")
message(STATUS "  Compiler ID:       ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Build directory:   ${CMAKE_BINARY_DIR}")
if(BUILD_STATIC_SDK)
    message(STATUS "  Static SDK:        ON (${CMAKE_STATIC_LIBRARY_PREFIX}oncrypto${CMAKE_STATIC_LIBRARY_SUFFIX})")
endif()
if(BUILD_CLI)
    message(STATUS "  CLI:               ON (${CMAKE_EXECUTABLE_PREFIX}oncrypto_cli${CMAKE_EXECUTABLE_SUFFIX})")
endif()
message(STATUS "  Tests:             ${BUILD_TESTS}")
message(STATUS "  Examples:          ${BUILD_EXAMPLES}")
message(STATUS "  Shared library:    ${CMAKE_SHARED_LIBRARY_PREFIX}oncrypto${CMAKE_SHARED_LIBRARY_SUFFIX}")
message(STATUS "")