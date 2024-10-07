#ifndef HYDROSTATICSREADER_H
#define HYDROSTATICSREADER_H

#include <string>
#include <vector>
#include <iostream>
#include <regex>
#include <fpdfview.h>
#include <fpdf_text.h>
#include <stdexcept>
#include <tuple>

class HydrostaticsReader {
public:
    HydrostaticsReader(const std::string& fileName, double displacement);
    const std::vector<std::vector<double>>& getData() const;
    std::tuple<double, double, double, double, double, double> interpolate() const;

private:
    std::vector<std::vector<double>> hydrostaticData;
    size_t currentColumn;
    size_t currentRow;
    double displacement;

    std::vector<double> extractNumericalValues(const std::string& line);
    void searchForPattern(FPDF_PAGE page);
    void insertValues(const std::vector<double>& values);
};

#endif // HYDROSTATICSREADER_H