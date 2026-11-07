cmake_minimum_required(VERSION 3.20)

project(Loadicator
    VERSION 2.0.0
    DESCRIPTION "Floating equilibrium and initial stability of a 174000 DWT bulk carrier"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LOADICATOR_BUILD_TESTS "Build the unit and regression tests" ON)
option(LOADICATOR_BUILD_TOOLS "Build the data extractor" ON)
option(LOADICATOR_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(cmake/compilerWarnings.cmake)

add_subdirectory(src)
add_subdirectory(app)

if(LOADICATOR_BUILD_TOOLS)
    add_subdirectory(tools/dataExtractor)
endif()

if(LOADICATOR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
