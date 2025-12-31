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
if(NOT DEFINED ENV{VULKAN_SDK})
    message(FATAL_ERROR "Environment variable VULKAN_SDK must point to your Vulkan SDK root.")
endif()

set(VULKAN_SDK_ROOT "$ENV{VULKAN_SDK}")
set(Vulkan_INCLUDE_DIR "${VULKAN_SDK_ROOT}/include" CACHE PATH "Vulkan Include Path" FORCE)

list(APPEND CMAKE_PREFIX_PATH
    "${VULKAN_SDK_ROOT}"
    "${VULKAN_SDK_ROOT}/lib/cmake"
    "${VULKAN_SDK_ROOT}/Lib/cmake"
    "${VULKAN_SDK_ROOT}/lib/cmake/Vulkan"
    "${VULKAN_SDK_ROOT}/Lib/cmake/Vulkan"
)

# --- Add Dependencies (Vendored) ---
# This builds SDL3, Boost, GLM, etc. from source
add_subdirectory(ThirdParty)

set(CMAKE_VERBOSE_MAKEFILE OFF)

# Only find Vulkan from system/SDK (everything else is in ThirdParty)
find_library(Vulkan_LIBRARY
    NAMES vulkan libvulkan
    PATHS "${VULKAN_SDK_ROOT}/lib"
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
)
find_package(Vulkan REQUIRED)

# --- DEBUG INFO ---
message(STATUS "-------------------------------------------------------------")
message(STATUS "Dependency Paths:")
message(STATUS "  Vulkan SDK Root:    ${VULKAN_SDK_ROOT}")
message(STATUS "  Vulkan Include:     ${Vulkan_INCLUDE_DIR}")
message(STATUS "  Vulkan Library:     ${Vulkan_LIBRARY}")
message(STATUS "  (Other libraries are built from source in ThirdParty/)")
message(STATUS "-------------------------------------------------------------")

# --- Vulkan Header-Only Setup ---
set(VULKAN_COMPILE_DEFS
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    VULKAN_HPP_NO_STD_MODULE=1
    VULKAN_HPP_NO_MODULE=1
)

message(STATUS "Using Vulkan headers (modules disabled)")
add_library(VulkanCppModule INTERFACE)
target_compile_definitions(VulkanCppModule INTERFACE ${VULKAN_COMPILE_DEFS})
target_include_directories(VulkanCppModule INTERFACE "${Vulkan_INCLUDE_DIR}")
target_link_libraries(VulkanCppModule INTERFACE Vulkan::Vulkan)

add_library(Vulkan::cppm ALIAS VulkanCppModule)

# --- Engine Executable ---
message(STATUS "CMake is using C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")

add_executable(engine
    src/main.cpp
    src/pch.h
    src/Constants.h
    src/core/vk_engine.cpp
    src/core/vk_engine.hpp
    src/core/vk_device.cpp
    src/core/vk_device.hpp
    src/core/vk_swapchain.cpp
    src/core/vk_swapchain.hpp
    src/core/vk_resource_manager.cpp
    src/core/vk_resource_manager.hpp
    src/core/vk_descriptors.cpp
    src/core/vk_descriptors.hpp
    src/core/types.hpp
    src/assets/assets_loader.cpp
    src/assets/assets_loader.hpp
    src/assets/model_storage.cpp
    src/assets/model_storage.hpp
    src/assets/texture_manager.cpp
    src/assets/texture_manager.hpp
    src/render/vk_pipeline.cpp
    src/render/vk_pipeline.hpp
    src/render/vk_renderer.cpp
    src/render/vk_renderer.hpp
    src/util/logger.cpp
    src/util/logger.hpp
    ThirdParty/implementations.cpp
    ThirdParty/termcolor.hpp
    ThirdParty/tiny_obj_loader.h
    ThirdParty/vk_mem_alloc.h
        src/util/debug.cpp
)

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
    ThirdParty::all  # This links SDL3, Boost, GLM, spdlog, vk-bootstrap
    Vulkan::Vulkan   # This links the Vulkan SDK
)

target_link_libraries(engine PRIVATE
    ${CMAKE_DL_LIBS}
    Vulkan::cppm
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

# --- Copy DLLs to Executable Directory ---
if(WIN32)
    # Copy SDL3 DLL
    add_custom_command(TARGET engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3-shared>
        $<TARGET_FILE_DIR:engine>
        COMMENT "Copying SDL3 DLL to executable directory"
    )
    
    # Copy Boost DLLs if they exist
    if(TARGET Boost::filesystem)
        add_custom_command(TARGET engine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Boost::filesystem>
            $<TARGET_FILE_DIR:engine>
            COMMENT "Copying Boost::filesystem DLL to executable directory"
        )
    endif()
    
    if(TARGET Boost::log)
        add_custom_command(TARGET engine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Boost::log>
            $<TARGET_FILE_DIR:engine>
            COMMENT "Copying Boost::log DLL to executable directory"
        )
    endif()
    
    if(TARGET Boost::thread)
        add_custom_command(TARGET engine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Boost::thread>
            $<TARGET_FILE_DIR:engine>
            COMMENT "Copying Boost::thread DLL to executable directory"
        )
    endif()
    
    # Copy fmt DLL if it's shared
    if(TARGET fmt::fmt)
        add_custom_command(TARGET engine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:fmt::fmt>
            $<TARGET_FILE_DIR:engine>
            COMMENT "Copying fmt DLL to executable directory"
        )
    endif()
    
    # Copy spdlog DLL if it's shared
    if(TARGET spdlog::spdlog)
        add_custom_command(TARGET engine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:spdlog::spdlog>
            $<TARGET_FILE_DIR:engine>
            COMMENT "Copying spdlog DLL to executable directory"
        )
    endif()
    
    # Copy glm DLL if it exists (shouldn't normally, but handle it)
    if(TARGET glm::glm)
        add_custom_command(TARGET engine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:glm::glm>
            $<TARGET_FILE_DIR:engine>
            COMMENT "Copying glm DLL to executable directory"
        )
    endif()
else()
    # Linux: Set RPATH to find shared libraries
    set_target_properties(engine PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
endif()