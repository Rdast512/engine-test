set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_AR llvm-ar)
set(CMAKE_RANLIB llvm-ranlib)
set(CMAKE_NM llvm-nm)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
add_link_options(-stdlib=libc++)
# # --- COMMON FLAGS for all build types ---
# # -Wall -Wextra -Wpedantic: Excellent, strict warnings.
# # -Wshadow: Warns about variable shadowing.
# # -m64: Explicitly target 64-bit.
# set(COMMON_FLAGS "-Wall -Wextra -Wpedantic -Wshadow -m64")
# # -pthread is essential for std::thread and other threading primitives on Linux.
# # -stdlib=libstdc++ is the key flag to force the GNU standard library.
# set(COMMON_CXX_FLAGS "-pthread -stdlib=libstdc++")

# # --- RELEASE-only flags for maximum performance ---
# # -O3: The highest level of optimization.
# # -flto=thin: ThinLTO offers most of the performance of full LTO with much faster link times.
# # -march=native: Optimizes for the host CPU (for distribution, consider a baseline like x86-64-v3).
# # -DNDEBUG: Disables asserts and other debug code.
# set(RELEASE_FLAGS "-O3 -flto=thin -march=native -DNDEBUG")

# # --- DEBUG-only flags for finding bugs ---
# # -g: Generate full debug information for GDB/LLDB.
# # -Og: A better debug optimization level than -O0. It improves runtime speed without hurting debugging.
# # -fsanitize=address,undefined: The Address and UndefinedBehavior Sanitizers. Even more powerful on Linux.
# set(DEBUG_FLAGS "-g -Og -fsanitize=address,undefined")

# # --- Combine and set the final flags using CMake Generator Expressions ---
# set(CMAKE_C_FLAGS_INIT "${COMMON_FLAGS}")
# set(CMAKE_CXX_FLAGS_INIT "${COMMON_FLAGS} ${COMMON_CXX_FLAGS}")

# set(CMAKE_C_FLAGS_DEBUG_INIT "${DEBUG_FLAGS}")
# set(CMAKE_CXX_FLAGS_DEBUG_INIT "${DEBUG_FLAGS}")

# set(CMAKE_C_FLAGS_RELEASE_INIT "${RELEASE_FLAGS}")
# set(CMAKE_CXX_FLAGS_RELEASE_INIT "${RELEASE_FLAGS}")```
