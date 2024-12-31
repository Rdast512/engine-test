cmake_minimum_required(VERSION 3.29)
project(engine)


set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
find_package(GlFW3 REQUIRED)
find_package(glm REQUIRED)
find_package(Boost REQUIRED)
message(STATUS "Vulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}")
# find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)


add_executable(engine main.cpp)
# message(STATUS "SDL2_INCLUDE_DIRS: ${SDL2_INCLUDE_DIRS}")

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
# target_link_libraries(engine PRIVATE SDL2::SDL2)
target_link_libraries(engine PUBLIC Boost::boost)
target_link_libraries(engine PUBLIC Vulkan::Vulkan)
target_link_libraries(engine PUBLIC glfw ${GLFW_LIBRARIES})
target_link_libraries(engine PUBLIC glm::glm)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE} -g -O3 -Wall -Wextra -Wpedantic")