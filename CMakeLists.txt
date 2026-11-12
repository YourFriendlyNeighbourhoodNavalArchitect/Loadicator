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

# Fails on arguments that the calling function does not take, so that a misspelt keyword cannot silently drop
# a link or a source.
macro(loadicator_check_arguments function)
    if(DEFINED arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "${function}: unexpected arguments '${arg_UNPARSED_ARGUMENTS}'")
    endif()
    if(DEFINED arg_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "${function}: no value for '${arg_KEYWORDS_MISSING_VALUES}'")
    endif()
endmacro()

# Paths are given within the directory of their target, so that no target can take the sources of another.
function(loadicator_check_relative_paths function)
    foreach(path IN LISTS ARGN)
        if(IS_ABSOLUTE "${path}" OR path MATCHES "(^|/)\\.\\.?(/|$)")
            message(FATAL_ERROR "${function}: '${path}' is not a path within the directory of the target")
        endif()
    endforeach()
endfunction()

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

# Records the layers that the files below a directory may include from. Each target has a directory of its
# own, so that the rules of one target cannot cover the files of another.
function(loadicator_check_includes target directory)
    get_property(checked GLOBAL PROPERTY loadicatorCheckedDirectories)
    foreach(other IN LISTS checked)
        string(FIND "${directory}/" "${other}/" directoryInOther)
        string(FIND "${other}/" "${directory}/" otherInDirectory)
        if(directoryInOther EQUAL 0 OR otherInDirectory EQUAL 0)
            message(FATAL_ERROR
                    "${target}: the directory '${directory}' overlaps '${other}' of another target")
        endif()
    endforeach()
    list(LENGTH checked index)
    set_property(GLOBAL APPEND PROPERTY loadicatorCheckedDirectories "${directory}")
    set_property(GLOBAL PROPERTY loadicatorAllowed_${index} "${ARGN}")
    set_property(GLOBAL APPEND PROPERTY loadicatorTargets ${target})
endfunction()

# A layer without SOURCES consists of headers only. SOURCES are relative to the directory of the layer.
function(add_loadicator_layer layer)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES;DEPENDS")
    loadicator_check_arguments(add_loadicator_layer)
    loadicator_check_relative_paths(add_loadicator_layer ${arg_SOURCES})
    loadicator_reachable_layers(below ${arg_DEPENDS})

    string(SUBSTRING ${layer} 0 1 initial)
    string(TOUPPER ${initial} initial)
    string(SUBSTRING ${layer} 1 -1 rest)
    set(target loadicator${initial}${rest})
    set_property(GLOBAL PROPERTY loadicatorReachable_${layer} ${below})
    set_property(GLOBAL APPEND PROPERTY loadicatorLayers ${layer})
    loadicator_check_includes(${target} src/loadicator/${layer} ${layer} ${below})

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
    loadicator_check_arguments(add_loadicator_executable)
    if(NOT DEFINED arg_DIRECTORY)
        message(FATAL_ERROR "add_loadicator_executable: ${name} needs a DIRECTORY")
    endif()
    loadicator_check_relative_paths(add_loadicator_executable "${arg_DIRECTORY}" ${arg_SOURCES})
    loadicator_reachable_layers(reachable ${arg_LAYERS})
    loadicator_check_includes(${name} "${arg_DIRECTORY}" ${reachable})

    list(TRANSFORM arg_SOURCES PREPEND "${arg_DIRECTORY}/")
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
    SOURCES interpolation.cpp spline.cpp splineSurface.cpp
    DEPENDS util)
add_loadicator_layer(domain
    SOURCES compartmentTable.cpp hydrostaticTable.cpp loadingCondition.cpp shipParticulars.cpp
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

# Once every target is declared, every target but the warning options must have its includes checked, and
# the rules of the layering test are written for it.
function(loadicator_write_layer_rules)
    get_property(targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    get_property(checkedTargets GLOBAL PROPERTY loadicatorTargets)
    foreach(target IN LISTS targets)
        if(NOT target STREQUAL "loadicatorOptions" AND NOT target IN_LIST checkedTargets)
            message(FATAL_ERROR
                    "${target} is not declared by add_loadicator_layer or add_loadicator_executable")
        endif()
    endforeach()

    # Bracket arguments keep every character of the paths, and the semicolons that separate the lists.
    get_property(layers GLOBAL PROPERTY loadicatorLayers)
    get_property(checkedDirectories GLOBAL PROPERTY loadicatorCheckedDirectories)
    set(layerRules "set(layers [==[${layers}]==])\n")
    string(APPEND layerRules "set(checkedDirectories [==[${checkedDirectories}]==])\n")
    list(LENGTH checkedDirectories directoryCount)
    math(EXPR lastIndex "${directoryCount} - 1")
    foreach(index RANGE ${lastIndex})
        get_property(allowed GLOBAL PROPERTY loadicatorAllowed_${index})
        string(APPEND layerRules "set(allowed_${index} [==[${allowed}]==])\n")
    endforeach()
    file(WRITE ${PROJECT_BINARY_DIR}/layerRules.cmake "${layerRules}")
endfunction()
cmake_language(DEFER CALL loadicator_write_layer_rules)

enable_testing()
add_test(NAME bookComparison COMMAND bookComparison ${PROJECT_SOURCE_DIR}/data)
# The check globs paths relative to the project, whose own path may hold characters that a glob reads as
# a pattern.
add_test(NAME layering
    COMMAND ${CMAKE_COMMAND} -D RULES=${PROJECT_BINARY_DIR}/layerRules.cmake
            -P ${PROJECT_SOURCE_DIR}/cmake/checkLayers.cmake
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
