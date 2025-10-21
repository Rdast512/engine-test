# NOTE: This file is now integrated directly into the main CMakeLists.txt
# It is kept here for reference only.
# 
# The vcpkg bootstrap logic has been moved to CMakeLists.txt (before project())
# to ensure it runs automatically on first configuration.

# Original bootstrap logic (now in CMakeLists.txt):
set(VCPKG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/vcpkg")

if(NOT EXISTS "${VCPKG_ROOT}/vcpkg${CMAKE_EXECUTABLE_SUFFIX}")
    message(STATUS "vcpkg not found. Downloading and bootstrapping vcpkg...")
    
    # Clone vcpkg repository
    find_package(Git QUIET)
    if(NOT GIT_FOUND)
        message(FATAL_ERROR "Git is required to download vcpkg")
    endif()
    
    execute_process(
        COMMAND ${GIT_EXECUTABLE} clone https://github.com/Microsoft/vcpkg.git "${VCPKG_ROOT}"
        RESULT_VARIABLE git_result
        OUTPUT_QUIET
        ERROR_QUIET
    )
    
    if(NOT git_result EQUAL 0)
        message(FATAL_ERROR "Failed to clone vcpkg repository")
    endif()
    
    # Bootstrap vcpkg
    if(WIN32)
        set(BOOTSTRAP_SCRIPT "${VCPKG_ROOT}/bootstrap-vcpkg.bat")
    else()
        set(BOOTSTRAP_SCRIPT "${VCPKG_ROOT}/bootstrap-vcpkg.sh")
    endif()
    
    message(STATUS "Bootstrapping vcpkg (this may take a few minutes)...")
    execute_process(
        COMMAND ${BOOTSTRAP_SCRIPT}
        WORKING_DIRECTORY "${VCPKG_ROOT}"
        RESULT_VARIABLE bootstrap_result
    )
    
    if(NOT bootstrap_result EQUAL 0)
        message(FATAL_ERROR "Failed to bootstrap vcpkg")
    endif()
    
    message(STATUS "vcpkg successfully bootstrapped at ${VCPKG_ROOT}")
endif()

# Set the toolchain file
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" 
        CACHE STRING "vcpkg toolchain file")
    message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()
