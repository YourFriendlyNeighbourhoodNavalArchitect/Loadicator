#include "CargoHoldReader.h"

// Implementing the constructor
CargoHoldReader::CargoHoldReader(const std::string& cargoHold) {
    if (!readFile(cargoHold)) {
        throw std::runtime_error("Unable to read TXT file: " + cargoHold);
    }
}

// Implementing the readFile method
bool CargoHoldReader::readFile(const std::string& cargoHold) {
    std::ifstream file(cargoHold);
    if (!file.is_open()) {
        return false; // Indicate error
    }
    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
        // Skip the first two lines, as required by the form of the TXT files
        if (lineCount <= 2) {
            continue;
        }
        line = trim(line);
        if (line.empty()) {
            continue; // Skip empty lines
        }
        std::stringstream ss(line);
        std::vector<double> row;
        double value;
        int columnCount = 0;
        while (ss >> value) {
            row.push_back(value);
            columnCount++;
        }
        // Ensure each row has exactly 6 columns, as required by the form of the TXT files
        if (columnCount == 6) {
            cargoData.push_back(row);
        }
    }

    file.close();
    return true; // Indicate success
}

// Implementing the getData method
const std::vector<std::vector<double>>& CargoHoldReader::getData() const {
    return cargoData;
}

// Implementing the trim function
std::string CargoHoldReader::trim(const std::string& str) const {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}