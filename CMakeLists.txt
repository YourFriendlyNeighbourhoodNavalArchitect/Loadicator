cmake_minimum_required(VERSION 3.20)
project(Loadicator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(loadicatorOptions INTERFACE)
if(MSVC)
    target_compile_options(loadicatorOptions INTERFACE /W4 /permissive- /utf-8)
else()
    target_compile_options(loadicatorOptions INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
        -Wno-missing-field-initializers)
endif()

add_library(loadicatorCore STATIC
    src/calc/analysis.cpp
    src/calc/equilibrium.cpp
    src/calc/loadResolution.cpp
    src/calc/massSummary.cpp
    src/calc/stability.cpp
    src/domain/compartmentTable.cpp
    src/domain/hydrostaticTable.cpp
    src/domain/loadingCondition.cpp
    src/io/csv.cpp
    src/io/dataLoader.cpp
    src/io/reportWriter.cpp
    src/io/textFile.cpp
    src/numeric/interpolation.cpp
    src/numeric/spline.cpp
    src/util/text.cpp)
target_include_directories(loadicatorCore PUBLIC src)
target_link_libraries(loadicatorCore PRIVATE loadicatorOptions)

add_executable(loadicator app/main.cpp app/commandLine.cpp)
target_link_libraries(loadicator PRIVATE loadicatorCore loadicatorOptions)

option(LOADICATOR_BUILD_EXTRACTOR "Build the data extractor" OFF)
if(LOADICATOR_BUILD_EXTRACTOR)
    add_executable(dataExtractor
        tools/dataExtractor/main.cpp
        tools/dataExtractor/compartmentTables.cpp
        tools/dataExtractor/dataWriter.cpp
        tools/dataExtractor/hydrostaticTables.cpp
        tools/dataExtractor/loadingConditions.cpp
        tools/dataExtractor/sourceText.cpp
        tools/dataExtractor/verification.cpp)
    target_link_libraries(dataExtractor PRIVATE loadicatorCore loadicatorOptions)
endif()

add_executable(bookComparison tests/bookComparison.cpp)
target_link_libraries(bookComparison PRIVATE loadicatorCore loadicatorOptions)

enable_testing()
add_test(NAME bookComparison COMMAND bookComparison ${PROJECT_SOURCE_DIR}/data)
