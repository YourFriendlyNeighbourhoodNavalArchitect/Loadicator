# Checks the includes of the library and of the programs against the layers that each of them links, as
# recorded by CMakeLists.txt in the rules file:
#
#   - a header of the library, included as "loadicator/<layer>/..." or <loadicator/<layer>/...>, must belong
#     to a layer that the including directory may use;
#   - any other quoted include must name a file beside the including file, so that no relative path can
#     reach into a layer;
#   - every file of the library must lie in the directory of a declared layer.
#
#   cmake -D RULES=<build>/layerRules.cmake -P checkLayers.cmake

cmake_minimum_required(VERSION 3.20)

include("${RULES}")
set(libraryRoot "${projectRoot}/src/loadicator")
if(NOT IS_DIRECTORY "${libraryRoot}")
    message(FATAL_ERROR "the library directory '${libraryRoot}' does not exist")
endif()

set(violations "")
foreach(directory IN LISTS checkedDirectories)
    string(MAKE_C_IDENTIFIER "${directory}" key)
    if(NOT IS_DIRECTORY "${projectRoot}/${directory}")
        string(APPEND violations "\n  ${directory}: the directory does not exist")
        continue()
    endif()
    file(GLOB_RECURSE sources LIST_DIRECTORIES false "${projectRoot}/${directory}/*")
    foreach(source IN LISTS sources)
        get_filename_component(sourceDirectory "${source}" DIRECTORY)
        file(STRINGS "${source}" includes REGEX "^[ \t]*#[ \t]*include")
        foreach(include IN LISTS includes)
            if(include MATCHES "^[ \t]*#[ \t]*include[ \t]*[<\"]loadicator/([^/>\"]*)/")
                if(NOT CMAKE_MATCH_1 IN_LIST allowed_${key})
                    string(APPEND violations "\n  ${source}: ${include}")
                endif()
            elseif(include MATCHES "^[ \t]*#[ \t]*include[ \t]*\"([^\"]*)\"")
                if(CMAKE_MATCH_1 MATCHES "[/\\\\]" OR NOT EXISTS "${sourceDirectory}/${CMAKE_MATCH_1}")
                    string(APPEND violations "\n  ${source}: ${include} is not a file beside it")
                endif()
            endif()
        endforeach()
    endforeach()
endforeach()

file(GLOB entries LIST_DIRECTORIES true "${libraryRoot}/*")
foreach(entry IN LISTS entries)
    get_filename_component(name "${entry}" NAME)
    if(NOT IS_DIRECTORY "${entry}")
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
