add_executable(dataExtractor
    main.cpp
    sourceText.cpp
    compartmentTables.cpp
    hydrostaticTables.cpp
    loadingConditions.cpp
)
target_link_libraries(dataExtractor PRIVATE Loadicator::core)
loadicatorSetWarnings(dataExtractor)
