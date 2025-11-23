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
set(COMMON_FLAGS "-Wall -Wextra -Wpedantic -Wshadow -m64 -DBOOST_THREAD_USE_WIN32")
set(COMMON_CXX_FLAGS "-pthread") # Needed for std::thread on MinGW


# set(COMMON_FLAGS "-Wall -Wextra -Wshadow -m64")
# set(COMMON_CXX_FLAGS "-pthread") # Needed for std::thread on MinGW

# # --- RELEASE-only flags for maximum performance ---
# set(RELEASE_FLAGS "-O3 -flto -march=native -DNDEBUG")

# # --- DEBUG-only flags for finding bugs ---
# # Note: ASan and UBSan cannot always be used together. ASan is usually more critical.
# # Choose one or create separate presets if you need both.
# set(DEBUG_FLAGS "-g -fsanitize=address,undefined")


# set(CMAKE_C_FLAGS "${COMMON_FLAGS} $<$<CONFIG:Debug>:${DEBUG_FLAGS}> $<$<CONFIG:Release>:${RELEASE_FLAGS}>" CACHE STRING "C compiler flags")
# set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} ${COMMON_CXX_FLAGS} $<$<CONFIG:Debug>:${DEBUG_FLAGS}> $<$<CONFIG:Release>:${RELEASE_FLAGS}>" CACHE STRING "C++ compiler flags")

# CRITICAL: Force Clang to target the MinGW environment. This prevents it
# from trying to find MSVC headers and libraries.
set(CMAKE_C_FLAGS   "-target x86_64-w64-windows-gnu" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-target x86_64-w64-windows-gnu" CACHE STRING "" FORCE)