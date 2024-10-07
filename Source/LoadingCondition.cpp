#include "LoadingCondition.h"

// Implementing the constructor
LoadingCondition::LoadingCondition(const std::string& trimStabilityBook, const std::string& soundingTables, const std::string& userInput)
    : trimStabilityBook(trimStabilityBook), soundingTables(soundingTables), userInput(userInput) {
    try {
        // Instantiate trimStabilityReader and get tankPlan and densities
        TrimStabilityReader trimReader(trimStabilityBook, userInput);
        tankPlan = trimReader.getData();
        densities = trimReader.getDensities(); // Retrieve densities array
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Error instantiating TrimStabilityReader.");
    }
}

// Implementing the calculate method
void LoadingCondition::calculate() {
    try {
        // Instantiate SoundingTablesReader to use for interpolation
        SoundingTablesReader soundingReader(soundingTables, tankPlan);

        // Process each key in tankPlan
        for (const auto& pair : tankPlan) {
            const std::string& key = pair.first;
            try {
                // Keys corresponding to cargo holds
                if (key.size() >= 4 && key.substr(0, 3) == "R1.") {
                    double volume, lcg, tcg, vcg, fsm;
                    std::tie(volume, lcg, tcg, vcg, fsm) = cargoHoldsCalculations(key, tankPlan);
                    double density = getDensity(key);
                    double mass = density * volume;
                    // Store results for mass, LCG, TCG, VCG and FSM
                    tankProperties[key] = std::make_tuple(mass, lcg, tcg, vcg, fsm);
                }
                // Remaining keys, excluding "Floating Condition"
                else if (key != "Floating Condition") {
                    // Check if key can be found in soundingData for interpolation
                    std::vector<std::vector<double>> soundingData = soundingReader.getData(key);
                    // Keys corresponding to tanks for which sounding tables are available
                    if (!soundingData.empty()) {
                        double volume, lcg, tcg, vcg, IMOM;
                        double fillPercentage = pair.second[1];
                        std::tie(volume, lcg, tcg, vcg, IMOM) = tanksCalculations(soundingData, fillPercentage);
                        double density = getDensity(key);
                        double mass = density * volume;
                        double fsm = density * IMOM;
                        tankProperties[key] = std::make_tuple(mass, lcg, tcg, vcg, fsm);
                    }
                    else if (key != "Lightweight") {
                        // Draw values directly from tankPlan
                        double mass = pair.second[0];
                        double lcg = pair.second[2];
                        double tcg = pair.second[3];
                        double vcg = pair.second[4];
                        double fsm = pair.second[5];
                        tankProperties[key] = std::make_tuple(mass, lcg, tcg, vcg, fsm);
                    }
                    else {
                        // Draw values directly from tankPlan
                        double mass = pair.second[0];
                        double lcg = pair.second[1];
                        double tcg = pair.second[2];
                        double vcg = pair.second[3];
                        tankProperties[key] = std::make_tuple(mass, lcg, tcg, vcg, 0);
                    }
                }
            }
            catch (const std::exception& e) {
                throw std::runtime_error("Unable to calculate properties of compartment: " + key);
            }
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Error instantiating SoundingTablesReader.");
    }
}

// Implementing the getData method
const std::unordered_map<std::string, std::tuple<double, double, double, double, double>>& LoadingCondition::getData() const {
    return tankProperties;
}

// Implementing the tanksCalculations method
std::tuple<double, double, double, double, double> LoadingCondition::tanksCalculations(std::vector<std::vector<double>> soundingData, double fillPercentage) const {
    double volume = 0.0, lcg = 0.0, tcg = 0.0, vcg = 0.0, IMOM = 0.0;
    // Ensure soundingData has at least two rows for interpolation
    if (soundingData.size() < 2) {
        throw std::runtime_error("Insufficient data for interpolation.");
    }
    // Find the rows for which interpolation is to be performed
    size_t idx1 = 0, idx2 = 1;
    double diff1 = std::abs(soundingData[idx1][8] - fillPercentage);
    double diff2 = std::abs(soundingData[idx2][8] - fillPercentage);
    for (size_t i = 2; i < soundingData.size(); ++i) {
        double currentDiff = std::abs(soundingData[i][8] - fillPercentage);
        if (currentDiff < diff1) {
            idx2 = idx1;
            idx1 = i;
            diff2 = diff1;
            diff1 = currentDiff;
        }
        else if (currentDiff < diff2) {
            idx2 = i;
            diff2 = currentDiff;
        }
    }
    // Ensure no division by zero
    if (soundingData[idx2][8] != soundingData[idx1][8]) {
        // Perform linear interpolations for volume, LCG, TCG, VCG and IMOM columns
        double value1 = soundingData[idx1][1];
        double value2 = soundingData[idx2][1];
        double fraction = (fillPercentage - soundingData[idx1][8]) / (soundingData[idx2][8] - soundingData[idx1][8]);
        volume = value1 + fraction * (value2 - value1);

        value1 = soundingData[idx1][9];
        value2 = soundingData[idx2][9];
        lcg = value1 + fraction * (value2 - value1);

        value1 = soundingData[idx1][10];
        value2 = soundingData[idx2][10];
        tcg = value1 + fraction * (value2 - value1);

        value1 = soundingData[idx1][11];
        value2 = soundingData[idx2][11];
        vcg = value1 + fraction * (value2 - value1);

        value1 = soundingData[idx1][12];
        value2 = soundingData[idx2][12];
        IMOM = value1 + fraction * (value2 - value1);
        if (IMOM < 0) {
            IMOM = 0;
        }
    }
    else {
        volume = soundingData[idx1][1];
        lcg = soundingData[idx1][9];
        tcg = soundingData[idx1][10];
        vcg = soundingData[idx1][11];
        IMOM = soundingData[idx1][12];
    }

    return std::make_tuple(volume, lcg, tcg, vcg, IMOM);
}

// Implementing the cargoHoldsCalculations method
std::tuple<double, double, double, double, double> LoadingCondition::cargoHoldsCalculations(const std::string& key, const std::unordered_map<std::string, std::vector<double>>& tankPlan) const {
    double volume = 0.0, lcg = 0.0, tcg = 0.0, vcg = 0.0, fsm = 0.0;
    // Fourth character in the key corresponds to the hold number
    char holdNumber = key[3];
    if (isdigit(holdNumber)) {
        std::string fileName = "Data/Cargo hold data/Hold (" + std::string(1, holdNumber) + ").txt";
        CargoHoldReader holdReader(fileName);
        const auto& cargoData = holdReader.getData();
        double fillPercentage = tankPlan.at(key)[1];
        double maxCargo = cargoData[11][1]; // Accessing the 12th row of the 2nd column
        volume = fillPercentage * maxCargo / 100.0;
        // Find the rows for which interpolation is to be performed
        size_t idx1 = 0, idx2 = 1;
        double diff1 = std::abs(cargoData[idx1][1] - volume);
        double diff2 = std::abs(cargoData[idx2][1] - volume);
        for (size_t i = 2; i < cargoData.size(); ++i) {
            double currentDiff = std::abs(cargoData[i][1] - volume);
            if (currentDiff < diff1) {
                idx2 = idx1;
                idx1 = i;
                diff2 = diff1;
                diff1 = currentDiff;
            }
            else if (currentDiff < diff2) {
                idx2 = i;
                diff2 = currentDiff;
            }
        }
        // Perform linear interpolations for LCG, TCG, VCG, FSM columns
        double value1 = cargoData[idx1][2];
        double value2 = cargoData[idx2][2];
        double fraction = (volume - cargoData[idx1][1]) / (cargoData[idx2][1] - cargoData[idx1][1]);
        lcg = value1 + fraction * (value2 - value1);

        value1 = cargoData[idx1][3];
        value2 = cargoData[idx2][3];
        tcg = value1 + fraction * (value2 - value1);

        value1 = cargoData[idx1][4];
        value2 = cargoData[idx2][4];
        vcg = value1 + fraction * (value2 - value1);
    }
    else {
        throw std::invalid_argument("Invalid key format.");
    }

    return std::make_tuple(volume, lcg, tcg, vcg, fsm);
}

// Implementing the getDensity method
double LoadingCondition::getDensity(const std::string& key) const {
    char densityIndexChar = key[1]; // Assuming key format is "RX.Y"
    if (isdigit(densityIndexChar)) {
        int densityIndex = densityIndexChar - '0'; // Convert char to integer index
        if (densityIndex >= 1 && densityIndex <= densities.size()) {
            return densities[densityIndex - 1]; // Return density
        }
        else {
            throw std::out_of_range("Density index out of bounds.");
        }
    }
    else {
        throw std::invalid_argument("Invalid key format for density calculation.");
    }
}