#ifndef TRIMSTABILITYREADER_H
#define TRIMSTABILITYREADER_H

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <regex>
#include <fpdfview.h>
#include <fpdf_text.h>
#include <stdexcept>
#include <sstream>

class TrimStabilityReader {
public:
    TrimStabilityReader(const std::string& fileName, const std::string& userInput);

    const std::unordered_map<std::string, std::vector<double>>& getData() const;

    const std::vector<double>& getDensities() const;

private:
    std::unordered_map<std::string, std::vector<double>> tankPlan;
    std::vector<double> densities;

    bool isValidInputFormat(const std::string& userInput);

    bool processText(FPDF_TEXTPAGE textPage, int userInput);

    bool searchForPatterns(FPDF_TEXTPAGE textPage);

    std::vector<double> extractNumericalValues(const std::string& line, bool skipFirst, bool skipNext);
};

#endif // TRIMSTABILITYREADER_H