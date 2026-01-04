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

# if(UNIX AND NOT APPLE)
#     add_compile_options(-stdlib=libc++)
#     add_link_options(-stdlib=libc++)
# endif()

message(STATUS "CMAKE_Version: ${CMAKE_VERSION}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# --- Compiler Setup ---
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    foreach(_flag_var CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS_MINSIZEREL
                     CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_MINSIZEREL)
        if(DEFINED ${_flag_var})
            string(REPLACE "/W4" "" ${_flag_var} "${${_flag_var}}")
            string(REPLACE "/W3" "" ${_flag_var} "${${_flag_var}}")
            string(REPLACE "/W2" "" ${_flag_var} "${${_flag_var}}")
            string(REPLACE "/W1" "" ${_flag_var} "${${_flag_var}}")
            string(REPLACE "/WX" "" ${_flag_var} "${${_flag_var}}")
            set(${_flag_var} "${${_flag_var}}" CACHE STRING "" FORCE)
        endif()
    endforeach()
endif()

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

# --- Engine Executable ---
message(STATUS "CMake is using C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")


file(GLOB_RECURSE ENGINE_SRC_FILES
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/ThirdParty/*.cpp"
)

file(GLOB_RECURSE ENGINE_HEADER_FILES
        "${CMAKE_SOURCE_DIR}/src/*.hpp"
        "${CMAKE_SOURCE_DIR}/src/*.h"
        "${CMAKE_SOURCE_DIR}/ThirdParty/*.h"
        "${CMAKE_SOURCE_DIR}/ThirdParty/*.hpp"
)

add_executable(engine
        ${ENGINE_SRC_FILES}
        ${ENGINE_HEADER_FILES}
)

#add_executable(engine
#    src/main.cpp
#    src/pch.h
#    src/Constants.h
#    src/core/vk_engine.cpp
#    src/core/vk_engine.hpp
#    src/core/vk_device.cpp
#    src/core/vk_device.hpp
#    src/core/vk_swapchain.cpp
#    src/core/vk_swapchain.hpp
#    src/core/vk_resource_manager.cpp
#    src/core/vk_resource_manager.hpp
#    src/core/vk_descriptors.cpp
#    src/core/vk_descriptors.hpp
#    src/core/types.hpp
#    src/assets/assets_loader.cpp
#    src/assets/assets_loader.hpp
#    src/assets/model_storage.cpp
#    src/assets/model_storage.hpp
#    src/assets/texture_manager.cpp
#    src/assets/texture_manager.hpp
#    src/render/vk_pipeline.cpp
#    src/render/vk_pipeline.hpp
#    src/render/vk_renderer.cpp
#    src/render/vk_renderer.hpp
#    src/util/logger.cpp
#    src/util/logger.hpp
#        src/core/vk_allocator.hpp
#        src/core/vk_allocator.hpp
#    ThirdParty/implementations.cpp
#    ThirdParty/termcolor.hpp
#    ThirdParty/tiny_obj_loader.h
#    ThirdParty/vk_mem_alloc.h
#        src/util/debug.cpp
#)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

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

# --- Linking ---
target_link_libraries(engine PUBLIC
    ThirdParty::all  # This links SDL3, Boost, GLM, spdlog, vk-bootstrap, Vulkan
)

target_link_libraries(engine PRIVATE
    ${CMAKE_DL_LIBS}
)


# --- Precompiled Headers ---
target_precompile_headers(engine PRIVATE src/pch.h)

# --- Paths Definitions ---
file(TO_CMAKE_PATH "${CMAKE_BINARY_DIR}/shaders" ENGINE_SHADER_DIR_PATH)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/models" ENGINE_MODELS_DIR_PATH)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/textures" ENGINE_TEXTURES_DIR_PATH)

target_compile_definitions(engine PRIVATE
    ENGINE_SHADER_DIR="${ENGINE_SHADER_DIR_PATH}"
    ENGINE_MODELS_DIR="${ENGINE_MODELS_DIR_PATH}"
    ENGINE_TEXTURES_DIR="${ENGINE_TEXTURES_DIR_PATH}"
)



# --- Manual Compiler Flags ---
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if(WIN32)
        # Windows with Clang (MSVC ABI) - no libstdc++ debug flags
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -m64 -march=native -mtune=native")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall -Wextra -Wpedantic -DNDEBUG -march=native -mtune=native -flto -funroll-loops -finline-functions -m64")
    else()
        # Linux with Clang/GCC (GCC ABI) - include libstdc++ debug flags
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -Og -m64 -march=native -mtune=native -pthread")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall -Wextra -Wpedantic -DNDEBUG -march=native -mtune=native -flto -funroll-loops -finline-functions -m64 -pthread")
    endif()
else()
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

add_dependencies(engine Shaders)


if(WIN32)
    # 1. Define list of targets whose DLLs need copying
    #    NOTE: We added Boost::log_setup and Boost::atomic because 
    #    Boost::log and Boost::filesystem often depend on them.
    set(SHARED_LIBS_TO_COPY
        SDL3::SDL3
        fmt::fmt
        spdlog::spdlog
        Boost::filesystem
        Boost::log
        Boost::log_setup
        Boost::thread
        Boost::atomic
        vk-bootstrap::vk-bootstrap 
    )

    # 2. Iterate and create copy commands
    foreach(_LIB ${SHARED_LIBS_TO_COPY})
        # Check if the target exists (in case you disabled a lib)
        if(TARGET ${_LIB})
            # Check if it is actually a shared library or executable (has a runtime file)
            get_target_property(_type ${_LIB} TYPE)
            
            if("${_type}" STREQUAL "SHARED_LIBRARY" OR "${_type}" STREQUAL "MODULE_LIBRARY")
                add_custom_command(TARGET engine POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:${_LIB}>
                    $<TARGET_FILE_DIR:engine>
                    COMMENT "Copying DLL for ${_LIB}..."
                )
            endif()
        endif()
    endforeach()
else()
    # Linux/macOS: RPATH settings
    # This ensures the executable looks in its own directory for .so files
    set_target_properties(engine PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN"
    )
endif()