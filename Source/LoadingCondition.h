#ifndef LOADINGCONDITION_H
#define LOADINGCONDITION_H

#include "TrimStabilityReader.h"
#include "SoundingTablesReader.h"
#include "CargoHoldReader.h"
#include <vector>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <iterator>
#include <stdexcept>

class LoadingCondition {
public:
    LoadingCondition(const std::string& trimStabilityBook, const std::string& soundingTables, const std::string& userInput);

    void calculate();

    const std::unordered_map<std::string, std::tuple<double, double, double, double, double>>& getData() const;

private:
    std::string trimStabilityBook;
    std::string soundingTables;
    std::string userInput;
    std::unordered_map<std::string, std::vector<double>> tankPlan;
    std::vector<double> densities;
    std::unordered_map<std::string, std::tuple<double, double, double, double, double>> tankProperties;
    
    std::tuple<double, double, double, double, double> tanksCalculations(std::vector<std::vector<double>> soundingData, double fillPercentage) const;

    std::tuple<double, double, double, double, double> cargoHoldsCalculations(const std::string& key, const std::unordered_map<std::string, std::vector<double>>& tankPlan) const;

    double getDensity(const std::string& key) const;
};

#endif // LOADINGCONDITION_H