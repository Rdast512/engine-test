cmake_minimum_required(VERSION 3.29)

project(engine)
if (POLICY CMP0167)
  cmake_policy(SET CMP0167 OLD)
endif ()


# --- Standard Configuration ---
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# if(UNIX AND NOT APPLE)
#     add_compile_options(-stdlib=libc++)
#     add_link_options(-stdlib=libc++)
# endif()
message(STATUS "CMAKE_Version: ${CMAKE_VERSION}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "=== TOOLCHAIN VERIFICATION ===")
message(STATUS "C   compiler:   ${CMAKE_C_COMPILER}")
message(STATUS "CXX compiler:   ${CMAKE_CXX_COMPILER}")
message(STATUS "RC  compiler:   ${CMAKE_RC_COMPILER}")
message(STATUS "Linker:         ${CMAKE_LINKER}")
message(STATUS "Linker type:    ${CMAKE_LINKER_TYPE}")
message(STATUS "AR:             ${CMAKE_AR}")
message(STATUS "RANLIB:         ${CMAKE_RANLIB}")
message(STATUS "NM:             ${CMAKE_NM}")
message(STATUS "STRIP:          ${CMAKE_STRIP}")
message(STATUS "OBJCOPY:        ${CMAKE_OBJCOPY}")
message(STATUS "OBJDUMP:        ${CMAKE_OBJDUMP}")
message(STATUS "Make program:   ${CMAKE_MAKE_PROGRAM}")
message(STATUS "CXX flags:      ${CMAKE_CXX_FLAGS}")
message(STATUS "C flags:        ${CMAKE_C_FLAGS}")
message(STATUS "==============================")
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  message(STATUS "Detected Clang/GCC compiler: ${CMAKE_CXX_COMPILER_ID}")

  # Fast sanitizer-only configure:
  # cmake -S . -B build-sanitizers -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
  # Common sanitizer options (The address, memory, and thread sanitizers are mutually exclusive. You can only use one at a time in a single build):
  # - address: heap/stack OOB, use-after-free, double free
  # - undefined: UB like bad casts, invalid shifts, signed overflow
  # - leak: memory leaks
  # - thread: data races
  # - memory: uninitialized reads (slower, often incompatible with others)
  option(ENABLE_SANITIZERS "Enable AddressSanitizer + UndefinedBehaviorSanitizer in Debug and Release builds" ON)
  if (ENABLE_SANITIZERS)
    add_compile_options("$<$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>:-fsanitize=address,undefined>"
                            "$<$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>:-fno-omit-frame-pointer>"
                            "$<$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>:-fno-optimize-sibling-calls>")
    add_link_options("$<$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>:-fsanitize=address,undefined>")
  endif ()

  if (WIN32)
    message(STATUS "Configuring compiler flags for Clang on Windows")

    # [TRACY-FIX] Generate PDB file for the linker
    # [TRACY-FIX] Also generate PDB for RelWithDebInfo if you use it
    add_link_options("$<$<CONFIG:Debug,RelWithDebInfo>:-Wl,--pdb=>")

    # [COVERAGE] NOTE: -fprofile-instr-generate is intentionally NOT set here as a
    # global link option. This project uses shared libraries (engine_core, engine_runtime…).
    # If every DLL links its own libclang_rt.profile, each gets independent counter arrays
    # and they conflict → only destructor hints appear.
    # The runtime is linked ONLY into the engine executable (see src/CMakeLists.txt) so
    # all DLLs register their counters with one runtime → single clean .profraw at exit.

    # [TRACY-FIX] Added -gcodeview. This converts DWARF to Microsoft CodeView format
    # so dbghelp.dll (used by Tracy) can read it.
    set(COMMON_DEBUG_FLAGS "-O0 -g -gdwarf -gcodeview -m64 -march=native -mtune=native -pthread")
    # [TRACY-FIX] amAdded -g to Release. This allows Tracy to see function names
    # in optimized builds without slowing down the code.
    set(COMMON_RELEASE_FLAGS "-O3 -g -gdwarf -gcodeview -Wall -Wextra -Wpedantic -DNDEBUG -march=native -mtune=native -flto -funroll-loops -finline-functions -m64 -pthread")

    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${COMMON_DEBUG_FLAGS}")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${COMMON_RELEASE_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${COMMON_DEBUG_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${COMMON_RELEASE_FLAGS}")

  else ()
    message(STATUS "Configuring compiler flags for Clang/GCC on Linux")

    # [TRACY-FIX] Removed -D_GLIBCXX_DEBUG for profiling reliability.
    # It distorts performance metrics heavily. Use AddressSanitizer instead for debugging.
    set(COMMON_DEBUG_FLAGS "-ggdb -Og -m64 -march=native -mtune=native -pthread")

    # [TRACY-FIX] Added -g (or -ggdb) to Release.
    # Tracy needs this to map addresses to function names.
    # Removed -fprofile-arcs -ftest-coverage (These destroy performance/profiling accuracy).
    set(COMMON_RELEASE_FLAGS "-O3 -ggdb -Wall -Wextra -Wpedantic -DNDEBUG -march=native -mtune=native -flto -funroll-loops -finline-functions -m64 -pthread")

    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${COMMON_DEBUG_FLAGS}")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${COMMON_RELEASE_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${COMMON_DEBUG_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${COMMON_RELEASE_FLAGS}")
  endif ()
else ()
  message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif ()


if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  message(FATAL_ERROR "This project targets Clang toolchains. Please configure CMake with clang.")
endif ()

find_program(CCACHE_PROGRAM ccache HINTS "C:/msys64/clang64/bin" "C:/msys64/usr/bin")
if (CCACHE_PROGRAM)
  message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
