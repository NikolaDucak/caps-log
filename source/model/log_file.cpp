#include "log_file.hpp"

#include "utils/string.hpp"
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>

namespace clog::model {

namespace {

/**
 * A bunch of regexes and group indexes for matching sections and tags.
 */
const auto SECTION_TITLE_REGEX = std::regex{"^# \\s*(.*?)\\s*$"};
const auto TAG_REGEX =
    std::regex{R"(^( +)?\*( +)([a-z A-Z 0-9]+)(\(.+\))?(:?))", std::regex_constants::extended};
const auto TASK_REGEX =
    std::regex{R"(^ *(- )?\[(.)] *(\(([[a-zA-Z0-9_:]*)\))? *([\sa-zA-Z0-9_-]*)( *):?( *)(.*))",
               std::regex_constants::extended};
const auto TASK_TITLE_MATCH{5};
const auto TAG_TITLE_MATCH{3};
const auto SECTION_TITLE_MATCH{1};

/**
 * A function that goes through a log file and calls a function for each line that is
 * not inside a markdown code block.
 */
void forEachLogLine(std::istream &input, const std::function<void(const std::string &)> &func) {
    std::string line;
    bool isInsideCodeBlock = false;
    while (getline(input, line)) {
        line = utils::lowercase(utils::trim(line));

        if (line.substr(0, 3) == "```")
            isInsideCodeBlock = !isInsideCodeBlock;

        if (isInsideCodeBlock)
            continue;

        func(line);
    }
}

} // namespace

bool LogFile::hasMeaningfullContent() {
    // TODO: maybe check if its more than a base template
    return not m_content.empty();
}

std::vector<std::string> LogFile::readTagTitles(std::istream &input) {
    std::vector<std::string> result;

    forEachLogLine(input, [&](const auto &line) {
        if (std::smatch sm; std::regex_match(line, sm, TAG_REGEX)) {
            result.push_back(utils::trim(sm[TAG_TITLE_MATCH]));
        }
    });

    return result;
}

std::vector<std::string> LogFile::readSectionTitles(std::istream &input, bool skipFirstLine) {
    std::vector<std::string> result;

    std::string line;

    if (skipFirstLine)
        getline(input, line);

    forEachLogLine(input, [&](const auto &line) {
        if (std::smatch sm; std::regex_match(line, sm, SECTION_TITLE_REGEX)) {
            result.push_back(utils::trim(sm[SECTION_TITLE_MATCH]));
        }
    });

    return result;
}

} // namespace clog::model
