set(MSYS2_ROOT "C:/msys64")

# Set the compilers using their full, unambiguous paths.
set(CMAKE_C_COMPILER   "${MSYS2_ROOT}/clang64/bin/clang.exe")
set(CMAKE_CXX_COMPILER "${MSYS2_ROOT}/clang64/bin/clang++.exe")
set(CMAKE_RC_COMPILER  "${MSYS2_ROOT}/clang64/bin/windres.exe")

# Set the build system using its full path.
set(CMAKE_MAKE_PROGRAM "${MSYS2_ROOT}/clang64/bin/ninja.exe")

# CRITICAL: Force Clang to target the MinGW environment. This prevents it
# from trying to find MSVC headers and libraries.
set(CMAKE_C_FLAGS   "-target x86_64-w64-windows-gnu" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-target x86_64-w64-windows-gnu" CACHE STRING "" FORCE)