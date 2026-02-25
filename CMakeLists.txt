cmake_minimum_required(VERSION 3.29)

project(engine)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()


# --- Standard Configuration ---
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_LINKER_TYPE LLD)
# set(CMAKE_EXE_LINKER_FLAGS "{CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld --coverage")

# if(UNIX AND NOT APPLE)
#     add_compile_options(-stdlib=libc++)
#     add_link_options(-stdlib=libc++)
# endif()
message(STATUS "CMAKE_Version: ${CMAKE_VERSION}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "Detected Clang/GCC compiler: ${CMAKE_CXX_COMPILER_ID}")

    if(WIN32)
        message(STATUS "Configuring compiler flags for Clang on Windows")

        # [TRACY-FIX] Generate PDB file for the linker
        add_link_options($<$<CONFIG:Debug>:-Wl,--pdb=>)
        # [TRACY-FIX] Also generate PDB for RelWithDebInfo if you use it
        add_link_options($<$<CONFIG:RelWithDebInfo>:-Wl,--pdb=>)

        # [TRACY-FIX] Added -gcodeview. This converts DWARF to Microsoft CodeView format
        # so dbghelp.dll (used by Tracy) can read it.
        set(COMMON_DEBUG_FLAGS "-O0 -g -m64 -march=native -mtune=native -pthread")

        # [TRACY-FIX] amAdded -g to Release. This allows Tracy to see function names
        # in optimized builds without slowing down the code.
        set(COMMON_RELEASE_FLAGS "-O3 -g -Wall -Wextra -Wpedantic -DNDEBUG -march=native -mtune=native -flto -funroll-loops -finline-functions -m64 -pthread")

        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${COMMON_DEBUG_FLAGS}")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${COMMON_RELEASE_FLAGS}")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${COMMON_DEBUG_FLAGS}")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${COMMON_RELEASE_FLAGS}")

    else()
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
    endif()
else()
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()


## --- Compiler Setup ---
#if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
#    foreach(_flag_var CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS_MINSIZEREL
#                     CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_MINSIZEREL)
#        if(DEFINED ${_flag_var})
#            string(REPLACE "/W4" "" ${_flag_var} "${${_flag_var}}")
#            string(REPLACE "/W3" "" ${_flag_var} "${${_flag_var}}")
#            string(REPLACE "/W2" "" ${_flag_var} "${${_flag_var}}")
#            string(REPLACE "/W1" "" ${_flag_var} "${${_flag_var}}")
#            string(REPLACE "/WX" "" ${_flag_var} "${${_flag_var}}")
#            set(${_flag_var} "${${_flag_var}}" CACHE STRING "" FORCE)
#        endif()
#    endforeach()
#endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "This project targets Clang toolchains. Please configure CMake with clang.")
endif()

find_program(CCACHE_PROGRAM ccache HINTS "C:/msys64/clang64/bin" "C:/msys64/usr/bin")
if(CCACHE_PROGRAM)
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
else()
    message(WARNING "ccache not found. Install it to speed up recompilation (e.g., 'pacman -S mingw-w64-clang-x86_64-ccache' in MSYS2).")
endif()

# --- Vulkan SDK Setup ---
# Vulkan setup is now handled inside ThirdParty/CMakeLists.txt

# --- Add Dependencies (Vendored) ---
# This builds SDL3, Boost, GLM, etc. from source
add_subdirectory(ThirdParty)

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Place all runtime and library outputs in the build root so DLLs sit next to the executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
foreach(CONFIG DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR})
endforeach()

# --- Shader Compilation ---
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
file(GLOB SLANG_SHADERS "${CMAKE_SOURCE_DIR}/shaders/shader.slang")

if(WIN32)
    set(SLANGC_EXECUTABLE "ThirdParty/Slang-win/bin/slangc.exe")
else()
    set(SLANGC_EXECUTABLE "ThirdParty/Slang-linux/bin/slangc")
endif()

set(COMPILED_SHADERS)
foreach(SLANG ${SLANG_SHADERS})
    get_filename_component(NAME_WE ${SLANG} NAME_WE)
    set(SPV_OUT "${CMAKE_BINARY_DIR}/shaders/${NAME_WE}.spv")

    add_custom_command(
        OUTPUT      ${SPV_OUT}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND     ${SLANGC_EXECUTABLE}
        shaders/${NAME_WE}.slang
        -target spirv
        -profile spirv_1_6
        -fvk-use-entrypoint-name
        -entry vertMain -entry fragMain
        -o ${SPV_OUT}
        DEPENDS     shaders/${NAME_WE}.slang
        COMMENT     "Compiling shader ${NAME_WE}.slang -> ${NAME_WE}.spv"
    )
    list(APPEND COMPILED_SHADERS ${SPV_OUT})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${COMPILED_SHADERS})

# --- Paths Definitions ---
file(TO_CMAKE_PATH "${CMAKE_BINARY_DIR}/shaders" ENGINE_SHADER_DIR_PATH)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/models" ENGINE_MODELS_DIR_PATH)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/textures" ENGINE_TEXTURES_DIR_PATH)

add_subdirectory(src)

