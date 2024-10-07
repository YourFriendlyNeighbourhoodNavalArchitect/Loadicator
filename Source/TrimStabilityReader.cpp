#include "TrimStabilityReader.h"

// Implementing the constructor
TrimStabilityReader::TrimStabilityReader(const std::string& fileName, const std::string& userInput) {
    // Validate userInput format
    if (!isValidInputFormat(userInput)) {
        throw std::runtime_error("Improper input format.");
    }

    // Convert userInput to int for processing
    int userIntInput = std::stoi(userInput);
    if (userIntInput < 1 || userIntInput > 31) {
        throw std::runtime_error("Non-existent loading condition.");
    }

    FPDF_InitLibrary();
    FPDF_DOCUMENT document = FPDF_LoadDocument(fileName.c_str(), nullptr);
    if (!document) {
        FPDF_DestroyLibrary();
        throw std::runtime_error("Unable to open PDF file " + fileName);
    }

    int pageCount = FPDF_GetPageCount(document);
    bool keywordFound = false;
    bool draughtMouldedFound = false;

    for (int i = 0; i < pageCount && !draughtMouldedFound; ++i) {
        FPDF_PAGE page = FPDF_LoadPage(document, i);
        if (!page) {
            continue;
        }

        FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
        if (!textPage) {
            FPDF_ClosePage(page);
            continue;
        }

        if (!keywordFound && processText(textPage, userIntInput)) {
            keywordFound = true;
        }

        if (keywordFound) {
            draughtMouldedFound = searchForPatterns(textPage);
        }

        FPDFText_ClosePage(textPage);
        FPDF_ClosePage(page);
    }

    FPDF_CloseDocument(document);
    FPDF_DestroyLibrary();
}

// Implementing the getData method
const std::unordered_map<std::string, std::vector<double>>& TrimStabilityReader::getData() const {
    return tankPlan;
}

// Implementing the getDensities method
const std::vector<double>& TrimStabilityReader::getDensities() const {
    return densities;
}

// Implementing isValidInputFormat method
bool TrimStabilityReader::isValidInputFormat(const std::string& userInput) {
    std::regex formatRegex(R"(\d{2})");
    return std::regex_match(userInput, formatRegex);
}

// Implementing the processText method
bool TrimStabilityReader::processText(FPDF_TEXTPAGE textPage, int userInput) {
    int textCount = FPDFText_CountChars(textPage);
    if (textCount <= 0) {
        return false;
    }

    std::string text;
    text.reserve(textCount);

    // Extract all text into a single string
    for (int i = 0; i < textCount; ++i) {
        unsigned short unicode = FPDFText_GetUnicode(textPage, i);
        text += static_cast<char>(unicode);
    }

    // Define the keyword
    char keyword[30];
    sprintf_s(keyword, sizeof(keyword), "LOADING CONDITION - COND%02d", userInput);

    // Search for the keyword
    if (text.find(keyword) != std::string::npos) {
        return true;
    }
    return false;
}

