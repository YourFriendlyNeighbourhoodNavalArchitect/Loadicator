#include "loadicator/report/reportWriter.h"

#include <format>
#include <string>
#include <string_view>

namespace loadicator {

namespace {

constexpr std::string_view rule =
    "--------------------------------------------------------------------------\n";

void writeWeightRow(std::ostream& output, std::string_view name, Mass mass, const Position& centre,
                    Moment freeSurfaceMoment) {
    output << std::format("  {:<14}{:>11.1f}{:>11.2f}{:>10.2f}{:>10.2f}{:>14.1f}\n", name, mass.value(),
                          centre.x.value(), centre.y.value(), centre.z.value(), freeSurfaceMoment.value());
}

void writeQuantity(std::ostream& output, std::string_view label, double value, std::string_view unit,
                   int decimals = 3) {
    output << std::format("  {:<28}{:>10.{}f} {}\n", label, value, decimals, unit);
}

} // namespace

void writeReport(std::ostream& output, const Analysis& analysis) {
    const FloatingPosition& position = analysis.floatingPosition;
    const InitialStability& stability = analysis.stability;

    output << "LOADICATOR  " << analysis.conditionName << '\n'
           << "Method:      " << toString(position.method) << '\n'
           << "Conventions: x from AP; y, TCG and heel positive to port; z above baseline;\n"
           << "             trim positive by the stern; sea water 1.025 t/m3\n\n";

    output << "WEIGHTS\n" << rule;
    output << std::format("  {:<14}{:>11}{:>11}{:>10}{:>10}{:>14}\n", "Item", "Mass [t]", "LCG [m]",
                          "TCG [m]", "VCG [m]", "FSM [t*m]");
    output << rule;
    for (const WeightItem& item : analysis.weights) {
        writeWeightRow(output, item.name, item.mass, item.centre, item.freeSurfaceMoment);
    }
    output << rule;
    writeWeightRow(output, "Total", analysis.mass.displacement, analysis.mass.centreOfGravity,
                   analysis.mass.freeSurfaceMoment);
    output << '\n';

    output << "FLOATING POSITION\n" << rule;
    writeQuantity(output, "Draught midships", position.draughtMidships.value(), "m");
    writeQuantity(output, "Draught aft (TA)", position.draughtAft.value(), "m");
    writeQuantity(output, "Draught forward (TF)", position.draughtForward.value(), "m");
    writeQuantity(output, "Trim", position.trim.value(), "m");
    writeQuantity(output, "LCB", position.lcb.value(), "m from AP");
    writeQuantity(output, "LCF", position.lcf.value(), "m from AP");
    if (position.trimExtrapolated) {
        output << "  Note: trim lies outside the tabulated range; hydrostatics extrapolated.\n";
    }
    output << '\n';

    output << "INITIAL STABILITY\n" << rule;
    writeQuantity(output, "KMT", stability.kmt.value(), "m");
    writeQuantity(output, "KG", stability.kg.value(), "m");
    writeQuantity(output, "GM solid (KMT - KG)", stability.gmSolid.value(), "m");
    writeQuantity(output, "Free-surface correction", stability.freeSurfaceCorrection.value(), "m");
    writeQuantity(output, "GM fluid", stability.gmFluid.value(), "m");
    if (stability.heel) {
        writeQuantity(output, "Heel", stability.heel->degrees(), "deg", 2);
    } else {
        output << "  Heel                        UNSTABLE: GM fluid is not positive\n";
    }
}

} // namespace loadicator
