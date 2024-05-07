#include "log_file.hpp"

#include "utils/string.hpp"
#include <functional>
#include <regex>

namespace caps_log::log {

namespace {

/**
 * A bunch of regexes and group indexes for matching sections and tags.
 */
const auto kSectionTitleRegex = std::regex{"^# \\s*(.*?)\\s*$"};
const auto kTagRegex =
    std::regex{R"(^( +)?\*( +)([a-z A-Z 0-9]+)(\(.+\))?(:.*)?)", std::regex_constants::extended};

const auto kTaskRegex =
    std::regex{R"(^ *(- )?\[(.)] *(\(([[a-zA-Z0-9_:]*)\))? *([\sa-zA-Z0-9_-]*)( *):?( *)(.*))",
               std::regex_constants::extended};
const auto kTaskTitleMatch{5};
const auto kTagTitleMatch{3};
const auto kSectionTitleMatch{1};

/**
 * A function that goes through a log file and calls a function for each line that is
 * not inside a markdown code block.
 */
void forEachLogLine(std::istream &input, const std::function<void(const std::string &)> &func) {
    std::string line;
    bool isInsideCodeBlock = false;
    while (getline(input, line)) {
        line = utils::lowercase(utils::trim(line));

        if (line.substr(0, 3) == "```") {
            isInsideCodeBlock = !isInsideCodeBlock;
        }

        if (isInsideCodeBlock) {
            continue;
        }

        func(line);
    }
}

} // namespace

std::vector<std::string> LogFile::readTagTitles() const {
    std::vector<std::string> result;
    std::stringstream sstream{m_content};

    forEachLogLine(sstream, [&](const auto &line) {
        if (std::smatch smatch; std::regex_match(line, smatch, kTagRegex)) {
            result.push_back(utils::trim(smatch[kTagTitleMatch]));
        }
    });

    return result;
}

std::vector<std::string> LogFile::readSectionTitles(bool skipFirstLine) const {
    std::stringstream sstream{m_content};
    std::vector<std::string> result;

    if (skipFirstLine) {
        std::string line;
        getline(sstream, line);
    }

    forEachLogLine(sstream, [&](const auto &line) {
        if (std::smatch smatch; std::regex_match(line, smatch, kSectionTitleRegex)) {
            result.push_back(utils::trim(smatch[kSectionTitleMatch]));
        }
    });

    return result;
}

} // namespace caps_log::log
