# Command-line parsing lives in a small library so the tests can exercise it.
add_library(loadicatorCli STATIC commandLine.cpp)
target_include_directories(loadicatorCli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(loadicatorCli PUBLIC Loadicator::core)
loadicatorSetWarnings(loadicatorCli)

add_executable(loadicator main.cpp)
target_link_libraries(loadicator PRIVATE loadicatorCli)
# Lets the program find the ship data from any working directory; override with --data-dir.
target_compile_definitions(loadicator PRIVATE LOADICATOR_DEFAULT_DATA_DIR="${PROJECT_SOURCE_DIR}/data")
loadicatorSetWarnings(loadicator)

install(TARGETS loadicator RUNTIME DESTINATION bin)
