set(MSYS2_ROOT "C:/msys64")
# Only apply libc++ to C++ sources to avoid unused-argument warnings on C objects
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
add_link_options(-stdlib=libc++)
# Set the compilers using their full, unambiguous paths.
set(CMAKE_C_COMPILER "${MSYS2_ROOT}/clang64/bin/clang.exe")
set(CMAKE_CXX_COMPILER "${MSYS2_ROOT}/clang64/bin/clang++.exe")
set(CMAKE_RC_COMPILER "${MSYS2_ROOT}/clang64/bin/windres.exe")
set(CMAKE_AR "${MSYS2_ROOT}/clang64/bin/llvm-ar.exe")
set(CMAKE_RANLIB "${MSYS2_ROOT}/clang64/bin/llvm-ranlib.exe")
set(CMAKE_NM "${MSYS2_ROOT}/clang64/bin/llvm-nm.exe")
set(CMAKE_LINKER_TYPE LLD)
# Set the build system using its full path.
set(CMAKE_MAKE_PROGRAM "${MSYS2_ROOT}/clang64/bin/ninja.exe")
set(CMAKE_C_FLAGS "-target x86_64-w64-windows-gnu" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-target x86_64-w64-windows-gnu" CACHE STRING "" FORCE)
