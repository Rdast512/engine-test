set(ASSETS_SOURCES
        assets_loader.cpp
        model_storage.cpp
        texture_manager.cpp
)

add_library(engine_assets SHARED ${ASSETS_SOURCES})
target_link_libraries(engine_assets PUBLIC engine_build_config engine_core engine_util ThirdParty::all)
target_precompile_headers(engine_assets PRIVATE ${PCH_HEADER})
