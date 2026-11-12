# Checks the includes of the library and of the programs against the layers that each of them links, as
# recorded by CMakeLists.txt in the rules file. Run from the root of the project:
#
#   cmake -D RULES=<build>/layerRules.cmake -P cmake/checkLayers.cmake
#
#   - every include must name its file in quotes or angle brackets, by a path without '.' or '..';
#   - a header of the library, "loadicator/<layer>/...", must belong to a layer that the including directory
#     may use;
#   - any other quoted include must name a file beside the including file;
#   - every declared directory must hold files, and every file of the library must lie in a declared layer.

cmake_minimum_required(VERSION 3.20)

include("${RULES}")
set(projectRoot "${CMAKE_CURRENT_SOURCE_DIR}")
set(libraryRoot "src/loadicator")
if(NOT IS_DIRECTORY "${libraryRoot}")
    message(FATAL_ERROR "no '${libraryRoot}' in '${projectRoot}': run from the root of the project")
endif()
# A glob reads *, ? and [ in the path of the project as a pattern; a class of one character matches each of
# them literally.
string(REGEX REPLACE "([][*?])" "[\\1]" projectPattern "${projectRoot}")

set(violations "")
set(index 0)
foreach(directory IN LISTS checkedDirectories)
    set(allowed "${allowed_${index}}")
    math(EXPR index "${index} + 1")
    file(GLOB_RECURSE sources LIST_DIRECTORIES false
         RELATIVE "${projectRoot}" "${projectPattern}/${directory}/*")
    if(NOT sources)
        string(APPEND violations "\n  ${directory}: no files to check")
        continue()
    endif()
    foreach(source IN LISTS sources)
        get_filename_component(sourceDirectory "${source}" DIRECTORY)
        file(STRINGS "${source}" includes REGEX "^[ \t]*#[ \t]*include")
        foreach(include IN LISTS includes)
            if(NOT include MATCHES "^[ \t]*#[ \t]*include[ \t]*([<\"])([^>\"]*)[>\"]")
                string(APPEND violations "\n  ${source}: ${include} cannot be checked")
                continue()
            endif()
            set(delimiter "${CMAKE_MATCH_1}")
            set(path "${CMAKE_MATCH_2}")
            if(path MATCHES "(^|/)\\.\\.?(/|$)" OR path MATCHES "\\\\")
                string(APPEND violations "\n  ${source}: ${include} is a relative path")
            elseif(path MATCHES "^loadicator/([^/]*)/")
                if(NOT CMAKE_MATCH_1 IN_LIST allowed)
                    string(APPEND violations "\n  ${source}: ${include}")
                endif()
            elseif(delimiter STREQUAL "\""
                   AND (path MATCHES "/" OR NOT EXISTS "${projectRoot}/${sourceDirectory}/${path}"))
                string(APPEND violations "\n  ${source}: ${include} is not a file beside it")
            endif()
        endforeach()
    endforeach()
endforeach()

file(GLOB entries LIST_DIRECTORIES true RELATIVE "${projectRoot}" "${projectPattern}/${libraryRoot}/*")
foreach(entry IN LISTS entries)
    get_filename_component(name "${entry}" NAME)
    if(NOT IS_DIRECTORY "${projectRoot}/${entry}")
        string(APPEND violations "\n  ${entry}: not inside a layer")
    elseif(NOT name IN_LIST layers)
        string(APPEND violations "\n  ${entry}: not a declared layer")
    endif()
endforeach()

if(violations)
    message(FATAL_ERROR "Layering violations:${violations}")
endif()
list(LENGTH checkedDirectories directoryCount)
message(STATUS "Includes of ${directoryCount} directories checked")
