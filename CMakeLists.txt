## ******************************************************************************
## This source code is licensed under the MIT license found in the
## LICENSE file in the root directory of this source tree.
##
## Copyright (c) 2022 Intel Corporation
## Copyright (c) 2022 Georgia Institute of Technology
## ******************************************************************************

# CMake requirement
cmake_minimum_required(VERSION 3.22)

# Start TACOS project
project(tacos)

# Default build mode: Release
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

# find external packages
find_package(Boost CONFIG REQUIRED COMPONENTS filesystem)

# Compile external libraries
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/yaml-cpp yaml-cpp)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/spdlog spdlog)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/pugixml pugixml)

# Create tacos binary target
add_executable(tacos)

# Set C++ compilation options
set_target_properties(tacos PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    COMPILE_WARNING_AS_ERROR ON)

# Compiled binary/library file locations
set_target_properties(tacos PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

# Path to header files
target_include_directories(tacos PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(tacos PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libs)

# Link to external libraries
target_link_libraries(tacos PRIVATE yaml-cpp)
target_link_libraries(tacos PRIVATE spdlog)
target_link_libraries(tacos PRIVATE pugixml)
target_link_libraries(tacos PRIVATE Boost::filesystem)

# List all source files to be compiled
file(GLOB tacos_srcs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/collective/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/event-queue/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/synthesizer/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/topology/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/writer/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logger/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/writer/*.cpp
)
target_sources(tacos PRIVATE ${tacos_srcs})
target_sources(tacos PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
