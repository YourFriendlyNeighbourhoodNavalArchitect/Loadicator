#include "Ship.h"

// Implementing the constructor
Ship::Ship(const std::string& trimStabilityBook, const std::string& soundingTables, const std::string& userInput, const std::string& hydrostaticTables)
    : loadCond(trimStabilityBook, soundingTables, userInput), hydroReader(hydrostaticTables, 0.0), userInput(userInput) {
    loadCond.calculate();
    tankProperties = loadCond.getData();

    displacement = 0.0;
    longitudinalMoment = 0.0;
    transverseMoment = 0.0;
    verticalMoment = 0.0;
    LCG = 0.0;
    TCG = 0.0;
    VCG = 0.0;
    draughtMoulded = 0.0;
    LCF = 0.0;
    LCB = 0.0;
    VCB = 0.0;
    KMT = 0.0;
    MCT = 0.0;
    trim = 0.0;
    GM = 0.0;
    heel = 0.0;
    TF = 0.0;
    TA = 0.0;

    // Compute displacement after loading condition processing
    for (const auto& entry : tankProperties) {
        double mass, lcg, tcg, vcg, fsm;
        std::tie(mass, lcg, tcg, vcg, fsm) = entry.second;
        displacement += mass;
        longitudinalMoment += mass * lcg;
        transverseMoment += mass * tcg + fsm;
        verticalMoment += mass * vcg;
    }

    // Compute COG (if displacement is not zero)
    if (displacement != 0) {
        LCG = longitudinalMoment / displacement;
        TCG = transverseMoment / displacement;
        VCG = verticalMoment / displacement;
    }
    else {
        std::cerr << "Calculated displacement is equal to zero." << std::endl;
    }

    // Initialize hydroReader with computed displacement
    hydroReader = HydrostaticsReader(hydrostaticTables, displacement);
    std::tie(draughtMoulded, LCF, LCB, VCB, KMT, MCT) = hydroReader.interpolate();

    // Utilize known equations to calculate ship equilibrium
    trim = displacement * (LCB - LCG) / (100 * MCT);
    GM = KMT - VCG;
    heel = std::atan(TCG / GM) * 180 / M_PI;
    TF = draughtMoulded - 0.5 * trim;
    TA = draughtMoulded + 0.5 * trim;
}

// Implementing the printResultsToFile method
void Ship::printResultsToFile(const std::string& fileName) const {
    // Open file stream for writing
    std::ofstream outFile(fileName);

    // Check if file opened successfully
    if (!outFile.is_open()) {
        throw std::runtime_error("Failed to open " + fileName + " for writing.");
    }

    std::ostream& output = outFile;

    output << "Loading Condition: " << userInput << std::endl;
    output << "Total weight: " << displacement << " [tons]" << std::endl;
    output << "LCG: " << LCG << " [m from AP]" << std::endl;
    output << "TCG: " << TCG << " [m]" << std::endl;
    output << "VCG: " << VCG << " [m]" << std::endl;
    output << "Draught moulded: " << draughtMoulded << " [m]" << std::endl;
    output << "LCF: " << LCF << " [m from AP]" << std::endl;
    output << "LCB: " << LCB << " [m from AP]" << std::endl;
    output << "VCB: " << VCB << " [m]" << std::endl;
    output << "KMT: " << KMT << " [m]" << std::endl;
    output << "MCT: " << MCT << " [tons/cm]" << std::endl;
    output << "Trim: " << trim << " [m]" << std::endl;
    output << "GM: " << GM << " [m]" << std::endl;
    output << "Heel: " << heel << " [deg]" << std::endl;
    output << "TF: " << TF << " [m]" << std::endl;
    output << "TA: " << TA << " [m]" << std::endl;

    outFile.close();

    // Inform user about successful file creation
    std::cout << "Results have been written to " << fileName << std::endl;
}