cmake_minimum_required(VERSION 3.16)
project(OnCrypto VERSION 1.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_EXAMPLES "Build example programs" ON)

find_package(OpenSSL 3 REQUIRED COMPONENTS Crypto)

# ============================================================
# Core as OBJECT library (برای لینک استاتیک بعدی)
# ============================================================

add_library(oncrypto_core OBJECT
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
    core/src/backend/OpenSSLBackend.cpp
    core/src/streaming/Streaming.cpp
)

target_include_directories(oncrypto_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/include)
target_link_libraries(oncrypto_core PUBLIC OpenSSL::Crypto)

# ============================================================
# CLI Application
# ============================================================

add_executable(oncrypto_cli cli/src/main.cpp)
target_include_directories(oncrypto_cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/core/include)
target_link_libraries(oncrypto_cli PRIVATE oncrypto_core)

# ============================================================
# Tests
# ============================================================

if(BUILD_TESTS)
    add_executable(oncrypto_test tests/test.cpp)
    target_include_directories(oncrypto_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/include
    )
    target_link_libraries(oncrypto_test PRIVATE oncrypto_core)
    
    add_executable(oncrypto_cli_test tests/test_cli.cpp)
    target_include_directories(oncrypto_cli_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/include)
endif()

# ============================================================
# Examples
# ============================================================

if(BUILD_EXAMPLES)
    add_executable(example_simple examples/example.cpp)
    target_include_directories(example_simple PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/core/include)
    target_link_libraries(example_simple PRIVATE oncrypto_core)
endif()

# ============================================================
# Compiler flags
# ============================================================

target_compile_options(oncrypto_core PRIVATE 
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra -pedantic>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

target_compile_options(oncrypto_cli PRIVATE 
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra -pedantic>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

if(BUILD_TESTS)
    target_compile_options(oncrypto_test PRIVATE 
        $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra -pedantic>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
    target_compile_options(oncrypto_cli_test PRIVATE 
        $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra -pedantic>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
endif()

if(BUILD_EXAMPLES)
    target_compile_options(example_simple PRIVATE 
        $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra -pedantic>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
endif()

message(STATUS "")
message(STATUS "OnCrypto ${PROJECT_VERSION} Configuration:")
message(STATUS "  Build type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  Tests:          ${BUILD_TESTS}")
message(STATUS "  Examples:       ${BUILD_EXAMPLES}")
message(STATUS "")
