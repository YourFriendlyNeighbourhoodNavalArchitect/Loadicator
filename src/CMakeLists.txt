add_library(loadicatorCore
    loadicator/numeric/interpolation.cpp
    loadicator/numeric/spline.cpp
    loadicator/text/numberParsing.cpp
    loadicator/io/csv.cpp
    loadicator/io/dataLoader.cpp
    loadicator/domain/compartmentTable.cpp
    loadicator/domain/hydrostaticTable.cpp
    loadicator/calc/loadResolution.cpp
    loadicator/calc/massSummary.cpp
    loadicator/calc/equilibrium.cpp
    loadicator/calc/stability.cpp
    loadicator/calc/analysis.cpp
    loadicator/report/reportWriter.cpp
)
add_library(Loadicator::core ALIAS loadicatorCore)

target_include_directories(loadicatorCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(loadicatorCore PUBLIC cxx_std_20)
loadicatorSetWarnings(loadicatorCore)
