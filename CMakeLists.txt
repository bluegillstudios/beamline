cmake_minimum_required(VERSION 3.10)
project(Beamline VERSION 1.0.1940)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(loader cpu image)

# Source files (adjust paths if needed)
set(SOURCES
    loader/SceneLoader.cpp
    cpu/RayTracer.cpp
    image/ImageSaver.cpp
    beamline.cpp
)

add_executable(beamline ${SOURCES})

# Link libraries if needed (e.g., pthread for multithreading on Linux)
if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(beamline Threads::Threads)
endif()