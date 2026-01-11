set(UTIL_SOURCES
        debug.cpp
        ../static_headers/logger.cpp
        maths.cpp
        vk_shaders.cpp
)

add_library(engine_util SHARED ${UTIL_SOURCES})
target_link_libraries(engine_util PUBLIC engine_build_config ThirdParty::all)
target_precompile_headers(engine_util PRIVATE ${PCH_HEADER})
