#include "io/textFile.h"

#include "util/error.h"

#include <fstream>
#include <iterator>

namespace loadicator::io {

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw LoadicatorError("cannot open '" + path.string() + "'");
    }
    std::string content{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    if (file.bad()) {
        throw LoadicatorError("cannot read '" + path.string() + "'");
    }
    return content;
}

}
