cmake_minimum_required(VERSION 3.20)
project(Loadicator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
        -Wno-missing-field-initializers)
endif()

file(GLOB_RECURSE librarySources CONFIGURE_DEPENDS src/*.cpp)
add_library(loadicatorCore STATIC ${librarySources})
target_include_directories(loadicatorCore PUBLIC src)

add_executable(loadicator app/main.cpp app/report.cpp)
target_link_libraries(loadicator PRIVATE loadicatorCore)

# Built only on request, by tools/extractData.sh.
file(GLOB extractorSources CONFIGURE_DEPENDS tools/dataExtractor/*.cpp)
add_executable(dataExtractor EXCLUDE_FROM_ALL ${extractorSources})
target_link_libraries(dataExtractor PRIVATE loadicatorCore)

add_executable(bookComparison tests/bookComparison.cpp)
target_link_libraries(bookComparison PRIVATE loadicatorCore)

enable_testing()
add_test(NAME bookComparison COMMAND bookComparison ${PROJECT_SOURCE_DIR}/data)