else ()
  message(WARNING "ccache not found. Install it to speed up recompilation (e.g., 'pacman -S mingw-w64-clang-x86_64-ccache' in MSYS2).")
endif ()

# --- Vulkan SDK Setup ---
# Vulkan setup is now handled inside ThirdParty/CMakeLists.txt

# Disable C++ module scanning globally before any targets are defined.
# None of the third-party deps or engine sources use C++20 modules yet.
# This avoids the expensive per-TU module-dependency scan that Ninja/clang trigger.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# --- Add Dependencies (Vendored) ---
# This builds SDL3, GLM, spdlog, etc. from source
add_subdirectory(ThirdParty)

set(CMAKE_VERBOSE_MAKEFILE OFF)
# set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Place all runtime and library outputs in the build root so DLLs sit next to the executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
foreach (CONFIG DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  string(TOUPPER ${CONFIG} CONFIG_UPPER)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR})
endforeach ()

# --- Shader Compilation ---
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
file(GLOB_RECURSE SLANG_SHADERS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/shaders/*.slang")
file(GLOB_RECURSE GLSL_SHADERS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/shaders/*.vert"
    "${CMAKE_SOURCE_DIR}/shaders/*.frag"
    "${CMAKE_SOURCE_DIR}/shaders/*.comp"
    "${CMAKE_SOURCE_DIR}/shaders/*.geom"
    "${CMAKE_SOURCE_DIR}/shaders/*.tesc"
    "${CMAKE_SOURCE_DIR}/shaders/*.tese"
    "${CMAKE_SOURCE_DIR}/shaders/*.rgen"
    "${CMAKE_SOURCE_DIR}/shaders/*.rchit"
    "${CMAKE_SOURCE_DIR}/shaders/*.rmiss"
    "${CMAKE_SOURCE_DIR}/shaders/*.rahit"
    "${CMAKE_SOURCE_DIR}/shaders/*.mesh"
    "${CMAKE_SOURCE_DIR}/shaders/*.task"
)

if (WIN32)
  set(SLANGC_EXECUTABLE "${CMAKE_SOURCE_DIR}/ThirdParty/Slang-win/bin/slangc.exe")
else ()
  set(SLANGC_EXECUTABLE "${CMAKE_SOURCE_DIR}/ThirdParty/Slang-linux/bin/slangc")
endif ()

find_program(GLSLC_EXECUTABLE NAMES glslc REQUIRED)
message(STATUS "Using glslc: ${GLSLC_EXECUTABLE}")

set(COMPILED_SHADERS)
foreach (SLANG ${SLANG_SHADERS})
  file(RELATIVE_PATH SLANG_REL_PATH "${CMAKE_SOURCE_DIR}/shaders" ${SLANG})
  get_filename_component(SLANG_REL_DIR ${SLANG_REL_PATH} DIRECTORY)
  get_filename_component(SLANG_NAME_WE ${SLANG} NAME_WE)
  if (SLANG_REL_DIR STREQUAL "")
    set(SLANG_OUT_DIR "${CMAKE_BINARY_DIR}/shaders")
  else ()
    set(SLANG_OUT_DIR "${CMAKE_BINARY_DIR}/shaders/${SLANG_REL_DIR}")
  endif ()
  set(SPV_OUT "${SLANG_OUT_DIR}/${SLANG_NAME_WE}.spv")

  add_custom_command(
            OUTPUT ${SPV_OUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SLANG_OUT_DIR}
            COMMAND ${SLANGC_EXECUTABLE}
            ${SLANG}
            -target spirv
            -profile sm_6_9
            -capability SPIRV_1_6+SPV_KHR_untyped_pointers+SPV_EXT_physical_storage_buffer+spvDescriptorHeapEXT+GL_EXT_buffer_reference
            -capability vk_mem_model
            -fvk-use-entrypoint-name -g -O0
            -entry vertMain -entry fragMain
            -o ${SPV_OUT}
            DEPENDS ${SLANG}
            COMMENT "Compiling Slang shader ${SLANG_REL_PATH}"
            VERBATIM
    )
  list(APPEND COMPILED_SHADERS ${SPV_OUT})
endforeach ()

foreach (GLSL ${GLSL_SHADERS})
  file(RELATIVE_PATH GLSL_REL_PATH "${CMAKE_SOURCE_DIR}/shaders" ${GLSL})
  get_filename_component(GLSL_REL_DIR ${GLSL_REL_PATH} DIRECTORY)
  get_filename_component(GLSL_FILE_NAME ${GLSL} NAME)
  if (GLSL_REL_DIR STREQUAL "")
    set(GLSL_OUT_DIR "${CMAKE_BINARY_DIR}/shaders")
  else ()
    set(GLSL_OUT_DIR "${CMAKE_BINARY_DIR}/shaders/${GLSL_REL_DIR}")
  endif ()
  set(SPV_OUT "${GLSL_OUT_DIR}/${GLSL_FILE_NAME}.spv")

  add_custom_command(
            OUTPUT ${SPV_OUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${GLSL_OUT_DIR}
            COMMAND ${GLSLC_EXECUTABLE} --target-env=vulkan1.4 -std=460core ${GLSL} -o ${SPV_OUT}
            DEPENDS ${GLSL}
            COMMENT "Compiling GLSL shader ${GLSL_REL_PATH}"
            VERBATIM
    )
  list(APPEND COMPILED_SHADERS ${SPV_OUT})
endforeach ()

add_custom_target(Shaders ALL DEPENDS ${COMPILED_SHADERS})

# --- Paths Definitions ---
file(TO_CMAKE_PATH "${CMAKE_BINARY_DIR}/shaders" ENGINE_SHADER_DIR_PATH)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/models" ENGINE_MODELS_DIR_PATH)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/textures" ENGINE_TEXTURES_DIR_PATH)

add_subdirectory(src)
