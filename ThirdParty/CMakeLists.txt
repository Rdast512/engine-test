# ThirdParty/CMakeLists.txt
include(FetchContent)

# Optimization: Disable C++ Module scanning for third-party libs
# They don't use modules yet, so scanning them is wasted time.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
# ------------------------------------------------------------------------------
# 1. Slang (Manual Download & Extract)
# ------------------------------------------------------------------------------
set(SLANG_TAG "v2025.24")
string(REGEX REPLACE "^v" "" SLANG_VERSION "${SLANG_TAG}")
set(SLANG_RELEASE_URL "https://github.com/shader-slang/slang/releases/download")

if(WIN32)
    set(SLANG_OS_NAME "win")
    set(SLANG_ARTIFACT_NAME "slang-${SLANG_VERSION}-windows-x86_64.zip")
elseif(UNIX AND NOT APPLE)
    set(SLANG_OS_NAME "linux")
    set(SLANG_ARTIFACT_NAME "slang-${SLANG_VERSION}-linux-x86_64.zip")
else()
    message(FATAL_ERROR "Unsupported OS for Slang.")
endif()

set(SLANG_DOWNLOAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/slang_download")
set(SLANG_EXTRACT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Slang-${SLANG_OS_NAME}")
set(SLANG_DOWNLOAD_URL "${SLANG_RELEASE_URL}/${SLANG_TAG}/${SLANG_ARTIFACT_NAME}")

if(NOT EXISTS "${SLANG_EXTRACT_DIR}/README.md")
    message(STATUS "Downloading Slang from ${SLANG_DOWNLOAD_URL}")
    file(DOWNLOAD ${SLANG_DOWNLOAD_URL} "${SLANG_DOWNLOAD_DIR}/${SLANG_ARTIFACT_NAME}" SHOW_PROGRESS STATUS download_status)
    list(GET download_status 0 error_code)
    
    if(NOT error_code EQUAL 0)
        list(GET download_status 1 error_msg)
        message(FATAL_ERROR "Failed to download Slang: ${error_msg}")
    endif()

    message(STATUS "Extracting Slang to ${SLANG_EXTRACT_DIR}...")
    file(ARCHIVE_EXTRACT INPUT "${SLANG_DOWNLOAD_DIR}/${SLANG_ARTIFACT_NAME}" DESTINATION "${SLANG_EXTRACT_DIR}")
    file(REMOVE_RECURSE "${SLANG_DOWNLOAD_DIR}")
endif()

# ------------------------------------------------------------------------------
# 2. GLM (Header Only)
# ------------------------------------------------------------------------------
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.2
)
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
if(WIN32)
elseif(UNIX AND NOT APPLE)
    set(SDL_WAYLAND ON CACHE BOOL "" FORCE)
    set(SDL_X11 ON CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.26
)
FetchContent_MakeAvailable(SDL3)

# ------------------------------------------------------------------------------
# 4. vk-bootstrap
# ------------------------------------------------------------------------------
set(VK_BOOTSTRAP_TEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    vk-bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
    GIT_TAG        v1.4.332
)
FetchContent_MakeAvailable(vk-bootstrap)

# ------------------------------------------------------------------------------
# 5. spdlog
# ------------------------------------------------------------------------------
# FetchContent_Declare(
#     spdlog
#     GIT_REPOSITORY https://github.com/gabime/spdlog.git
#     GIT_TAG        v1.12.0
# )
# FetchContent_MakeAvailable(spdlog)

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
    URL "https://github.com/boostorg/boost/releases/download/boost-1.89.0/boost-1.89.0-cmake.tar.xz"
)

FetchContent_MakeAvailable(Boost)

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
    glm::glm
    SDL3::SDL3
    vk-bootstrap::vk-bootstrap
    # spdlog::spdlog
    # Boost Targets
    Boost::filesystem
    Boost::log
    Boost::system
)
add_library(ThirdParty::all ALIAS ThirdPartyLibs)