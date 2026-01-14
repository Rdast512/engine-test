# Root CMake for engine source modules

set(PCH_HEADER ${CMAKE_SOURCE_DIR}/src/pch.h)

add_library(engine_build_config INTERFACE)
target_include_directories(engine_build_config INTERFACE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(engine_build_config INTERFACE ${CMAKE_SOURCE_DIR}/src/static_headers)
target_compile_definitions(engine_build_config INTERFACE
    ENGINE_SHADER_DIR="${ENGINE_SHADER_DIR_PATH}"
    ENGINE_MODELS_DIR="${ENGINE_MODELS_DIR_PATH}"
    ENGINE_TEXTURES_DIR="${ENGINE_TEXTURES_DIR_PATH}"
)

add_subdirectory(util)
add_subdirectory(core)
add_subdirectory(assets)
add_subdirectory(render)
add_subdirectory(scene)
add_subdirectory(postprocessing)

add_library(engine_runtime SHARED core/vk_engine.cpp core/vk_engine.hpp)
target_link_libraries(engine_runtime
    PUBLIC
        engine_build_config
        engine_core
        engine_assets
        engine_render
        engine_util
        ThirdParty::all
)

target_link_libraries(engine_runtime PRIVATE
        ${CMAKE_DL_LIBS}
)

target_precompile_headers(engine_runtime PRIVATE ${PCH_HEADER})

add_executable(engine main.cpp pch.h Constants.h)
target_link_libraries(engine PRIVATE engine_runtime)
target_precompile_headers(engine PRIVATE ${PCH_HEADER})
add_dependencies(engine Shaders)

if(WIN32)
    # Copy third-party DLLs needed at runtime next to the executable
    set(SHARED_LIBS_TO_COPY
        SDL3::SDL3
        fmt::fmt
        spdlog::spdlog
        Boost::filesystem
        Boost::log
        Boost::log_setup
        Boost::thread
        Boost::atomic
        TracyProfiler
    )

    foreach(_LIB ${SHARED_LIBS_TO_COPY})
        if(TARGET ${_LIB})
            get_target_property(_type ${_LIB} TYPE)
            if("${_type}" STREQUAL "SHARED_LIBRARY" OR "${_type}" STREQUAL "MODULE_LIBRARY")
                add_custom_command(TARGET engine POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        $<TARGET_FILE:${_LIB}>
                        $<TARGET_FILE_DIR:engine>
                    COMMENT "Copying DLL for ${_LIB}...")
            endif()
        endif()
    endforeach()

    # Copy module DLLs next to the executable
    add_custom_command(TARGET engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:engine_runtime>
            $<TARGET_FILE_DIR:engine>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:engine_core>
            $<TARGET_FILE_DIR:engine>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:engine_assets>
            $<TARGET_FILE_DIR:engine>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:engine_render>
            $<TARGET_FILE_DIR:engine>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:engine_util>
            $<TARGET_FILE_DIR:engine>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:engine_scene>
            $<TARGET_FILE_DIR:engine>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:engine_postprocessing>
            $<TARGET_FILE_DIR:engine>
        COMMENT "Copying engine module DLLs")
else()
    # On Linux copy third-party shared libs next to the executable
    set(SHARED_LIBS_TO_COPY
        SDL3::SDL3
        fmt::fmt
        spdlog::spdlog
        glm::glm
        Boost::filesystem
        Boost::log
        Boost::log_setup
        Boost::system
        Boost::thread
        Boost::atomic
        Boost::chrono
        Boost::container
        Boost::date_time
        TracyProfiler
    )

    foreach(_LIB ${SHARED_LIBS_TO_COPY})
        if(TARGET ${_LIB})
            get_target_property(_type ${_LIB} TYPE)
            if("${_type}" STREQUAL "SHARED_LIBRARY" OR "${_type}" STREQUAL "MODULE_LIBRARY")
                add_custom_command(TARGET engine POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        $<TARGET_FILE:${_LIB}>
                        $<TARGET_FILE_DIR:engine>
                    COMMAND ${CMAKE_COMMAND} -E create_symlink
                        $<TARGET_FILE:${_LIB}>
                        $<TARGET_FILE_DIR:engine>/$<TARGET_SONAME_FILE_NAME:${_LIB}>
                    COMMAND ${CMAKE_COMMAND} -E create_symlink
                        $<TARGET_FILE:${_LIB}>
                        $<TARGET_FILE_DIR:engine>/$<TARGET_LINKER_FILE_NAME:${_LIB}>
                    COMMENT "Copying shared lib (and SONAME links) for ${_LIB}...")
            endif()
        endif()
    endforeach()

    set_target_properties(engine PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN")

    # Ensure the runtime module resolves siblings via $ORIGIN
    set_target_properties(engine_runtime PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN")
endif()
