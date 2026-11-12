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

# The library is built in layers, one target per directory of src/loadicator. A layer links only the
# layers named in its DEPENDS, and through them the layers below; the layering test checks that its
# sources include headers of no other layer. A layer without SOURCES consists of headers only.
set(loadicatorLayerRules "")
function(add_loadicator_layer layer)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES;DEPENDS")
    string(SUBSTRING ${layer} 0 1 initial)
    string(TOUPPER ${initial} initial)
    string(SUBSTRING ${layer} 1 -1 rest)
    set(target loadicator${initial}${rest})

    set(reachable "")
    foreach(dependency IN LISTS arg_DEPENDS)
        list(APPEND reachable ${dependency} ${loadicatorReachable_${dependency}})
    endforeach()
    list(REMOVE_DUPLICATES reachable)
    set(loadicatorReachable_${layer} "${reachable}" PARENT_SCOPE)
    set(loadicatorTarget_${layer} ${target} PARENT_SCOPE)
    list(JOIN reachable "," reachableText)
    list(APPEND loadicatorLayerRules "${layer}=${reachableText}")
    set(loadicatorLayerRules "${loadicatorLayerRules}" PARENT_SCOPE)

    set(dependencyTargets "")
    foreach(dependency IN LISTS arg_DEPENDS)
        list(APPEND dependencyTargets ${loadicatorTarget_${dependency}})
    endforeach()
    if(arg_SOURCES)
        add_library(${target} STATIC ${arg_SOURCES})
        target_include_directories(${target} PUBLIC src)
        target_link_libraries(${target} PUBLIC ${dependencyTargets} PRIVATE loadicatorOptions)
    else()
        add_library(${target} INTERFACE)
        target_include_directories(${target} INTERFACE src)
        target_link_libraries(${target} INTERFACE ${dependencyTargets})
    endif()
endfunction()

add_loadicator_layer(util
    SOURCES src/loadicator/util/text.cpp)
add_loadicator_layer(numeric
    SOURCES src/loadicator/numeric/interpolation.cpp
            src/loadicator/numeric/spline.cpp
    DEPENDS util)
add_loadicator_layer(domain
    SOURCES src/loadicator/domain/compartmentTable.cpp
            src/loadicator/domain/hydrostaticTable.cpp
            src/loadicator/domain/loadingCondition.cpp
    DEPENDS numeric)
add_loadicator_layer(calc
    SOURCES src/loadicator/calc/analysis.cpp
            src/loadicator/calc/equilibrium.cpp
            src/loadicator/calc/loadResolution.cpp
            src/loadicator/calc/massSummary.cpp
            src/loadicator/calc/stability.cpp
    DEPENDS domain)
add_loadicator_layer(io
    SOURCES src/loadicator/io/csv.cpp
            src/loadicator/io/dataLoader.cpp
            src/loadicator/io/textFile.cpp
    DEPENDS domain)
add_loadicator_layer(report
    SOURCES src/loadicator/report/reportWriter.cpp
    DEPENDS calc)
# The results printed in the trim and stability book, for the data extractor and the comparison with the book.
add_loadicator_layer(book)

add_executable(loadicator app/main.cpp app/commandLine.cpp)
target_link_libraries(loadicator PRIVATE loadicatorIo loadicatorCalc loadicatorReport loadicatorOptions)

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
    target_link_libraries(dataExtractor PRIVATE loadicatorIo loadicatorCalc loadicatorBook loadicatorOptions)
endif()

add_executable(bookComparison tests/bookComparison.cpp)
target_link_libraries(bookComparison PRIVATE loadicatorIo loadicatorCalc loadicatorBook loadicatorOptions)

enable_testing()
add_test(NAME bookComparison COMMAND bookComparison ${PROJECT_SOURCE_DIR}/data)
list(JOIN loadicatorLayerRules "|" layerRules)
add_test(NAME layering
    COMMAND ${CMAKE_COMMAND} -D SOURCE_ROOT=${PROJECT_SOURCE_DIR}/src/loadicator -D "LAYER_RULES=${layerRules}"
            -P ${PROJECT_SOURCE_DIR}/cmake/checkLayers.cmake)
