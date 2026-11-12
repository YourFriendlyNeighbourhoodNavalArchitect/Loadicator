cmake_minimum_required(VERSION 3.24)
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

# The library is built in layers, one target loadicator::<layer> per directory src/<layer>, which links the
# layers it depends on. Each layer has an include directory of its own that reaches only the targets linking
# it, so that the compiler rejects an include of a layer that a target does not link. The headers test
# compiles every header on its own against its layer, which covers the headers that no source compiles.
set(CMAKE_VERIFY_INTERFACE_HEADER_SETS ON)
function(add_loadicator_layer layer)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES;DEPENDS")
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "add_loadicator_layer(${layer}): unexpected '${arg_UNPARSED_ARGUMENTS}'")
    endif()
    string(SUBSTRING ${layer} 0 1 initial)
    string(TOUPPER ${initial} initial)
    string(SUBSTRING ${layer} 1 -1 rest)
    set(target loadicator${initial}${rest})
    if(arg_SOURCES)
        list(TRANSFORM arg_SOURCES PREPEND src/${layer}/)
        add_library(${target} STATIC ${arg_SOURCES})
        target_link_libraries(${target} PRIVATE loadicatorOptions)
        set(scope PUBLIC)
    else()
        add_library(${target} INTERFACE)
        set(scope INTERFACE)
    endif()
    file(GLOB_RECURSE headers CONFIGURE_DEPENDS src/${layer}/include/*.h)
    target_sources(${target} ${scope} FILE_SET HEADERS BASE_DIRS src/${layer}/include FILES ${headers})
    list(TRANSFORM arg_DEPENDS PREPEND loadicator::)
    target_link_libraries(${target} ${scope} ${arg_DEPENDS})
    add_library(loadicator::${layer} ALIAS ${target})
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

add_executable(loadicator app/main.cpp app/commandLine.cpp)
target_link_libraries(loadicator PRIVATE loadicator::io loadicator::calc loadicator::report loadicatorOptions)

# Built only on request, by tools/extractData.sh.
add_executable(dataExtractor EXCLUDE_FROM_ALL
    tools/dataExtractor/main.cpp
    tools/dataExtractor/compartmentTables.cpp
    tools/dataExtractor/dataWriter.cpp
    tools/dataExtractor/hydrostaticTables.cpp
    tools/dataExtractor/loadingConditions.cpp
    tools/dataExtractor/sourceText.cpp
    tools/dataExtractor/verification.cpp)
target_link_libraries(dataExtractor
    PRIVATE loadicator::io loadicator::calc loadicator::book loadicatorOptions)

add_executable(bookComparison tests/bookComparison.cpp)
target_link_libraries(bookComparison
    PRIVATE loadicator::io loadicator::calc loadicator::book loadicatorOptions)

enable_testing()
add_test(NAME bookComparison COMMAND bookComparison ${PROJECT_SOURCE_DIR}/data)
add_test(NAME headers
    COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR} --target all_verify_interface_header_sets)
