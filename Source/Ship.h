#ifndef SHIP_H
#define SHIP_H

#include "LoadingCondition.h"
#include "HydrostaticsReader.h"
#include <string>
#include <unordered_map>
#include <tuple>
#include <iostream>
#include <cmath>
#include <fstream>

#ifndef M_PI
constexpr double M_PI = 3.14159265358979323846;
#endif

class Ship {
public:
    Ship(const std::string& trimStabilityBook, const std::string& soundingTables, const std::string& userInput, const std::string& hydrostaticTables);

    void printResultsToFile(const std::string& fileName = "Results.txt") const;

private:
    LoadingCondition loadCond;
    std::unordered_map<std::string, std::tuple<double, double, double, double, double>> tankProperties;
    HydrostaticsReader hydroReader;
    std::string userInput;
    double displacement, longitudinalMoment, transverseMoment, verticalMoment, LCG, TCG, VCG;
    double draughtMoulded, LCF, LCB, VCB, KMT, MCT, trim, GM, heel, TF, TA;
};

#endif // SHIP_H