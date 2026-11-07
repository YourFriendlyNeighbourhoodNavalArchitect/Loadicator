#include "../testing.h"
#include "loadicator/calc/loadResolution.h"
#include "loadicator/domain/compartmentTable.h"

using namespace loadicator;
using namespace loadicator::literals;

namespace {

CompartmentTable sampleTank() {
    const std::vector<CompartmentTable::Row> rows{
        {Volume{0.0}, Position{10.0_m, 1.0_m, 0.0_m}, SecondMomentOfArea{100.0}},
        {Volume{50.0}, Position{11.0_m, 1.0_m, 1.0_m}, SecondMomentOfArea{200.0}},
        {Volume{98.0}, Position{12.0_m, 1.0_m, 2.0_m}, SecondMomentOfArea{0.0}},
    };
    return CompartmentTable(CompartmentId{"T1"}, "test tank", Volume{100.0}, rows);
}

CompartmentLibrary sampleLibrary() {
    CompartmentLibrary library;
    library.add(sampleTank());
    return library;
}

} // namespace

TEST_CASE("compartment/interpolatesByVolume") {
    const auto state = sampleTank().stateAt(Volume{25.0});
    CHECK_NEAR(state.centroid.x.value(), 10.5, 1e-12);
    CHECK_NEAR(state.centroid.z.value(), 0.5, 1e-12);
    CHECK_NEAR(state.freeSurfaceInertia.value(), 150.0, 1e-12);
}

TEST_CASE("compartment/rangeChecks") {
    const CompartmentTable tank = sampleTank();
    CHECK_NEAR(tank.stateAt(Volume{100.0}).centroid.x.value(), 12.0, 1e-12); // capacity beyond last row
    CHECK_THROWS(tank.stateAt(Volume{-1.0}));
    CHECK_THROWS(tank.stateAt(Volume{101.0}));
    CHECK_THROWS(CompartmentTable(CompartmentId{"T2"}, "", Volume{10.0},
                                  {{Volume{0.0}, {}, {}}, {Volume{20.0}, {}, {}}})); // capacity < volume
    CHECK_THROWS(CompartmentId{""});
    CompartmentLibrary library = sampleLibrary();
    CHECK_THROWS(library.add(sampleTank()));
    CHECK_THROWS(library.at(CompartmentId{"missing"}));
}

TEST_CASE("compartment/resolvesMassAndFill") {
    const CompartmentLibrary library = sampleLibrary();
    LoadingCondition condition{.name = "test"};
    condition.tankLoads.push_back(
        {CompartmentId{"T1"}, TankAmount{Mass{51.25}}, 1.025_tPerM3, CargoPhase::liquid});
    condition.tankLoads.push_back(
        {CompartmentId{"T1"}, TankAmount{ratioFromPercent(25.0)}, 2.0_tPerM3, CargoPhase::solid});
    condition.fixedWeights.push_back(
        {"Lightweight", Mass{1000.0}, Position{50.0_m, 0.0_m, 5.0_m}, Moment{0.0}});

    const auto items = resolveLoadingCondition(condition, library);
    CHECK(items.size() == 3);
    CHECK_NEAR(items[0].mass.value(), 51.25, 1e-9);              // 50 m3 at 1.025
    CHECK_NEAR(items[0].freeSurfaceMoment.value(), 205.0, 1e-9); // ρ · i = 1.025 · 200
    CHECK_NEAR(items[1].mass.value(), 50.0, 1e-9);               // 25 % of 100 m3 at 2.0
    CHECK_NEAR(items[1].freeSurfaceMoment.value(), 0.0, 0.0);    // solids have no free surface
    CHECK(items[2].name == "Lightweight");
}

TEST_CASE("compartment/rejectsInvalidLoads") {
    const CompartmentLibrary library = sampleLibrary();
    LoadingCondition unknown{.name = "unknown"};
    unknown.tankLoads.push_back(
        {CompartmentId{"R9.9"}, TankAmount{Mass{1.0}}, 1.0_tPerM3, CargoPhase::liquid});
    // Audit C5: the original silently fell back to book values for unknown tanks.
    CHECK_THROWS(resolveLoadingCondition(unknown, library));

    LoadingCondition noDensity{.name = "density"};
    noDensity.tankLoads.push_back(
        {CompartmentId{"T1"}, TankAmount{Mass{1.0}}, Density{0.0}, CargoPhase::liquid});
    // Audit C10: a missing density silently became 0 t/m3.
    CHECK_THROWS(resolveLoadingCondition(noDensity, library));
}
