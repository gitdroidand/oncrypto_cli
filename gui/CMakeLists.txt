cmake_minimum_required(VERSION 3.21)

project(
    OnCryptoDesktop
    VERSION 1.0.0
    LANGUAGES CXX
)

# ============================================================
# Global configuration
# ============================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ============================================================
# Qt
# ============================================================

find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    Svg
)

# ============================================================
# Paths
# ============================================================

set(ONCRYPTO_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set(ONCRYPTO_INCLUDE_DIR
    "${ONCRYPTO_ROOT}/include"
)

set(ONCRYPTO_LIB_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)

# ============================================================
# OnCrypto Core
# ============================================================

add_library(OnCrypto::Core SHARED IMPORTED GLOBAL)

set_target_properties(OnCrypto::Core PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES
        "${ONCRYPTO_INCLUDE_DIR}"
)

# ============================================================
# Platform / ABI selection
# ============================================================

if(WIN32)

    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR
            "OnCryptoDesktop supports only 64-bit Windows."
        )
    endif()

    set(ONCRYPTO_PLATFORM_DIR
        "${ONCRYPTO_LIB_DIR}/windows_x64"
    )

    set(ONCRYPTO_IMPLIB
        "${ONCRYPTO_PLATFORM_DIR}/liboncrypto.dll.a"
    )

    set(ONCRYPTO_DLL
        "${ONCRYPTO_PLATFORM_DIR}/oncrypto.dll"
    )

    if(NOT EXISTS "${ONCRYPTO_IMPLIB}")
        message(FATAL_ERROR
            "OnCrypto import library not found:\n"
            "  ${ONCRYPTO_IMPLIB}"
        )
    endif()

    if(NOT EXISTS "${ONCRYPTO_DLL}")
        message(FATAL_ERROR
            "OnCrypto DLL not found:\n"
            "  ${ONCRYPTO_DLL}"
        )
    endif()

    set_target_properties(OnCrypto::Core PROPERTIES
        IMPORTED_IMPLIB
            "${ONCRYPTO_IMPLIB}"

        IMPORTED_LOCATION
            "${ONCRYPTO_DLL}"
    )

    message(STATUS
        "OnCrypto platform: Windows x64"
    )

    message(STATUS
        "OnCrypto DLL: ${ONCRYPTO_DLL}"
    )

    message(STATUS
        "OnCrypto import library: ${ONCRYPTO_IMPLIB}"
    )

elseif(UNIX AND NOT APPLE AND NOT ANDROID)

    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR
            "OnCryptoDesktop supports only 64-bit Linux."
        )
    endif()

    set(ONCRYPTO_PLATFORM_DIR
        "${ONCRYPTO_LIB_DIR}/linux_x64"
    )

    set(ONCRYPTO_SHARED
        "${ONCRYPTO_PLATFORM_DIR}/liboncrypto.so"
    )

    if(NOT EXISTS "${ONCRYPTO_SHARED}")
        message(FATAL_ERROR
            "OnCrypto shared library not found:\n"
            "  ${ONCRYPTO_SHARED}"
        )
    endif()

    set_target_properties(OnCrypto::Core PROPERTIES
        IMPORTED_LOCATION
            "${ONCRYPTO_SHARED}"
    )

    message(STATUS
        "OnCrypto platform: Linux x64"
    )

elseif(ANDROID)

    if(ANDROID_ABI STREQUAL "arm64-v8a")

        set(ONCRYPTO_PLATFORM_DIR
            "${ONCRYPTO_LIB_DIR}/arm64-v8a"
        )

    elseif(ANDROID_ABI STREQUAL "armeabi-v7a")

        set(ONCRYPTO_PLATFORM_DIR
            "${ONCRYPTO_LIB_DIR}/armeabi-v7a"
        )

    else()

        message(FATAL_ERROR
            "Unsupported Android ABI: ${ANDROID_ABI}"
        )

    endif()

    set(ONCRYPTO_SHARED
        "${ONCRYPTO_PLATFORM_DIR}/liboncrypto.so"
    )

    if(NOT EXISTS "${ONCRYPTO_SHARED}")
        message(FATAL_ERROR
            "OnCrypto shared library not found:\n"
            "  ${ONCRYPTO_SHARED}"
        )
    endif()

    set_target_properties(OnCrypto::Core PROPERTIES
        IMPORTED_LOCATION
            "${ONCRYPTO_SHARED}"
    )

    message(STATUS
        "OnCrypto platform: Android ${ANDROID_ABI}"
    )

else()

    message(FATAL_ERROR
        "Unsupported platform."
    )

endif()

# ============================================================
# Application sources
# ============================================================

set(ONCRYPTO_DESKTOP_SOURCES

    src/main.cpp
    src/gui.cpp

    src/ui/core/onc_service.cpp
    src/ui/core/session_stats.cpp
    src/ui/core/file_utils.cpp
    src/ui/core/recent_files.cpp
    src/ui/core/password_validator.cpp

    src/ui/screen/layout_utils.cpp
    src/ui/screen/dashboard.cpp
    src/ui/screen/encrypt.cpp
    src/ui/screen/decrypt.cpp

    include/gui/ui/screen/layout_utils.hpp

    src/ui/navhost.cpp
    src/ui/theme.cpp
    src/ui/toolbar.cpp
    src/ui/menu.cpp
    src/ui/content.cpp
    src/ui/drawer.cpp

    include/gui/gui.hpp

    resources/resources.qrc
)

# ============================================================
# Application
# ============================================================

add_executable(
    OnCryptoDesktop
    ${ONCRYPTO_DESKTOP_SOURCES}
)

# ============================================================
# Include directories
# ============================================================

target_include_directories(
    OnCryptoDesktop
    PRIVATE

        "${CMAKE_CURRENT_SOURCE_DIR}/include"

        "${CMAKE_CURRENT_SOURCE_DIR}/../core/include"

        "${ONCRYPTO_INCLUDE_DIR}"
)


# ============================================================
# Qt + OnCrypto
# ============================================================

target_link_libraries(
    OnCryptoDesktop
    PRIVATE

        Qt6::Widgets
        Qt6::Svg

        OnCrypto::Core
)

# ============================================================
# Compile options
# ============================================================

if(MINGW)

    target_compile_options(
        OnCryptoDesktop
        PRIVATE

            -ffunction-sections
            -fdata-sections
    )

    target_link_options(
        OnCryptoDesktop
        PRIVATE

            -static-libgcc
            -static-libstdc++
    )

endif()

# ============================================================
# Windows application
# ============================================================

if(WIN32)

    set_target_properties(
        OnCryptoDesktop
        PROPERTIES

            WIN32_EXECUTABLE TRUE
    )

    # Copy OnCrypto DLL beside the executable.
    add_custom_command(
        TARGET OnCryptoDesktop
        POST_BUILD

        COMMAND
            ${CMAKE_COMMAND}
            -E copy_if_different

            "${ONCRYPTO_DLL}"

            "$<TARGET_FILE_DIR:OnCryptoDesktop>"
    )

endif()

# ============================================================
# Diagnostics
# ============================================================

message(STATUS "")
message(STATUS "========== OnCryptoDesktop ==========")
message(STATUS "C++ standard: C++20")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Architecture: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Pointer size: ${CMAKE_SIZEOF_VOID_P}")
message(STATUS "OnCrypto include: ${ONCRYPTO_INCLUDE_DIR}")

if(WIN32)
    message(STATUS "OnCrypto DLL: ${ONCRYPTO_DLL}")
    message(STATUS "OnCrypto import lib: ${ONCRYPTO_IMPLIB}")
endif()

message(STATUS "=====================================")
message(STATUS "")