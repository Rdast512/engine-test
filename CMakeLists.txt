cmake_minimum_required(VERSION 3.29)
project(engine)


set(CMAKE_CXX_STANDARD 23)
message(STATUS "CMAKE_Version: ${CMAKE_VERSION}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
set(Vulkan_DIR "C:/VulkanSDK/1.4.313.1")
set(ENV{VULKAN_SDK} "C:/VulkanSDK/1.4.313.1")
#list(APPEND CMAKE_PREFIX_PATH "C:/VulkanSDK/1.4.313.1")
find_package(Vulkan REQUIRED)
find_package(vk-bootstrap REQUIRED)
find_package(GlFW3 REQUIRED)
find_package(glm REQUIRED)
find_package(Boost REQUIRED)
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
message(STATUS "Vulkan_INCLUDE_DIR: ${Vulkan_INCLUDE_DIR}")
message(STATUS "Vulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan_LIBRARIES: ${Vulkan_LIBRARIES}")
message(STATUS "GLFW_INCLUDE_DIRS: ${GLFW_INCLUDE_DIRS}")
message(STATUS "GLFW_LIBRARIES: ${GLFW_LIBRARIES}")
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
message(STATUS "SDL3_INCLUDE_DIRS: ${SDL3_INCLUDE_DIRS}")
message(STATUS "SDL3_LIBRARIES: ${SDL3_LIBRARIES}")

message(${CMAKE_SOURCE_DIR})


add_subdirectory(ThirdParty)

add_executable(engine newMain.cpp)


file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

file(GLOB SLANG_SHADERS "${CMAKE_SOURCE_DIR}/shaders/*.slang")

set(COMPILED_SHADERS)
foreach(SLANG ${SLANG_SHADERS})
    get_filename_component(NAME_WE ${SLANG} NAME_WE)
    set(SPV_OUT "${CMAKE_BINARY_DIR}/shaders/${NAME_WE}.spv")

    add_custom_command(
            OUTPUT      ${SPV_OUT}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMAND     ThirdParty/Slang/bin/slangc.exe
            shaders/${NAME_WE}.slang
            -target spirv -o ${SPV_OUT}
            DEPENDS     shaders/${NAME_WE}.slang
            COMMENT     "Compiling shader ${NAME_WE}.slang → ${NAME_WE}.spv"
    )

    list(APPEND COMPILED_SHADERS ${SPV_OUT})
endforeach()

# ceate a target that builds all SPIR-V files
add_custom_target(Shaders ALL
        DEPENDS ${COMPILED_SHADERS}
)



set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${SDL3_INCLUDE_DIRS})
target_link_libraries(engine PUBLIC Boost::boost)
target_link_libraries(engine PUBLIC Vulkan::Vulkan)
target_link_libraries(engine PUBLIC glfw ${GLFW_LIBRARIES})
target_link_libraries(engine PUBLIC glm::glm)
target_link_libraries(engine PUBLIC SDL3::SDL3)
target_link_libraries(engine PUBLIC vk-bootstrap::vk-bootstrap)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb -Wall -Wextra -Wpedantic -fpermissive")
set(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE} -ggdb -O3 -Wall -Wextra -Wpedantic")
add_dependencies(engine Shaders)