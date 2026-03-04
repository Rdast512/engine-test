set(CORE_SOURCES
    vk_allocator.cpp
    vk_descriptors.cpp
    vk_device.cpp
    vk_resource_manager.cpp
    vk_swapchain.cpp
    ${CMAKE_SOURCE_DIR}/ThirdParty/implementations.cpp
)

add_library(engine_core SHARED ${CORE_SOURCES})
target_link_libraries(engine_core PUBLIC engine_build_config engine_util ThirdParty::all)
target_precompile_headers(engine_core PRIVATE ${PCH_HEADER})

# implementations.cpp defines *_IMPLEMENTATION macros and re-includes the single-header
# libs from scratch — it must not receive the PCH since the PCH already has the guards.
#set_source_files_properties(
#    ${CMAKE_SOURCE_DIR}/ThirdParty/implementations.cpp
#    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
#)