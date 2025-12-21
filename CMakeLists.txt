cmake_minimum_required(VERSION 3.20)
project(komodo LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Helpful message when a vcpkg manifest is present but the toolchain isn't used.
# (This is the most common reason `find_package(portaudio CONFIG)` fails.)
if(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg.json" AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "vcpkg manifest detected (vcpkg.json). If PortAudio is not found, configure with the vcpkg toolchain:")
    message(STATUS "  - Use presets: cmake --preset default")
    message(STATUS "  - Or use script: .\\scripts\\build.ps1 -Clean")
    message(STATUS "  - Or pass: -DCMAKE_TOOLCHAIN_FILE=.../vcpkg.cmake")
endif()

find_package(portaudio CONFIG REQUIRED)

# whisper.cpp (prebuilt) -------------------------------------------------------
# We keep whisper.cpp sources under external/ (for headers), but we link against
# prebuilt binaries stored under libs/whisper/.
set(WHISPER_SOURCE_ROOT "${CMAKE_SOURCE_DIR}/external/whisper.cpp")
set(WHISPER_LIB_ROOT "${CMAKE_SOURCE_DIR}/libs/whisper")

if(EXISTS "${WHISPER_SOURCE_ROOT}")
    add_library(whisper SHARED IMPORTED GLOBAL)

    set_target_properties(whisper PROPERTIES
        # whisper.h includes ggml headers as "ggml.h" / "ggml-cpu.h", which live under
        # whisper.cpp/ggml/include. Expose both roots to consumers.
        INTERFACE_INCLUDE_DIRECTORIES "${WHISPER_SOURCE_ROOT}/include;${WHISPER_SOURCE_ROOT}/ggml/include"
        IMPORTED_LOCATION_RELEASE "${WHISPER_LIB_ROOT}/bin/Release/whisper.dll"
        IMPORTED_IMPLIB_RELEASE   "${WHISPER_LIB_ROOT}/lib/Release/whisper.lib"
        IMPORTED_LOCATION_DEBUG   "${WHISPER_LIB_ROOT}/bin/Debug/whisper.dll"
        IMPORTED_IMPLIB_DEBUG     "${WHISPER_LIB_ROOT}/lib/Debug/whisper.lib"
    )

    # Validate only the configuration we're about to build.
    # (For multi-config generators like Visual Studio, the config is chosen at build time.
    #  Our scripts pass KOMODO_WHISPER_CONFIG to tell CMake which prebuilt to validate.)
    set(KOMODO_WHISPER_CONFIG "" CACHE STRING "Whisper prebuilt configuration to validate (Debug/Release).")
    if(NOT KOMODO_WHISPER_CONFIG)
        if(CMAKE_BUILD_TYPE)
            set(KOMODO_WHISPER_CONFIG "${CMAKE_BUILD_TYPE}")
        else()
            set(KOMODO_WHISPER_CONFIG "Release")
        endif()
    endif()

    if(NOT EXISTS "${WHISPER_LIB_ROOT}/lib/${KOMODO_WHISPER_CONFIG}/whisper.lib")
        message(FATAL_ERROR
            "whisper.cpp prebuilt library not found for ${KOMODO_WHISPER_CONFIG}.\n"
            "Expected:\n"
            "  ${WHISPER_LIB_ROOT}/lib/${KOMODO_WHISPER_CONFIG}/whisper.lib\n"
            "  ${WHISPER_LIB_ROOT}/bin/${KOMODO_WHISPER_CONFIG}/whisper.dll\n"
            "\n"
            "Build and copy it into libs/ by running:\n"
            "  .\\scripts\\build_whisper.ps1 -Configuration ${KOMODO_WHISPER_CONFIG}\n"
        )
    endif()
else()
    message(STATUS "external/whisper.cpp not present; building komodo without whisper support.")
endif()

add_executable(komodo
    src/main.cpp
    src/audio.cpp
    src/transcriber.cpp
)

target_include_directories(komodo PRIVATE src)

target_link_libraries(komodo PRIVATE 
    portaudio
)

if(TARGET whisper)
    target_link_libraries(komodo PRIVATE whisper)

    # Runtime DLLs for whisper.cpp on Windows.
    if(WIN32)
        add_custom_command(TARGET komodo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WHISPER_LIB_ROOT}/bin/$<CONFIG>/whisper.dll"
                "$<TARGET_FILE_DIR:komodo>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WHISPER_LIB_ROOT}/bin/$<CONFIG>/ggml.dll"
                "$<TARGET_FILE_DIR:komodo>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WHISPER_LIB_ROOT}/bin/$<CONFIG>/ggml-cpu.dll"
                "$<TARGET_FILE_DIR:komodo>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WHISPER_LIB_ROOT}/bin/$<CONFIG>/ggml-base.dll"
                "$<TARGET_FILE_DIR:komodo>"
        )
    endif()
endif()

if (WIN32)
    target_link_libraries(komodo PRIVATE user32)
endif ()
