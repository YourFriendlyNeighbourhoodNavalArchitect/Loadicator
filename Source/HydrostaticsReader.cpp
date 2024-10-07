#include "HydrostaticsReader.h"

// Implementing the constructor
HydrostaticsReader::HydrostaticsReader(const std::string& fileName, double displacement)
    : currentColumn(0), currentRow(0), displacement(displacement) {
    FPDF_InitLibrary();
    FPDF_DOCUMENT document = FPDF_LoadDocument(fileName.c_str(), nullptr);
    if (!document) {
        FPDF_DestroyLibrary();
        throw std::runtime_error("Unable to open PDF file " + fileName);
    }

    // Initial size of output matrix, based on the form of the PDF file 
    hydrostaticData.resize(83, std::vector<double>(30));

    int pageCount = FPDF_GetPageCount(document);

    // First few pages are of no use
    for (int i = 3; i < pageCount; ++i) {
        FPDF_PAGE page = FPDF_LoadPage(document, i);
        if (!page) {
            continue;
        }

        searchForPattern(page);

        FPDF_ClosePage(page);
    }

    FPDF_CloseDocument(document);
    FPDF_DestroyLibrary();
}

// Implementing the getData method
const std::vector<std::vector<double>>& HydrostaticsReader::getData() const {
    return hydrostaticData;
}

// Implementing the interpolate method
std::tuple<double, double, double, double, double, double> HydrostaticsReader::interpolate() const {
    if (hydrostaticData.empty() || hydrostaticData[1].empty()) {
        throw std::runtime_error("Matrix is empty or not properly initialized.");
    }

    size_t idx1 = 0, idx2 = 1;
    double diff1 = std::abs(hydrostaticData[idx1][1] - displacement);
    double diff2 = std::abs(hydrostaticData[idx2][1] - displacement);

    // Find the rows for which interpolation is to be performed
    for (size_t i = 2; i < hydrostaticData.size(); ++i) {
        double currentDiff = std::abs(hydrostaticData[i][1] - displacement);
        if (currentDiff < diff1) {
            idx2 = idx1;
            diff2 = diff1;
            idx1 = i;
            diff1 = currentDiff;
        }
        else if (currentDiff < diff2) {
            idx2 = i;
            diff2 = currentDiff;
        }
    }

    // 139.1 is half the ship's LBP
    double draughtMoulded, LCF, LCB, VCB, KMT, MCT;
    if (hydrostaticData[idx1][1] != hydrostaticData[idx2][1]) {
        double fraction = (displacement - hydrostaticData[idx1][1]) / (hydrostaticData[idx2][1] - hydrostaticData[idx1][1]);
        draughtMoulded = hydrostaticData[idx1][0] + fraction * (hydrostaticData[idx2][0] - hydrostaticData[idx1][0]);
        LCF = 139.1 + hydrostaticData[idx1][4] + fraction * (hydrostaticData[idx2][4] - hydrostaticData[idx1][4]);
        LCB = 139.1 + hydrostaticData[idx1][5] + fraction * (hydrostaticData[idx2][5] - hydrostaticData[idx1][5]);
        VCB = hydrostaticData[idx1][6] + fraction * (hydrostaticData[idx2][6] - hydrostaticData[idx1][6]);
        KMT = hydrostaticData[idx1][7] + fraction * (hydrostaticData[idx2][7] - hydrostaticData[idx1][7]);
        MCT = hydrostaticData[idx1][11] + fraction * (hydrostaticData[idx2][11] - hydrostaticData[idx1][11]);
    }
    else {
        draughtMoulded = hydrostaticData[idx1][0];
        LCB = 139.1 + hydrostaticData[idx1][4];
        LCB = 139.1 + hydrostaticData[idx1][5];
        VCB = hydrostaticData[idx1][6];
        KMT = hydrostaticData[idx1][7];
        MCT = hydrostaticData[idx1][11];
    }

    return std::make_tuple(draughtMoulded, LCF, LCB, VCB, KMT, MCT);
}

// Implementing the extractNumericalValues method
std::vector<double> HydrostaticsReader::extractNumericalValues(const std::string& line) {
    std::vector<double> values;
    // Regular expression included decimal numbers of the form ".XXX"
    std::regex numberRegex(R"(-?\d+(\.\d+)?|\.\d+)");

    size_t patternPos = line.find("]:");
    if (patternPos == std::string::npos) {
        return values;
    }

    std::string subLine = line.substr(patternPos + 2);

    auto numbersBegin = std::sregex_iterator(subLine.begin(), subLine.end(), numberRegex);
    auto numbersEnd = std::sregex_iterator();

    for (std::sregex_iterator i = numbersBegin; i != numbersEnd; ++i) {
        std::smatch match = *i;
        values.push_back(std::stod(match.str()));
    }

    return values;
}

// Implementing the searchForPattern method
void HydrostaticsReader::searchForPattern(FPDF_PAGE page) {
    FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);

    int textCount = FPDFText_CountChars(textPage);
    if (textCount <= 0) {
        FPDFText_ClosePage(textPage);
        return;
    }

    std::string line;
    bool patternFound = false;

    for (int i = 0; i < textCount; ++i) {
        unsigned short unicode = FPDFText_GetUnicode(textPage, i);
        char asciiChar = static_cast<char>(unicode);

        // Line seperators
        if (asciiChar == '\n' || asciiChar == '\r') {
            if (patternFound) {
                std::vector<double> values = extractNumericalValues(line);
                insertValues(values);
            }
            line.clear();
            patternFound = false;
        }
        else {
            line += asciiChar;
            if (line.find("]:") != std::string::npos) {
                patternFound = true;
            }
        }
    }

    if (patternFound) {
        std::vector<double> values = extractNumericalValues(line);
        insertValues(values);
    }

    FPDFText_ClosePage(textPage);
}

// Implementing the insertValues method
void HydrostaticsReader::insertValues(const std::vector<double>& values) {
    size_t numValues = values.size();

    // In the case that the initial size estimation is incorrect
    if (currentRow + numValues > hydrostaticData.size()) {
        hydrostaticData.resize(currentRow + numValues, std::vector<double>(30));
    }

    // Popoulating the matrix
    for (size_t i = 0; i < numValues; ++i) {
        if (currentRow + i < hydrostaticData.size() && currentColumn < hydrostaticData[currentRow + i].size()) {
            hydrostaticData[currentRow + i][currentColumn] = values[i];
        }
        else {
            throw std::runtime_error("Attempted to access out-of-bounds element in matrix.");
        }
    }

    currentColumn++;

    if (currentColumn >= 30) {
        currentColumn = 0;
        currentRow += numValues;
    }
}