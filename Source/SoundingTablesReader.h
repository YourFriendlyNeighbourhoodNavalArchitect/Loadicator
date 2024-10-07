#ifndef SOUNDINGTABLESREADER_H
#define SOUNDINGTABLESREADER_H

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <string>
#include <vector>
#include <regex>
#include <cctype>
#include <iostream>

class SoundingTablesReader {
public:
    SoundingTablesReader(const std::string& fileName, const std::unordered_map<std::string, std::vector<double>>& tankPlan);

    const std::vector<std::vector<double>>& getData(const std::string& key) const;

private:
    std::unordered_map<std::string, std::vector<std::vector<double>>> soundingData;

    void readFile(const std::string& fileName, const std::unordered_map<std::string, std::vector<double>>& tankPlan);
};

#endif // SOUNDINGTABLESREADER_H