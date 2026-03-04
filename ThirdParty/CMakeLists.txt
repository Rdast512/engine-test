# ThirdParty/CMakeLists.txt
include(FetchContent)


# Keep downloaded third-party sources/builds inside ThirdParty/_deps/<config> so
# debug/release (or multi-config) artifacts stay separated and survive cache wipes.
if (CMAKE_CONFIGURATION_TYPES)
    set(_thirdparty_cfg_dir "multi-config")
else ()
    string(TOLOWER "${CMAKE_BUILD_TYPE}" _thirdparty_cfg_dir)
endif ()
set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/_deps/${_thirdparty_cfg_dir}" CACHE PATH "Where FetchContent stores dependency sources/builds" FORCE)
message(STATUS "FetchContent base dir: ${FETCHCONTENT_BASE_DIR}")

# Optimization: Disable C++ Module scanning for third-party libs
# They don't use modules yet, so scanning them is wasted time.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# ------------------------------------------------------------------------------
# Vulkan SDK (found via VULKAN_SDK env var)
# ------------------------------------------------------------------------------
if (NOT DEFINED ENV{VULKAN_SDK})
    message(FATAL_ERROR "Environment variable VULKAN_SDK must point to your Vulkan SDK root.")
endif ()

set(VULKAN_SDK_ROOT "$ENV{VULKAN_SDK}")
set(Vulkan_INCLUDE_DIR "${VULKAN_SDK_ROOT}/include" CACHE PATH "Vulkan Include Path" FORCE)

list(APPEND CMAKE_PREFIX_PATH
        "${VULKAN_SDK_ROOT}"
        "${VULKAN_SDK_ROOT}/lib/cmake"
        "${VULKAN_SDK_ROOT}/Lib/cmake"
        "${VULKAN_SDK_ROOT}/lib/cmake/Vulkan"
        "${VULKAN_SDK_ROOT}/Lib/cmake/Vulkan"
)

find_library(Vulkan_LIBRARY
        NAMES vulkan libvulkan
        PATHS "${VULKAN_SDK_ROOT}/lib"
        NO_DEFAULT_PATH
        NO_CMAKE_FIND_ROOT_PATH
)
find_package(Vulkan REQUIRED)

message(STATUS "-------------------------------------------------------------")
message(STATUS "Vulkan SDK Configuration:")
message(STATUS "  Vulkan SDK Root:    ${VULKAN_SDK_ROOT}")
message(STATUS "  Vulkan Include:     ${Vulkan_INCLUDE_DIR}")
message(STATUS "  Vulkan Library:     ${Vulkan_LIBRARY}")
message(STATUS "-------------------------------------------------------------")

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

# ------------------------------------------------------------------------------
# Vulkan Memory Allocator (from Vulkan SDK)
# ------------------------------------------------------------------------------
set(VMA_HEADER_PATH "${VULKAN_SDK_ROOT}/include/vma/vk_mem_alloc.h")
if (NOT EXISTS "${VMA_HEADER_PATH}")
    message(FATAL_ERROR "VMA header not found at ${VMA_HEADER_PATH}. Ensure Vulkan SDK includes VMA.")
endif ()

message(STATUS "Using VMA from Vulkan SDK: ${VMA_HEADER_PATH}")
add_library(VulkanMemoryAllocator INTERFACE)
target_include_directories(VulkanMemoryAllocator INTERFACE "${VULKAN_SDK_ROOT}/include")
target_link_libraries(VulkanMemoryAllocator INTERFACE Vulkan::Vulkan)

# ------------------------------------------------------------------------------
# 1. Slang (Manual Download & Extract)
# ------------------------------------------------------------------------------
set(SLANG_TAG "v2026.3.1")
string(REGEX REPLACE "^v" "" SLANG_VERSION "${SLANG_TAG}")
set(SLANG_RELEASE_URL "https://github.com/shader-slang/slang/releases/download")

if (WIN32)
    set(SLANG_OS_NAME "win")
    set(SLANG_ARTIFACT_NAME "slang-${SLANG_VERSION}-windows-x86_64.zip")
elseif (UNIX AND NOT APPLE)
    set(SLANG_OS_NAME "linux")
    set(SLANG_ARTIFACT_NAME "slang-${SLANG_VERSION}-linux-x86_64.zip")
else ()
    message(FATAL_ERROR "Unsupported OS for Slang.")
endif ()

