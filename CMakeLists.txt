cmake_minimum_required(VERSION 3.16)
project(OnCrypto VERSION 1.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# فلگ‌های ساخت
# ============================================================
option(BUILD_SHARED_LIB "Build shared library (liboncrypto.so)" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_TESTS "Build unit tests" ON)

find_package(OpenSSL 3 REQUIRED COMPONENTS Crypto)

# ============================================================
# کتابخانه مشترک (اختیاری)
# ============================================================
if(BUILD_SHARED_LIB)
    add_library(oncrypto SHARED
        src/oncrypto.cpp
        src/repo/CryptoRepository.cpp
        src/cryption/algo/AES256GCM.cpp
        src/cryption/algo/ChaCha20.cpp
        src/cryption/algo/XChaCha20.cpp
        src/utils/KeyDerivation.cpp
        src/utils/FileUtils.cpp
    )

    target_include_directories(oncrypto 
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(oncrypto 
        PUBLIC 
            OpenSSL::Crypto
    )

    set_target_properties(oncrypto PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
    )
endif()

# ============================================================
# برنامه CLI (همیشه ساخته میشه)
# ============================================================
add_executable(oncrypto_cli main.cpp)
target_include_directories(oncrypto_cli PRIVATE include)

if(BUILD_SHARED_LIB)
    target_link_libraries(oncrypto_cli PRIVATE oncrypto)
else()
    # اگر کتابخانه غیرفعال باشه، از فایل‌های مستقیم استفاده کن
    target_sources(oncrypto_cli PRIVATE
        src/oncrypto.cpp
        src/repo/CryptoRepository.cpp
        src/cryption/algo/AES256GCM.cpp
        src/cryption/algo/ChaCha20.cpp
        src/cryption/algo/XChaCha20.cpp
        src/utils/KeyDerivation.cpp
        src/utils/FileUtils.cpp
    )
    target_link_libraries(oncrypto_cli PRIVATE OpenSSL::Crypto)
endif()

# ============================================================
# تست‌ها (اختیاری)
# ============================================================
if(BUILD_TESTS)
    add_executable(oncrypto_test tests/test.cpp)
    target_include_directories(oncrypto_test PRIVATE 
        include
        tests/include
    )
    if(BUILD_SHARED_LIB)
        target_link_libraries(oncrypto_test PRIVATE oncrypto)
    else()
        target_sources(oncrypto_test PRIVATE
            src/oncrypto.cpp
            src/repo/CryptoRepository.cpp
            src/cryption/algo/AES256GCM.cpp
            src/cryption/algo/ChaCha20.cpp
            src/cryption/algo/XChaCha20.cpp
            src/utils/KeyDerivation.cpp
            src/utils/FileUtils.cpp
        )
        target_link_libraries(oncrypto_test PRIVATE OpenSSL::Crypto)
    endif()
endif()

# ============================================================
# مثال‌ها (اختیاری)
# ============================================================
if(BUILD_EXAMPLES AND BUILD_SHARED_LIB)
    add_executable(example_simple examples/example.cpp)
    target_include_directories(example_simple PRIVATE include)
    target_link_libraries(example_simple PRIVATE oncrypto)
endif()

# ============================================================
# نصب (فقط وقتی کتابخانه ساخته بشه)
# ============================================================
if(BUILD_SHARED_LIB)
    include(GNUInstallDirs)
    
    install(TARGETS oncrypto
        EXPORT oncryptoTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    
    install(DIRECTORY include/oncrypto
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp"
    )
    
    install(EXPORT oncryptoTargets
        FILE oncryptoConfig.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/oncrypto
    )
endif()

# ============================================================
# کامپایلر flags
# ============================================================
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(oncrypto_cli PRIVATE -g -O0 -Wall -Wextra -pedantic)
    if(BUILD_SHARED_LIB)
        target_compile_options(oncrypto PRIVATE -g -O0 -Wall -Wextra -pedantic)
    endif()
else()
    target_compile_options(oncrypto_cli PRIVATE -O3 -DNDEBUG)
    if(BUILD_SHARED_LIB)
        target_compile_options(oncrypto PRIVATE -O3 -DNDEBUG)
    endif()
endif()

if(BUILD_TESTS)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(oncrypto_test PRIVATE -g -O0 -Wall -Wextra -pedantic)
    else()
        target_compile_options(oncrypto_test PRIVATE -O3 -DNDEBUG)
    endif()
endif()

if(BUILD_EXAMPLES AND BUILD_SHARED_LIB)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(example_simple PRIVATE -g -O0 -Wall -Wextra -pedantic)
    else()
        target_compile_options(example_simple PRIVATE -O3 -DNDEBUG)
    endif()
endif()

# ============================================================
# چاپ خلاصه
# ============================================================
message(STATUS "")
message(STATUS "OnCrypto ${PROJECT_VERSION} Configuration:")
message(STATUS "  Build type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build shared lib: ${BUILD_SHARED_LIB}")
message(STATUS "  Build tests:      ${BUILD_TESTS}")
message(STATUS "  Build examples:   ${BUILD_EXAMPLES}")
message(STATUS "")
