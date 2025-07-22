cmake_minimum_required(VERSION 3.29)
project(engine)


set(CMAKE_CXX_STANDARD 23)
message(STATUS "CMAKE_Version: ${CMAKE_VERSION}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
if(WIN32)
    set(Vulkan_DIR "C:/VulkanSDK/1.4.313.1")
    set(ENV{VULKAN_SDK} "C:/VulkanSDK/1.4.313.1")
elseif(UNIX AND NOT APPLE)
    # Linux configuration
    #    set(Vulkan_DIR "/usr/include/vulkan")
    #    set(ENV{VULKAN_SDK} "/usr")
    # Alternative paths if Vulkan SDK is installed in different locations
    set(Vulkan_DIR "/home/rdast/VulkanSDK/1.4.321.1/x86_64")
    set(ENV{VULKAN_SDK} "/home/rdast/VulkanSDK/1.4.321.1/x86_64")
    list(APPEND CMAKE_PREFIX_PATH "/home/rdast/.vcpkg-clion/vcpkg/installed/x64-linux-dynamic")
    list(APPEND CMAKE_PREFIX_PATH "/home/rdast/.vcpkg-clion/vcpkg/installed/x64-linux-dynamic/share")

    include_directories("/home/rdast/.vcpkg-clion/vcpkg/installed/x64-linux-dynamic/include")

endif()
#list(APPEND CMAKE_PREFIX_PATH "C:/VulkanSDK/1.4.313.1")

find_package(Vulkan REQUIRED)
find_package(vk-bootstrap REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Boost REQUIRED)
find_package(SDL3 REQUIRED CONFIG REQUIRED)


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

message(${CMAKE_SOURCE_DIR})



add_executable(engine newMain.cpp)


file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

file(GLOB SLANG_SHADERS "${CMAKE_SOURCE_DIR}/shaders/*.slang")

# Set the appropriate slangc executable based on platform
if(WIN32)
    set(SLANGC_EXECUTABLE "ThirdParty/Slang/bin/slangc.exe")
else()
    set(SLANGC_EXECUTABLE "ThirdParty/Slang-linux/bin/slangc")
endif()

set(COMPILED_SHADERS)
foreach(SLANG ${SLANG_SHADERS})
    get_filename_component(NAME_WE ${SLANG} NAME_WE)
    set(SPV_OUT "${CMAKE_SOURCE_DIR}/cmake-build-debug/shaders/${NAME_WE}.spv")

    add_custom_command(
            OUTPUT      ${SPV_OUT}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMAND     ${SLANGC_EXECUTABLE}
            shaders/${NAME_WE}.slang
            -target spirv -o ${SPV_OUT}
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
        Boost::boost
        Vulkan::Vulkan
        glfw
        glm::glm
        SDL3::SDL3
)
target_link_libraries(engine PRIVATE
        vk-bootstrap::vk-bootstrap
        vk-bootstrap::vk-bootstrap-compiler-warnings
        ${CMAKE_DL_LIBS}
)
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb -Wall -Wextra -Wpedantic -fpermissive")
set(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE} -ggdb -O3 -Wall -Wextra -Wpedantic")
add_dependencies(engine Shaders)