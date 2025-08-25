cmake_minimum_required(VERSION 3.29)
project(engine)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_EXPERIMENTAL_CXX_MODULES ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
message(STATUS "CMAKE_Version: ${CMAKE_VERSION}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

if(WIN32)
    set(Vulkan_DIR "C:/VulkanSDK/1.4.313.1")
    set(ENV{VULKAN_SDK} "C:/VulkanSDK/1.4.313.1")
elseif(UNIX AND NOT APPLE)
    # Linux configuration
    #    set(Vulkan_DIR "/usr/include/vulkan")
    #    set(ENV{VULKAN_SDK} "/usr")
    # Alternative paths if Vulkan SDK is installed in different locations
    set(Vulkan_DIR "$ENV{HOME}/VulkanSDK/1.4.321.1/x86_64")
    set(ENV{VULKAN_SDK} "$ENV{HOME}/VulkanSDK/1.4.321.1/x86_64")
    list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/.vcpkg-clion/vcpkg/installed/x64-linux-dynamic")
    list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/.vcpkg-clion/vcpkg/installed/x64-linux-dynamic/share")

    include_directories("$ENV{HOME}/.vcpkg-clion/vcpkg/installed/x64-linux-dynamic/include")

endif()

add_subdirectory(ThirdParty)

set(CMAKE_VERBOSE_MAKEFILE OFF)
find_package(Vulkan REQUIRED)
find_package(vk-bootstrap REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem log system)
find_package(SDL3 REQUIRED CONFIG REQUIRED)
add_library(VulkanCppModule)
add_library(Vulkan::cppm ALIAS VulkanCppModule)
target_compile_definitions(VulkanCppModule PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)

target_include_directories(VulkanCppModule
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
message(STATUS "Vulkan_INCLUDE_DIR: ${Vulkan_INCLUDE_DIR}")
message(STATUS "Vulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan_LIBRARIES: ${Vulkan_LIBRARIES}")
message(STATUS "GLFW_INCLUDE_DIRS: ${GLFW_INCLUDE_DIRS}")
message(STATUS "GLFW_LIBRARIES: ${GLFW_LIBRARIES}")
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
message(STATUS "SDL3_INCLUDE_DIRS: ${SDL3_INCLUDE_DIRS}")
message(STATUS "SDL3_LIBRARIES: ${SDL3_LIBRARIES}")
message(STATUS "vk-bootstrap_INCLUDE_DIRS: ${vk-bootstrap_INCLUDE_DIRS}")
message(STATUS "vk-bootstrap_LIBRARIES: ${vk-bootstrap_LIBRARIES}")
message(STATUS "${CMAKE_DL_LIBS}")
message(STATUS "CMake is using C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
message(${CMAKE_SOURCE_DIR})

# future project structure
#src/engine.cpp
#src/engine.h
#src/renderer.cpp
#src/renderer.h
#src/scene.cpp
#src/scene.h
#src/camera.cpp
#src/camera.h
#src/input.cpp
#src/input.h
#src/shader.cpp
#src/shader.h
#src/model.cpp
#src/model.h
#src/texture.cpp
#src/texture.h
#src/material.cpp
#src/material.h



add_executable(engine
#        newVersion.cpp
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


set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
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
    set(SPV_OUT "${CMAKE_SOURCE_DIR}/build_vscode/main/shaders/${NAME_WE}.spv")

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

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_DL_LIBS} ${Vulkan_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${SDL3_INCLUDE_DIRS})
target_link_libraries(engine PUBLIC
        Boost::filesystem
        Boost::log
        Boost::system
        glfw
        glm::glm
        SDL3::SDL3

)

target_link_libraries(engine PRIVATE
        vk-bootstrap::vk-bootstrap
        vk-bootstrap::vk-bootstrap-compiler-warnings
        ${CMAKE_DL_LIBS}
        Vulkan::cppm
        # vulkan_module
)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} \
-Wall -Wextra -Wpedantic -ggdb -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC \
 -O0 -m64 -march=native -mtune=native -mavx -mavx2 -flto -fno-plt -fdevirtualize -ffast-math -pthread")
set(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall -Wextra -Wpedantic -DNDEBUG -march=native -mtune=native -flto -ffast-math \
    -funroll-loops -finline-functions -fprefetch-loop-arrays -m64 -march=native -mtune=native -mavx -mavx2")
add_dependencies(engine Shaders)