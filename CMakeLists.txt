cmake_minimum_required(VERSION 3.29)

project(engine)
if(POLICY CMP0167)
        cmake_policy(SET CMP0167 OLD)
endif()


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_EXPERIMENTAL_CXX_MODULES ON CACHE BOOL "" FORCE)
set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message(STATUS "CMAKE_Version: ${CMAKE_VERSION}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

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
        message(FATAL_ERROR "This project targets Clang or GCC toolchains. Please configure CMake with clang or gcc.")
endif()

if(NOT DEFINED ENV{VULKAN_SDK})
        message(FATAL_ERROR "Environment variable VULKAN_SDK must point to your Vulkan SDK root.")
endif()

set(VULKAN_SDK_ROOT "$ENV{VULKAN_SDK}")

# if(UNIX AND NOT APPLE)
#         set(_default_vcpkg "$ENV{HOME}/.vcpkg-clion/vcpkg/installed/x64-linux-dynamic")
#         if(EXISTS "${_default_vcpkg}")
#                 list(APPEND CMAKE_PREFIX_PATH "${_default_vcpkg}" "${_default_vcpkg}/share")
#                 include_directories("${_default_vcpkg}/include")
#         endif()
# endif()


set(Vulkan_INCLUDE_DIR "${VULKAN_SDK_ROOT}/include" CACHE PATH "Vulkan Include Path" FORCE)


list(APPEND CMAKE_PREFIX_PATH
                "${VULKAN_SDK_ROOT}"
                "${VULKAN_SDK_ROOT}/lib/cmake"
                "${VULKAN_SDK_ROOT}/Lib/cmake"
                "${VULKAN_SDK_ROOT}/lib/cmake/Vulkan"
                "${VULKAN_SDK_ROOT}/Lib/cmake/Vulkan")

add_subdirectory(ThirdParty)

set(CMAKE_VERBOSE_MAKEFILE OFF)
find_library(Vulkan_LIBRARY
    NAMES vulkan libvulkan
    PATHS "${VULKAN_SDK_ROOT}/lib"
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
)
find_package(Vulkan REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem log system)
find_package(SDL3 CONFIG REQUIRED)

# --- DEBUG INFO: Print found paths ---
message(STATUS "-------------------------------------------------------------")
message(STATUS "Dependency Paths:")
message(STATUS "  Vulkan SDK Root:    ${VULKAN_SDK_ROOT}")
message(STATUS "  Vulkan Include:     ${Vulkan_INCLUDE_DIR}")
message(STATUS "  Vulkan Library:     ${Vulkan_LIBRARY}")
message(STATUS "  SDL3_DIR:           ${SDL3_DIR}")
message(STATUS "  vk-bootstrap_DIR:   ${vk-bootstrap_DIR}")
message(STATUS "  glm_DIR:            ${glm_DIR}")
message(STATUS "  Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "-------------------------------------------------------------")

add_library(VulkanCppModule)

# Fix for Linux/Clang "declaration does not declare anything" error in vulkan.cppm
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(VulkanCppModule PRIVATE -fpermissive)
endif()

add_library(Vulkan::cppm ALIAS VulkanCppModule)
target_compile_definitions(VulkanCppModule PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)

target_include_directories(VulkanCppModule
        BEFORE  # <--- IMPORTANT: This forces SDK headers to check first, preventing shadowing
        PRIVATE
        "${Vulkan_INCLUDE_DIR}"
)
target_link_libraries(VulkanCppModule
        PUBLIC
        Vulkan::Vulkan
)

set_target_properties(VulkanCppModule PROPERTIES CXX_STANDARD 23)

target_sources(VulkanCppModule
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS
        "${Vulkan_INCLUDE_DIR}"
        FILES
        "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
)

# Add the vulkan.cppm file directly as a source file
target_sources(VulkanCppModule
        PRIVATE
        "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
)

set(VK_BOOTSTRAP_WERROR	ON CACHE BOOL "Enable warnings as errors for vk-bootstrap")
if(DEFINED Vulkan_INCLUDE_DIRS)
        message(STATUS "Vulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}")
endif()
if(DEFINED Vulkan_LIBRARIES)
        message(STATUS "Vulkan_LIBRARIES: ${Vulkan_LIBRARIES}")
endif()
message(STATUS "CMake is using C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
message(${CMAKE_SOURCE_DIR})



add_executable(engine
        src/core/main.cpp
        src/core/Application.cpp
        src/core/Application.h
        src/graphics/VulkanContext.cpp
        src/graphics/VulkanContext.h
        src/graphics/SwapChain.cpp
        src/graphics/SwapChain.h
        src/resources/ResourceManager.cpp
        src/resources/ResourceManager.h
        src/resources/TextureManager.cpp
        src/resources/TextureManager.h
        src/resources/AssetsLoader.cpp
        src/resources/AssetsLoader.h
        src/graphics/DescriptorManager.cpp
        src/graphics/DescriptorManager.h
        src/graphics/Pipeline.cpp
        src/graphics/Pipeline.h
        src/graphics/Renderer.cpp
        src/graphics/Renderer.h
        src/core/Utils.cpp
        src/core/Utils.h
        ThirdParty/termcolor.hpp
        ThirdParty/tiny_obj_loader.h
        ThirdParty/vk_mem_alloc.h
        ThirdParty/implementations.cpp
)


# set(Boost_USE_STATIC_LIBS OFF)
# set(Boost_USE_MULTITHREADED ON)
# set(Boost_USE_STATIC_RUNTIME OFF)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
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
#     set(SPV_OUT "${CMAKE_SOURCE_DIR}/cmake-build-debug/shaders/${NAME_WE}.spv")
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
            COMMENT     "Compiling shader ${NAME_WE}.slang → ${NAME_WE}.spv"
    )

    list(APPEND COMPILED_SHADERS ${SPV_OUT})
endforeach()

# Create a target that builds all SPIR-V files
add_custom_target(Shaders ALL
        DEPENDS ${COMPILED_SHADERS}
)


#set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
#set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
#set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

if(Vulkan_INCLUDE_DIRS)
        target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
endif()
target_link_libraries(engine PUBLIC
        Boost::filesystem
        Boost::log
        Boost::system
        glfw
        glm::glm
        SDL3::SDL3
        ThirdParty::headers

)

set(VK_BOOTSTRAP_TARGETS)
if(TARGET VkBootstrap::VkBootstrap)
        list(APPEND VK_BOOTSTRAP_TARGETS VkBootstrap::VkBootstrap)
endif()
if(TARGET vk-bootstrap::vk-bootstrap)
        list(APPEND VK_BOOTSTRAP_TARGETS vk-bootstrap::vk-bootstrap)
endif()
if(TARGET vk-bootstrap::vk-bootstrap-compiler-warnings AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        list(APPEND VK_BOOTSTRAP_TARGETS vk-bootstrap::vk-bootstrap-compiler-warnings)
endif()

if(NOT VK_BOOTSTRAP_TARGETS)
        message(FATAL_ERROR "vk-bootstrap CMake target not found. Ensure vk-bootstrap is installed via vcpkg and the toolchain is configured.")
endif()

target_link_libraries(engine PRIVATE
        ${VK_BOOTSTRAP_TARGETS}
        ${CMAKE_DL_LIBS}
        Vulkan::cppm
        # vulkan_module
)

file(TO_CMAKE_PATH "${CMAKE_BINARY_DIR}/shaders" ENGINE_SHADER_DIR_PATH)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/models" ENGINE_MODELS_DIR_PATH)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/textures" ENGINE_TEXTURES_DIR_PATH)
target_compile_definitions(engine PRIVATE
        ENGINE_SHADER_DIR="${ENGINE_SHADER_DIR_PATH}"
        ENGINE_MODELS_DIR="${ENGINE_MODELS_DIR_PATH}"
        ENGINE_TEXTURES_DIR="${ENGINE_TEXTURES_DIR_PATH}"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        if(WIN32)
                # Windows with Clang (MSVC ABI) - no libstdc++ debug flags
                set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -m64 -march=native -mtune=native -flto")
                set(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall -Wextra -Wpedantic -DNDEBUG -march=native -mtune=native -flto \
        -funroll-loops -finline-functions -fprefetch-loop-arrays -m64")
        else()
                # Linux with Clang/GCC (GCC ABI) - include libstdc++ debug flags
                set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC \
 -Og -m64 -march=native -mtune=native -flto -pthread")
                set(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall -Wextra -Wpedantic -DNDEBUG -march=native -mtune=native -flto \
        -funroll-loops -finline-functions -fprefetch-loop-arrays -m64 -pthread")
        endif()
else()
        message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()


add_dependencies(engine Shaders)