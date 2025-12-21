cmake_minimum_required(VERSION 3.20)
project(komodo LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(portaudio CONFIG REQUIRED)

add_executable(komodo
    src/main.cpp
    src/audio.cpp
    src/transcriber.cpp
)

target_include_directories(komodo PRIVATE src)

target_link_libraries(komodo PRIVATE portaudio)
