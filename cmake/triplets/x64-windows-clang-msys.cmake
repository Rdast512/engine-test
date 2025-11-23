# This file describes the MSYS2 CLANG64 environment to vcpkg.

set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# Tell vcpkg to load our custom toolchain file.
# VCPKG_CMAKE_SYSTEM_NAME is set to empty to prevent vcpkg from thinking
# it's a cross-compilation scenario, which simplifies things.
set(VCPKG_CMAKE_SYSTEM_NAME "MinGW")
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchains/clang-windows.cmake")