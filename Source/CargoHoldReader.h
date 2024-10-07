#ifndef CARGOHOLDREADER_H
#define CARGOHOLDREADER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>

class CargoHoldReader {
public:
    CargoHoldReader(const std::string& cargoHold);
    bool readFile(const std::string& cargoHold);
    const std::vector<std::vector<double>>& getData() const;

private:
    std::vector<std::vector<double>> cargoData;
    std::string trim(const std::string& str) const;
};

#endif // CARGOHOLDREADER_H