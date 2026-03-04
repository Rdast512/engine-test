set(MSYS2_ROOT "C:/msys64")

# --- Options ---
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
add_link_options(-stdlib=libc++)

# --- Compilers ---
set(CMAKE_C_COMPILER "${MSYS2_ROOT}/clang64/bin/clang.exe")
set(CMAKE_CXX_COMPILER "${MSYS2_ROOT}/clang64/bin/clang++.exe")
# SWITCHED to llvm-rc
set(CMAKE_RC_COMPILER "${MSYS2_ROOT}/clang64/bin/llvm-rc.exe")

# --- Binary Utilities ---
set(CMAKE_AR "${MSYS2_ROOT}/clang64/bin/llvm-ar.exe")
set(CMAKE_RANLIB "${MSYS2_ROOT}/clang64/bin/llvm-ranlib.exe")
set(CMAKE_NM "${MSYS2_ROOT}/clang64/bin/llvm-nm.exe")
# ADDED these for completeness
set(CMAKE_STRIP "${MSYS2_ROOT}/clang64/bin/llvm-strip.exe")
set(CMAKE_OBJCOPY "${MSYS2_ROOT}/clang64/bin/llvm-objcopy.exe")
set(CMAKE_OBJDUMP "${MSYS2_ROOT}/clang64/bin/llvm-objdump.exe")

# --- Linker & Make ---
set(CMAKE_LINKER_TYPE LLD)
set(CMAKE_MAKE_PROGRAM "${MSYS2_ROOT}/clang64/bin/ninja.exe")

# --- Flags ---
set(CMAKE_C_FLAGS "-target x86_64-w64-windows-gnu" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-target x86_64-w64-windows-gnu" CACHE STRING "" FORCE)