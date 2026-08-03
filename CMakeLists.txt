cmake_minimum_required(VERSION 3.16)
project(OnCrypto VERSION 1.6.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

option(BUILD_CLI "Build the oncrypto command line application" ON)
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_STATIC_SDK "Build public static SDK library (liboncrypto.a)" ON)
option(ENABLE_VISIBILITY "Hide internal symbols in shared library" ON)

find_package(OpenSSL 3 REQUIRED COMPONENTS Crypto)

if(TARGET OpenSSL::Crypto)
    set(ONCRYPTO_OPENSSL_LIB OpenSSL::Crypto)
elseif(TARGET OpenSSL::CryptoStatic)
    set(ONCRYPTO_OPENSSL_LIB OpenSSL::CryptoStatic)
else()
    message(FATAL_ERROR "OpenSSL Crypto target not found")
endif()

message(STATUS "OnCrypto: using OpenSSL target ${ONCRYPTO_OPENSSL_LIB}")

set(CORE_PUBLIC_SOURCES
    core/src/oncrypto.cpp
    core/src/Builder.cpp
    core/src/Advanced.cpp
    core/src/repo/CryptoRepository.cpp
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

add_library(oncrypto_core_objs OBJECT ${CORE_PUBLIC_SOURCES})
add_library(oncrypto_engine_objs OBJECT ${ENGINE_SOURCES})

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

target_compile_definitions(oncrypto_core_objs PRIVATE ONCRYPTO_BUILD)

target_compile_definitions(oncrypto_engine_objs PRIVATE ONCRYPTO_BUILD ONCRYPTO_ENGINE_BUILD)

target_compile_options(oncrypto_core_objs PRIVATE
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra -pedantic>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
    $<$<AND:$<BOOL:${ENABLE_VISIBILITY}>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-fvisibility=hidden -fvisibility-inlines-hidden>
)

target_compile_options(oncrypto_engine_objs PRIVATE
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra -pedantic>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
    $<$<AND:$<BOOL:${ENABLE_VISIBILITY}>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-fvisibility=hidden -fvisibility-inlines-hidden>
)

set_target_properties(oncrypto_core_objs PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
set_target_properties(oncrypto_engine_objs PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

add_library(oncrypto_core STATIC $<TARGET_OBJECTS:oncrypto_core_objs>)
set_target_properties(oncrypto_core PROPERTIES
    OUTPUT_NAME oncrypto_core
    POSITION_INDEPENDENT_CODE ON
)

add_library(oncrypto_engine STATIC $<TARGET_OBJECTS:oncrypto_engine_objs>)
set_target_properties(oncrypto_engine PROPERTIES
    OUTPUT_NAME oncrypto_engine
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(oncrypto_engine
    PUBLIC
        ${ONCRYPTO_OPENSSL_LIB}
)

add_library(oncrypto SHARED
    $<TARGET_OBJECTS:oncrypto_core_objs>
    $<TARGET_OBJECTS:oncrypto_engine_objs>
)

target_link_libraries(oncrypto
    PRIVATE
        ${ONCRYPTO_OPENSSL_LIB}
)

target_compile_definitions(oncrypto PRIVATE ONCRYPTO_BUILD)

set_target_properties(oncrypto PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME oncrypto
    CLEAN_DIRECT_OUTPUT 1
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

if(BUILD_STATIC_SDK)
    add_library(oncrypto_static STATIC
        $<TARGET_OBJECTS:oncrypto_core_objs>
        $<TARGET_OBJECTS:oncrypto_engine_objs>
    )
    target_link_libraries(oncrypto_static
        PUBLIC
            ${ONCRYPTO_OPENSSL_LIB}
    )
    set_target_properties(oncrypto_static PROPERTIES
        OUTPUT_NAME oncrypto
        CLEAN_DIRECT_OUTPUT 1
    )
endif()

if(BUILD_CLI)
    add_executable(oncrypto_cli cli/src/main.cpp)

    target_include_directories(oncrypto_cli
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/core/include
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(oncrypto_cli
        PRIVATE
            oncrypto
    )
endif()

if(BUILD_TESTS)
    add_executable(oncrypto_test tests/test.cpp)

    target_include_directories(oncrypto_test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/core/include
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/include
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(oncrypto_test
        PRIVATE
            oncrypto_core
            oncrypto_engine
    )

    add_executable(oncrypto_cli_test tests/test_cli.cpp)

    target_include_directories(oncrypto_cli_test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/include
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(oncrypto_cli_test
        PRIVATE
            oncrypto
    )
endif()

if(BUILD_EXAMPLES)
    add_executable(example_simple examples/example.cpp)

    target_include_directories(example_simple
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/core/include
    )

    target_link_libraries(example_simple
        PRIVATE
            oncrypto
    )
endif()

install(TARGETS oncrypto
    EXPORT OnCryptoTargets
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
    FILES_MATCHING PATTERN "*.hpp"
    PATTERN "backend" EXCLUDE
)

install(EXPORT OnCryptoTargets
    FILE OnCryptoConfig.cmake
    NAMESPACE OnCrypto::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OnCrypto
)

message(STATUS "")
message(STATUS "OnCrypto ${PROJECT_VERSION} Configuration:")
message(STATUS "  Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared library:    ${CMAKE_BINARY_DIR}/liboncrypto.so")
if(BUILD_STATIC_SDK)
    message(STATUS "  Static library:    ${CMAKE_BINARY_DIR}/liboncrypto.a")
endif()
if(BUILD_CLI)
    message(STATUS "  CLI executable:    ${CMAKE_BINARY_DIR}/oncrypto_cli")
endif()
message(STATUS "  Tests:             ${BUILD_TESTS}")
message(STATUS "  Examples:          ${BUILD_EXAMPLES}")
message(STATUS "")
