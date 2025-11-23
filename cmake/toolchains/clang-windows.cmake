set(MSYS2_ROOT "C:/msys64")

# Set the compilers using their full, unambiguous paths.
set(CMAKE_C_COMPILER   "${MSYS2_ROOT}/clang64/bin/clang.exe")
set(CMAKE_CXX_COMPILER "${MSYS2_ROOT}/clang64/bin/clang++.exe")
set(CMAKE_RC_COMPILER  "${MSYS2_ROOT}/clang64/bin/windres.exe")
set(CMAKE_AR           "${MSYS2_ROOT}/clang64/bin/llvm-ar.exe")
set(CMAKE_RANLIB       "${MSYS2_ROOT}/clang64/bin/llvm-ranlib.exe")
set(CMAKE_NM           "${MSYS2_ROOT}/clang64/bin/llvm-nm.exe")

# Set the build system using its full path.
set(CMAKE_MAKE_PROGRAM "${MSYS2_ROOT}/clang64/bin/ninja.exe")
set(CMAKE_C_FLAGS   "-target x86_64-w64-windows-gnu" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-target x86_64-w64-windows-gnu" CACHE STRING "" FORCE)


# # Debug Flags
# set(GLOBAL_DEBUG_FLAGS "-g -O0 -D_DEBUG")

# # Release Flags (LTO enabled for everything!)
# set(GLOBAL_REL_FLAGS   "-O3 -DNDEBUG -flto=thin -march=native")

# # Apply flags based on build type
# set(CMAKE_C_FLAGS_INIT   "${GLOBAL_C_FLAGS}")
# set(CMAKE_CXX_FLAGS_INIT "${GLOBAL_CXX_FLAGS}")

# set(CMAKE_C_FLAGS_DEBUG_INIT   "${GLOBAL_DEBUG_FLAGS}")
# set(CMAKE_CXX_FLAGS_DEBUG_INIT "${GLOBAL_DEBUG_FLAGS}")

# set(CMAKE_C_FLAGS_RELEASE_INIT   "${GLOBAL_REL_FLAGS}")
# set(CMAKE_CXX_FLAGS_RELEASE_INIT "${GLOBAL_REL_FLAGS}")