#include "log_file.hpp"

#include "utils/string.hpp"
#include <functional>
#include <ranges>
#include <regex>
#include <sstream>

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

LogFile &LogFile::parse(bool skipFirstLine) {
    std::stringstream sstream{m_content};

    std::string lastSection = kRootSectionKey.data();

    forEachLogLine(sstream, [&](const auto &line) {
        if (std::smatch smatch; std::regex_match(line, smatch, kSectionTitleRegex)) {
            if (not skipFirstLine) {
                lastSection = utils::trim(smatch[kSectionTitleMatch]);
                m_tagsPerSection[lastSection] = {};
            }
        } else if (std::smatch smatch; std::regex_match(line, smatch, kTagRegex)) {
            m_tagsPerSection[lastSection].insert(utils::trim(smatch[kTagTitleMatch]));
        }
        skipFirstLine = false;
    });

    return *this;
}

std::set<std::string> LogFile::getTagTitles() const {
    std::set<std::string> tags;
    for (const auto &tagsPerSection : std::views::values(m_tagsPerSection)) {
        tags.insert(tagsPerSection.begin(), tagsPerSection.end());
    }
    return tags;
}

std::set<std::string> LogFile::getSectionTitles() const {
    std::set<std::string> sections;
    for (const auto &section : std::views::keys(m_tagsPerSection)) {
        sections.insert(section);
    }
    return sections;
}

std::map<std::string, std::set<std::string>> LogFile::getTagsPerSection() const {
    return m_tagsPerSection;
}

} // namespace caps_log::log