set(SLANG_DOWNLOAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/slang_download")
set(SLANG_EXTRACT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Slang-${SLANG_OS_NAME}")
set(SLANG_DOWNLOAD_URL "${SLANG_RELEASE_URL}/${SLANG_TAG}/${SLANG_ARTIFACT_NAME}")

if (NOT EXISTS "${SLANG_EXTRACT_DIR}/README.md")
    message(STATUS "Downloading Slang from ${SLANG_DOWNLOAD_URL}")
    file(DOWNLOAD ${SLANG_DOWNLOAD_URL} "${SLANG_DOWNLOAD_DIR}/${SLANG_ARTIFACT_NAME}" SHOW_PROGRESS STATUS download_status)
    list(GET download_status 0 error_code)

    if (NOT error_code EQUAL 0)
        list(GET download_status 1 error_msg)
        message(FATAL_ERROR "Failed to download Slang: ${error_msg}")
    endif ()

    message(STATUS "Extracting Slang to ${SLANG_EXTRACT_DIR}...")
    file(ARCHIVE_EXTRACT INPUT "${SLANG_DOWNLOAD_DIR}/${SLANG_ARTIFACT_NAME}" DESTINATION "${SLANG_EXTRACT_DIR}")
    file(REMOVE_RECURSE "${SLANG_DOWNLOAD_DIR}")
endif ()

# ==============================================================================
# FetchContent — Options set BEFORE declare/populate so they are visible
# when CMake configures each sub-project during MakeAvailable.
# ==============================================================================

# --- SDL3 ---
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(SDL_VULKAN ON CACHE BOOL "" FORCE)
if (UNIX AND NOT APPLE)
    set(SDL_WAYLAND ON CACHE BOOL "" FORCE)
    set(SDL_X11 ON CACHE BOOL "" FORCE)
endif ()

# --- fmt ---
set(FMT_SHARED ON CACHE BOOL "" FORCE)
set(FMT_DOC OFF CACHE BOOL "" FORCE)
set(FMT_TEST OFF CACHE BOOL "" FORCE)

# --- spdlog ---
set(SPDLOG_BUILD_SHARED ON CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_PIC ON CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "" FORCE)

# --- Boost ---
set(BOOST_INCLUDE_LIBRARIES filesystem algorithm thread log asio CACHE STRING "" FORCE)
set(BOOST_IOSTREAMS_ENABLE_ZLIB OFF CACHE BOOL "" FORCE)
set(BOOST_IOSTREAMS_ENABLE_BZIP2 OFF CACHE BOOL "" FORCE)
set(BOOST_IOSTREAMS_ENABLE_LZMA OFF CACHE BOOL "" FORCE)
set(BOOST_IOSTREAMS_ENABLE_ZSTD OFF CACHE BOOL "" FORCE)

# ==============================================================================
# FetchContent Declares — ALL declared before MakeAvailable so CMake can
# resolve them as a batch and avoid redundant network/disk operations.
# ==============================================================================

# 2. GLM (Header Only)
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.2
        GIT_SHALLOW TRUE
)

# 3. SDL3
FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.2
        GIT_SHALLOW TRUE
)

# 4. fmt
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 12.1.0
        GIT_SHALLOW TRUE
)

# 5. spdlog
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.16.0
        GIT_SHALLOW TRUE
)

# 6. Boost
FetchContent_Declare(
        Boost
        URL "https://github.com/boostorg/boost/releases/download/boost-1.90.0/boost-1.90.0-cmake.tar.xz"
)

# 7. ImGui
FetchContent_Declare(
        ImGui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.92.6
        GIT_SHALLOW TRUE
)

# 8. Taskflow
FetchContent_Declare(
        taskflow
        GIT_REPOSITORY https://github.com/taskflow/taskflow.git
        GIT_TAG v4.0.0
        GIT_SHALLOW TRUE
)

# 9. RapidJSON (header-only)
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        rapidjson
        GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
        GIT_TAG master
        GIT_SHALLOW TRUE
)

# ==============================================================================
# Populate all dependencies in a single batch call.
# Tracy is declared and populated separately below (after GIT_SHALLOW/PROGRESS).
# ==============================================================================
message(STATUS "Fetching ThirdParty dependencies (glm, SDL3, fmt, spdlog, Boost, ImGui, taskflow, rapidjson)...")
FetchContent_MakeAvailable(glm SDL3 fmt spdlog Boost ImGui taskflow rapidjson)

# ------------------------------------------------------------------------------
set(IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        # BACKENDS
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

# NOTE: Keeping ImGui STATIC is highly recommended even in a shared build.
# Exporting ImGui symbols across DLL boundaries requires complex configuration.
add_library(imgui STATIC ${IMGUI_SOURCES})

# UNITY_BUILD merges all 7 imgui .cpp files into one TU — ~3-4x faster compile.
# PIC required so imgui can link into shared engine libraries.
set_target_properties(imgui PROPERTIES
        UNITY_BUILD ON
        POSITION_INDEPENDENT_CODE ON
)

target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC
        SDL3::SDL3
        Vulkan::Vulkan
)


# ------------------------------------------------------------------------------
# 8. Tracy
# ------------------------------------------------------------------------------

FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG v0.13.1
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
message(STATUS "Downloading Tracy...")
FetchContent_MakeAvailable(tracy)

# Use upstream Tracy target to keep a single Tracy client library.
# Disable system tracing globally for this workspace's Tracy integration.
if (TARGET TracyClient)
        target_compile_definitions(TracyClient PUBLIC TRACY_NO_SYSTEM_TRACING)
endif ()

if (WIN32)
        # Keep Windows target compatibility and required system libs for Tracy.
        target_compile_definitions(TracyClient PRIVATE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
        target_link_libraries(TracyClient PRIVATE ws2_32 dbghelp user32)
endif ()


# ------------------------------------------------------------------------------
# Targets Definition
# ------------------------------------------------------------------------------

# 1. Headers-only interface
add_library(ThirdPartyHeaders INTERFACE)
target_include_directories(ThirdPartyHeaders INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${SLANG_EXTRACT_DIR}/include>
        $<BUILD_INTERFACE:${rapidjson_SOURCE_DIR}/include>
)
add_library(ThirdParty::headers ALIAS ThirdPartyHeaders)

# 2. All-in-one Interface for libs
add_library(ThirdPartyLibs INTERFACE)
target_link_libraries(ThirdPartyLibs INTERFACE
        ThirdParty::headers
        Vulkan::Vulkan
        Vulkan::cppm
        VulkanMemoryAllocator
        glm::glm
        SDL3::SDL3
        #  vk-bootstrap::vk-bootstrap
        spdlog::spdlog
        Tracy::TracyClient
        fmt::fmt
        # Boost Targets
        imgui
        Boost::filesystem
        Boost::log
        Boost::system
        Taskflow::Taskflow
)


add_library(ThirdParty::all ALIAS ThirdPartyLibs)