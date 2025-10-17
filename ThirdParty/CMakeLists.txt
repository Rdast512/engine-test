# YourProject/ThirdParty/CMakeLists.txt

# vcpkg is already bootstrapped and configured via cmake/bootstrap-vcpkg.cmake
# Packages are installed automatically via manifest mode (vcpkg.json in project root)

# --- Slang Dependency Configuration ---
# This is where you specify the tag you want.
# Check the Slang GitHub releases page for available tags.
set(SLANG_TAG "v2025.13.2") # Example version tag

# Extract version number from tag (remove 'v' prefix if present)
string(REGEX REPLACE "^v" "" SLANG_VERSION "${SLANG_TAG}")

# The base URL for Slang releases on GitHub
set(SLANG_RELEASE_URL "https://github.com/shader-slang/slang/releases/download")

# --- OS-Specific Configuration ---
if(WIN32)
    set(SLANG_OS_NAME "win")
    set(SLANG_ARTIFACT_NAME "slang-${SLANG_VERSION}-windows-x86_64.zip")
    set(SLANG_LIB_SUBDIR "bin/windows-x64")
elseif(UNIX AND NOT APPLE)
    set(SLANG_OS_NAME "linux")
    set(SLANG_ARTIFACT_NAME "slang-${SLANG_VERSION}-linux-x86_64.zip")
    set(SLANG_LIB_SUBDIR "bin/linux-x86_64")
else()
    message(FATAL_ERROR "Unsupported OS for pre-compiled Slang binaries.")
endif()

# Define destination folder as Slang-{os}
set(SLANG_DOWNLOAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/slang_download")
set(SLANG_EXTRACT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Slang-${SLANG_OS_NAME}")

# Construct the full download URL
set(SLANG_DOWNLOAD_URL "${SLANG_RELEASE_URL}/${SLANG_TAG}/${SLANG_ARTIFACT_NAME}")

# --- Download and Extract ---
# Only proceed if the binaries haven't been extracted yet.
# --- Download and Extract ---
# Only proceed if the binaries haven't been extracted yet.
if(NOT EXISTS "${SLANG_EXTRACT_DIR}/README.md")
    # Create the download directory if it doesn't exist
    message(STATUS "Downloading Slang binaries from ${SLANG_DOWNLOAD_URL}")

    # Download the archive
    file(DOWNLOAD ${SLANG_DOWNLOAD_URL} "${SLANG_DOWNLOAD_DIR}/${SLANG_ARTIFACT_NAME}" SHOW_PROGRESS STATUS download_status)
    list(GET download_status 0 error_code)
    if(NOT error_code EQUAL 0)
        list(GET download_status 1 error_msg)
        message(FATAL_ERROR "Failed to download Slang: ${error_msg}")
    endif()

    message(STATUS "Extracting Slang binaries to ${SLANG_EXTRACT_DIR}...")
    # Extract the contents
    file(ARCHIVE_EXTRACT
            INPUT "${SLANG_DOWNLOAD_DIR}/${SLANG_ARTIFACT_NAME}"
            DESTINATION "${SLANG_EXTRACT_DIR}"
    )

    # Clean up the download directory after successful extraction
    message(STATUS "Cleaning up temporary download files...")
    file(REMOVE_RECURSE "${SLANG_DOWNLOAD_DIR}")
endif()