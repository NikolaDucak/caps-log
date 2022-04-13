#include "LogFile.hpp"

#include <filesystem>
#include <fstream>
#include <regex>
#include <set>

namespace clog::model {

namespace {
std::string replace_first_occurance(std::string& s, const std::string& toReplace,
                                    const std::string& replaceWith) {
    std::size_t pos = s.find(toReplace);
    if (pos == std::string::npos)
        return s;
    return s.replace(pos, toReplace.length(), replaceWith);
}

std::string lowercase(std::string& str) {
    std::string data = str;
    std::transform(data.begin(), data.end(), data.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return data;
}

std::string trim(const std::string& str, const std::string& whitespace = " \t\n") {
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return "";  // no content
    const auto strEnd   = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;
    return str.substr(strBegin, strRange);
}

}  // namespace

const std::string LogFile::LOG_FILE_TITLE_DATE_FORMAT = "%-d. %-m. %Y.";
const std::string LogFile::LOG_FILE_BASE_TEMPLATE =
    R"(# %DATE%    

# Summary

# Tasks

)";

bool LogFile::hasMeaningfullContent() {
    // it is meaningless if it has all empty sections
    return true;
}


}  // namespace clog::model
