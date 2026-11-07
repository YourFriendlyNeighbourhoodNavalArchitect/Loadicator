add_executable(loadicatorTests
    testing.cpp
    unit/quantityTests.cpp
    unit/numberParsingTests.cpp
    unit/interpolationTests.cpp
    unit/csvTests.cpp
    unit/compartmentTests.cpp
    unit/stabilityTests.cpp
    unit/equilibriumTests.cpp
    unit/applicationTests.cpp
    regressionTests.cpp
)
target_link_libraries(loadicatorTests PRIVATE loadicatorCli)
target_compile_definitions(loadicatorTests PRIVATE LOADICATOR_TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/data")
loadicatorSetWarnings(loadicatorTests)

foreach(suite units text numeric csv compartment stability equilibrium app regression)
    add_test(NAME ${suite} COMMAND loadicatorTests ${suite}/)
endforeach()
