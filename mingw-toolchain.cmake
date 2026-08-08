# mingw-toolchain.cmake
# First load vcpkg
include("/home/mehdi_mehdark/vcpkg/scripts/buildsystems/vcpkg.cmake")

# Force Windows target
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Set compilers with FORCE
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc CACHE STRING "C compiler" FORCE)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++ CACHE STRING "C++ compiler" FORCE)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres CACHE STRING "RC compiler" FORCE)

# Disable find_root_path
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)

# Disable RPATH
set(CMAKE_SKIP_RPATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_CROSSCOMPILING TRUE)

message(STATUS "MinGW toolchain loaded with triplet: ${VCPKG_TARGET_TRIPLET}")