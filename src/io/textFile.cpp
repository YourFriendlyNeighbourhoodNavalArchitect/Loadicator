#include "loadicator/io/textFile.h"

#include "loadicator/util/error.h"

#include <fstream>
#include <iterator>
#include <string_view>

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
    // Text editors and spreadsheet programs may begin UTF-8 text with a byte order mark, which is no part of
    // the text itself.
    constexpr std::string_view byteOrderMark = "\xEF\xBB\xBF";
    if (content.starts_with(byteOrderMark)) {
        content.erase(0, byteOrderMark.size());
    }
    return content;
}

}
