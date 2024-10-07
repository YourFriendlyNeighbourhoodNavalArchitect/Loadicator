#include "SoundingTablesReader.h"

// Implementing the constructor
SoundingTablesReader::SoundingTablesReader(const std::string& fileName, const std::unordered_map<std::string, std::vector<double>>& tankPlan) {
    readFile(fileName, tankPlan);
}

// Implementing the getData method
const std::vector<std::vector<double>>& SoundingTablesReader::getData(const std::string& key) const {
    auto it = soundingData.find(key);
    if (it != soundingData.end()) {
        return it->second;
    }
    else {
        static const std::vector<std::vector<double>> emptyMatrix;
        return emptyMatrix;
    }
}

// Implementing the readFile method
void SoundingTablesReader::readFile(const std::string& fileName, const std::unordered_map<std::string, std::vector<double>>& tankPlan) {
    std::ifstream file(fileName);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open file " + fileName);
    }

    std::string line;
    std::string currentKey;
    std::vector<std::vector<double>> currentMatrix;
    int skipCount = 0;
    bool capturing = false;

    // Regular expressions to identify useful data
    std::regex keyRegex(R"(Compartment ident: (\S+))");
    std::regex dashRegex(R"(-{3,})");

    while (std::getline(file, line)) {
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
            return !std::isspace(ch);
            }));

        std::smatch match;
        if (std::regex_search(line, match, keyRegex)) {
            std::string foundKey = match[1].str();
            if (tankPlan.find(foundKey) != tankPlan.end()) {
                if (!currentKey.empty() && !currentMatrix.empty()) {
                    soundingData[currentKey] = currentMatrix;
                }
                currentKey = foundKey;
                currentMatrix.clear();
                // skipCount set according to the form of the TXT file
                skipCount = 19;
                capturing = false;
            }
            else if (capturing) {
                break;
            }
        }

        if (!currentKey.empty()) {
            if (skipCount > 0) {
                --skipCount;
            }
            else if (std::regex_search(line, dashRegex)) {
                soundingData[currentKey] = currentMatrix;
                currentKey = "";
                currentMatrix.clear();
                capturing = false;
            }
            else {
                capturing = true;

                std::istringstream iss(line);
                std::vector<double> doubles;
                double value;
                while (iss >> value) {
                    doubles.push_back(value);
                }
                // 13 is the number of data columns to be stored
                while (doubles.size() < 13 && std::getline(file, line)) {
                    line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
                        return !std::isspace(ch);
                        }));

                    std::istringstream iss_next(line);
                    while (iss_next >> value) {
                        doubles.push_back(value);
                    }
                }

                if (doubles.size() == 13) {
                    currentMatrix.push_back(doubles);
                }
            }
        }
    }

    if (!currentKey.empty() && !currentMatrix.empty()) {
        soundingData[currentKey] = currentMatrix;
    }

    file.close();
}