// Implementing the searchForPatterns method
bool TrimStabilityReader::searchForPatterns(FPDF_TEXTPAGE textPage) {
    int textCount = FPDFText_CountChars(textPage);
    if (textCount <= 0) {
        return false;
    }

    std::string text;
    text.reserve(textCount);

    // Extract all text into a single string
    for (int i = 0; i < textCount; ++i) {
        unsigned short unicode = FPDFText_GetUnicode(textPage, i);
        text += static_cast<char>(unicode);
    }

    // Define the regular expressions to search for
    std::regex patternRegex(R"(R([1-6])\.(\d{1,2}|0[1-9])(?:([PS])?)?)");
    std::regex floatingConditionRegex(R"(Draught\s+moulded)");
    std::regex lightweightRegex(R"(Lightweight)");
    std::regex crewStRegex(R"(CREW&ST\.)");
    std::regex oilWatRegex(R"(OIL&WAT\.)");
    std::regex rhoRegex(R"(RHO)");

    // Split the text into lines and search each line for the pattern
    std::istringstream stream(text);
    std::string line;
    bool foundDraughtMoulded = false;
    int currentLineIndex = 0;

    while (std::getline(stream, line)) {
        std::smatch match;
        if (std::regex_search(line, match, patternRegex)) {
            // Process patternRegex
            std::string remainingLine; // String to hold the remaining part of the line

            // Check for "NO.X" pattern (unwated X numerical value)
            std::regex noXPattern(R"(NO\.(\d+))"); // Capture the integer after "NO."
            std::smatch noXMatch;
            if (std::regex_search(line, noXMatch, noXPattern)) {
                // Both patternRegex and "NO.X" found, start after "NO.X"
                size_t pos = line.find(noXMatch[0].str()) + noXMatch[0].str().size();
                remainingLine = line.substr(pos);
            }
            else {
                // Only patternRegex found, start after the pattern itself
                size_t pos = line.find(match[0].str()) + match[0].str().size();
                remainingLine = line.substr(pos);
            }

            // Extract numerical values from the remaining part of the line
            std::vector<double> values = extractNumericalValues(remainingLine, false, false);
            tankPlan[match.str()].insert(tankPlan[match.str()].end(), values.begin(), values.end());
        }
        if (std::regex_search(line, lightweightRegex)) {
            // Save the line where "Lightweight" is found
            std::vector<double> values = extractNumericalValues(line, false, false);
            tankPlan["Lightweight"].insert(tankPlan["Lightweight"].end(), values.begin(), values.end());
        }
        if (std::regex_search(line, crewStRegex)) {
            // Save the line where "CREW&ST." is found
            std::vector<double> values = extractNumericalValues(line, false, false);
            tankPlan["Crew and Stores"].insert(tankPlan["Crew and Stores"].end(), values.begin(), values.end());
        }
        if (std::regex_search(line, oilWatRegex)) {
            // Save the line where "OIL&WAT." is found
            std::vector<double> values = extractNumericalValues(line, false, false);
            tankPlan["Oil and Water"].insert(tankPlan["Oil and Water"].end(), values.begin(), values.end());
        }
        if (std::regex_search(line, rhoRegex)) {
            // When "RHO" is found, extract numerical values from the same line
            std::vector<double> values = extractNumericalValues(line, false, false);

            // Store the current position in the input stream
            std::streampos originalPosition = stream.tellg();

            // Move two lines below
            std::string nextLine;
            if (std::getline(stream, nextLine) && std::getline(stream, nextLine)) {
                // Store values in densities vector
                // Indices correspond to compartment type, thus compartment content
                if (nextLine.size() > 2) {
                    char secondChar = nextLine[2];
                    if (std::isdigit(secondChar)) {
                        int index = secondChar - '0'; // Convert char to integer index

                        // Resize densities vector if necessary
                        if (index >= densities.size()) {
                            densities.resize(index + 1);
                        }

                        densities[index - 1] = values[0]; // Store the density value
                    }
                }
            }

            // Seek back to the original position in the input stream
            stream.seekg(originalPosition);
        }
        // This key is not used in the project, but we search for it nonetheless for two reasons
        // Firstly, to signal the code that it need not look any further for patterns
        // Secondly, to enable an iterative approximation of final equilibrium
        if (std::regex_search(line, floatingConditionRegex)) {
            // Save the line where "Draught moulded" is found
            std::vector<double> values = extractNumericalValues(line, false, false);
            tankPlan["Floating Condition"].insert(tankPlan["Floating Condition"].end(), values.begin(), values.end());
            foundDraughtMoulded = true;
            // Now continue saving the next 5 lines
            for (int k = 0; k < 5 && std::getline(stream, line); ++k) {
                std::vector<double> values = extractNumericalValues(line, false, false);
                tankPlan["Floating Condition"].insert(tankPlan["Floating Condition"].end(), values.begin(), values.end());
            }
            break; // Stop searching after "Draught moulded" is found
        }
        ++currentLineIndex;
    }

    return foundDraughtMoulded;
}

// Implementing the extractNumericalValues method
std::vector<double> TrimStabilityReader::extractNumericalValues(const std::string& line, bool skipFirst, bool skipNext) {
    std::vector<double> values;
    std::regex numberRegex(R"(-?\b\d+(\.\d+)?\b)");
    auto numbersBegin = std::sregex_iterator(line.begin(), line.end(), numberRegex);
    auto numbersEnd = std::sregex_iterator();

    bool skip = skipFirst;
    for (std::sregex_iterator i = numbersBegin; i != numbersEnd; ++i) {
        std::smatch match = *i;
        if (skip) {
            skip = false;
            continue;
        }
        if (skipNext) {
            skipNext = false;
            continue;
        }
        values.push_back(std::stod(match.str()));
    }

    return values;
}