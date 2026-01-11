# ThirdParty/CMakeLists.txt
include(FetchContent)

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
set(SLANG_TAG "v2025.24")
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

# ------------------------------------------------------------------------------
# 2. GLM (Header Only)
# ------------------------------------------------------------------------------
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.2
)
message(STATUS "Downloading GLM...")
FetchContent_MakeAvailable(glm)

# ------------------------------------------------------------------------------
# 3. SDL3 (Build from source)
# ------------------------------------------------------------------------------
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

FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.0
)
message(STATUS "Downloading SDL3...")
FetchContent_MakeAvailable(SDL3)

## ------------------------------------------------------------------------------
## 4. vk-bootstrap
## ------------------------------------------------------------------------------
#set(VK_BOOTSTRAP_TEST OFF CACHE BOOL "" FORCE)
#FetchContent_Declare(
#    vk-bootstrap
#    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
#    GIT_TAG        v1.4.332
#)
#message(STATUS "Downloading vk-bootstrap...")
#FetchContent_MakeAvailable(vk-bootstrap)


# ------------------------------------------------------------------------------
# 4. volk
# ------------------------------------------------------------------------------
#FetchContent_Declare(
#    volk
#        GIT_REPOSITORY https://github.com/zeux/volk.git
#)
#message(STATUS "Downloading volk...")
#FetchContent_MakeAvailable(volk)

# ------------------------------------------------------------------------------
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 12.1.0
)
set(FMT_SHARED ON CACHE BOOL "" FORCE)
set(FMT_DOC OFF CACHE BOOL "" FORCE)
set(FMT_TEST OFF CACHE BOOL "" FORCE)
message(STATUS "Downloading fmt...")
FetchContent_MakeAvailable(fmt)

# ------------------------------------------------------------------------------
# 5. spdlog
# ------------------------------------------------------------------------------
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.16.0
)
set(SPDLOG_BUILD_SHARED ON CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_PIC ON CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "" FORCE)
message(STATUS "Downloading spdlog...")
FetchContent_MakeAvailable(spdlog)


# ------------------------------------------------------------------------------
# 6. Boost (Compiled from Source)
# ------------------------------------------------------------------------------
message(STATUS "Fetching Boost (selected libraries only)...")

# Only build these libraries (and their dependencies)
set(BOOST_INCLUDE_LIBRARIES filesystem algorithm thread log asio CACHE STRING "" FORCE)

# Disable external compression dependencies
set(BOOST_IOSTREAMS_ENABLE_ZLIB OFF CACHE BOOL "" FORCE)
set(BOOST_IOSTREAMS_ENABLE_BZIP2 OFF CACHE BOOL "" FORCE)
set(BOOST_IOSTREAMS_ENABLE_LZMA OFF CACHE BOOL "" FORCE)
set(BOOST_IOSTREAMS_ENABLE_ZSTD OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        Boost
        URL "https://github.com/boostorg/boost/releases/download/boost-1.90.0/boost-1.90.0-cmake.tar.xz"
)

message(STATUS "Downloading Boost...")
FetchContent_MakeAvailable(Boost)


# ------------------------------------------------------------------------------
# 7. ImGui
# ------------------------------------------------------------------------------
FetchContent_Declare(
        ImGui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.92.5
)

message(STATUS "Downloading ImGui...")
FetchContent_MakeAvailable(ImGui)

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
        GIT_TAG master
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
message(STATUS "Downloading Tracy...")
FetchContent_MakeAvailable(tracy)


# ------------------------------------------------------------------------------
# Targets Definition
# ------------------------------------------------------------------------------

# 1. Headers-only interface
add_library(ThirdPartyHeaders INTERFACE)
target_include_directories(ThirdPartyHeaders INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${SLANG_EXTRACT_DIR}/include>
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
        fmt::fmt
        # Boost Targets
        imgui
        Boost::filesystem
        Boost::log
        Boost::system
)
add_library(ThirdParty::all ALIAS ThirdPartyLibs)