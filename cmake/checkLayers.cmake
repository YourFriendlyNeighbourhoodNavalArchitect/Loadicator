# Checks that the sources of each layer of the library include headers only of their own layer and of the
# layers that it depends on, and that every file of the library lies in a declared layer.
#
#   cmake -D SOURCE_ROOT=<src/loadicator> -D LAYER_RULES=<layer>=<layer>,<layer>|... -P checkLayers.cmake

cmake_minimum_required(VERSION 3.20)

string(REPLACE "|" ";" rules "${LAYER_RULES}")
set(layers "")
set(violations "")
foreach(rule IN LISTS rules)
    string(FIND "${rule}" "=" separator)
    string(SUBSTRING "${rule}" 0 ${separator} layer)
    math(EXPR reachableStart "${separator} + 1")
    string(SUBSTRING "${rule}" ${reachableStart} -1 reachableText)
    string(REPLACE "," ";" allowed "${reachableText}")
    list(APPEND allowed ${layer})
    list(APPEND layers ${layer})

    file(GLOB sources "${SOURCE_ROOT}/${layer}/*")
    foreach(source IN LISTS sources)
        file(STRINGS "${source}" includes REGEX "^[ \t]*#[ \t]*include[ \t]*\"loadicator/")
        foreach(include IN LISTS includes)
            string(REGEX MATCH "\"loadicator/([^/\"]+)/" ignored "${include}")
            if(NOT CMAKE_MATCH_1 IN_LIST allowed)
                string(APPEND violations "\n  ${source}: ${include}")
            endif()
        endforeach()
    endforeach()
endforeach()

file(GLOB directories LIST_DIRECTORIES true "${SOURCE_ROOT}/*")
foreach(directory IN LISTS directories)
    get_filename_component(name "${directory}" NAME)
    if(NOT IS_DIRECTORY "${directory}")
        string(APPEND violations "\n  ${directory}: not inside a layer")
    elseif(NOT name IN_LIST layers)
        string(APPEND violations "\n  ${directory}: not a declared layer")
    endif()
endforeach()

if(violations)
    message(FATAL_ERROR "Layering violations:${violations}")
endif()
list(LENGTH layers layerCount)
message(STATUS "${layerCount} layers checked")
