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

# The library is built in layers, one target loadicator::<layer> per directory src/loadicator/<layer>. A layer
# links the layers named in its DEPENDS, and through them the layers below. Every layer exports the same
# include directory, so the build alone cannot stop a source from including a header of a layer that it does
# not link: the layering test checks the includes of each layer and of each program against what it links.

# The layers that a layer or a program may include from: those it links and all the layers below them.
function(loadicator_reachable_layers output)
    set(reachable "")
    get_property(declared GLOBAL PROPERTY loadicatorLayers)
    foreach(layer IN LISTS ARGN)
        if(NOT layer IN_LIST declared)
            message(FATAL_ERROR "'${layer}' is not a layer declared before this point")
        endif()
        get_property(below GLOBAL PROPERTY loadicatorReachable_${layer})
        list(APPEND reachable ${layer} ${below})
    endforeach()
    list(REMOVE_DUPLICATES reachable)
    set(${output} "${reachable}" PARENT_SCOPE)
endfunction()

function(loadicator_check_includes directory)
    set_property(GLOBAL APPEND PROPERTY loadicatorCheckedDirectories ${directory})
    string(MAKE_C_IDENTIFIER "${directory}" key)
    set_property(GLOBAL PROPERTY loadicatorAllowed_${key} ${ARGN})
endfunction()

# A layer without SOURCES consists of headers only. SOURCES are relative to the directory of the layer.
function(add_loadicator_layer layer)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES;DEPENDS")
    loadicator_reachable_layers(below ${arg_DEPENDS})
    set_property(GLOBAL PROPERTY loadicatorReachable_${layer} ${below})
    set_property(GLOBAL APPEND PROPERTY loadicatorLayers ${layer})
    loadicator_check_includes(src/loadicator/${layer} ${layer} ${below})

    string(SUBSTRING ${layer} 0 1 initial)
    string(TOUPPER ${initial} initial)
    string(SUBSTRING ${layer} 1 -1 rest)
    set(target loadicator${initial}${rest})
    list(TRANSFORM arg_DEPENDS PREPEND loadicator::)
    if(arg_SOURCES)
        list(TRANSFORM arg_SOURCES PREPEND src/loadicator/${layer}/)
        add_library(${target} STATIC ${arg_SOURCES})
        target_include_directories(${target} PUBLIC src)
        target_link_libraries(${target} PUBLIC ${arg_DEPENDS} PRIVATE loadicatorOptions)
    else()
        add_library(${target} INTERFACE)
        target_include_directories(${target} INTERFACE src)
        target_link_libraries(${target} INTERFACE ${arg_DEPENDS})
    endif()
    add_library(loadicator::${layer} ALIAS ${target})
endfunction()

# A program built from the sources of one directory, relative to which SOURCES are given, and linking LAYERS.
function(add_loadicator_executable name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "EXCLUDE_FROM_ALL" "DIRECTORY" "SOURCES;LAYERS")
    loadicator_reachable_layers(reachable ${arg_LAYERS})
    loadicator_check_includes(${arg_DIRECTORY} ${reachable})

    list(TRANSFORM arg_SOURCES PREPEND ${arg_DIRECTORY}/)
    list(TRANSFORM arg_LAYERS PREPEND loadicator::)
    if(arg_EXCLUDE_FROM_ALL)
        add_executable(${name} EXCLUDE_FROM_ALL ${arg_SOURCES})
    else()
        add_executable(${name} ${arg_SOURCES})
    endif()
    target_link_libraries(${name} PRIVATE ${arg_LAYERS} loadicatorOptions)
endfunction()

add_loadicator_layer(util
    SOURCES text.cpp)
add_loadicator_layer(numeric
    SOURCES interpolation.cpp spline.cpp
    DEPENDS util)
add_loadicator_layer(domain
    SOURCES compartmentTable.cpp hydrostaticTable.cpp loadingCondition.cpp
    DEPENDS numeric)
add_loadicator_layer(calc
    SOURCES analysis.cpp equilibrium.cpp loadResolution.cpp massSummary.cpp stability.cpp
    DEPENDS domain)
add_loadicator_layer(io
    SOURCES csv.cpp dataLoader.cpp textFile.cpp
    DEPENDS domain)
add_loadicator_layer(report
    SOURCES reportWriter.cpp
    DEPENDS calc)
# The results printed in the trim and stability book, for the data extractor and the comparison with the book.
add_loadicator_layer(book)

add_loadicator_executable(loadicator
    DIRECTORY app
    SOURCES main.cpp commandLine.cpp
    LAYERS io calc report)

# Built only on request, by tools/extractData.sh.
add_loadicator_executable(dataExtractor EXCLUDE_FROM_ALL
    DIRECTORY tools/dataExtractor
    SOURCES main.cpp compartmentTables.cpp dataWriter.cpp hydrostaticTables.cpp loadingConditions.cpp
            sourceText.cpp verification.cpp
    LAYERS io calc book)

add_loadicator_executable(bookComparison
    DIRECTORY tests
    SOURCES bookComparison.cpp
    LAYERS io calc book)

get_property(layers GLOBAL PROPERTY loadicatorLayers)
get_property(checkedDirectories GLOBAL PROPERTY loadicatorCheckedDirectories)
set(layerRules "set(projectRoot \"${PROJECT_SOURCE_DIR}\")\n")
string(APPEND layerRules "set(layers ${layers})\n")
string(APPEND layerRules "set(checkedDirectories ${checkedDirectories})\n")
foreach(directory IN LISTS checkedDirectories)
    string(MAKE_C_IDENTIFIER "${directory}" key)
    get_property(allowed GLOBAL PROPERTY loadicatorAllowed_${key})
    string(APPEND layerRules "set(allowed_${key} ${allowed})\n")
endforeach()
file(WRITE ${PROJECT_BINARY_DIR}/layerRules.cmake "${layerRules}")

enable_testing()
add_test(NAME bookComparison COMMAND bookComparison ${PROJECT_SOURCE_DIR}/data)
add_test(NAME layering
    COMMAND ${CMAKE_COMMAND} -D RULES=${PROJECT_BINARY_DIR}/layerRules.cmake
            -P ${PROJECT_SOURCE_DIR}/cmake/checkLayers.cmake)
