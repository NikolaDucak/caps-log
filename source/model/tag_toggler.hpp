#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "fmt/format.h"
#include "log_repository_base.hpp"
#include "utils/string.hpp"

struct TagTogglerConfig {
    std::vector<std::string> tagDestinationSections;

    bool allowAddingToNonExistingLogFiles;
    bool removeAllTagInstances = true;
    bool removeTagsWithBodies = false;

    bool addLogsToSpecificsection() { return not tagDestinationSections.empty(); }
};

class TagToggler {
    std::shared_ptr<caps_log::model::LogRepositoryBase> m_repo;
    TagTogglerConfig m_config;

  public:
    TagToggler(std::shared_ptr<caps_log::model::LogRepositoryBase> repo, TagTogglerConfig conf)
        : m_repo{std::move(repo)}, m_config{std::move(conf)} {}

    void toggle(const std::string &tagName, caps_log::date::Date date) {
        auto log = m_repo->read(date);
        if (log) {
            toggleInExistingFile(tagName, *log);
        } else {
            toggleInNonExistingFile(tagName, date);
        }
    }

  private:
    void toggleInNonExistingFile(const std::string &tagName, caps_log::date::Date &date) {
        if (not m_config.allowAddingToNonExistingLogFiles)
            return;

        auto content = (m_config.addLogsToSpecificsection())
                           ? write(m_config.tagDestinationSections.front(), tagName)
                           : makeTagString(tagName);
        m_repo->write({date, content});
    }

    void toggleInExistingFile(const std::string &tagName, caps_log::model::LogFile &log) {
        auto content = log.getContent();
        if (auto tags = findTagIfAlreadyThere(tagName, content); not tags.empty()) {
            for (const auto &tag : tags) {
                eraseSubStr(content, tag);
            }

            if (caps_log::utils::trim(content).empty()) {
                m_repo->remove(log.getDate());
            } else {
                m_repo->write({log.getDate(), content});
            }

        } else {
            content.insert(getInsertionDestinationIter(content), makeTagString(tagName));
            m_repo->write({log.getDate(), content});
        }
    }

    void eraseSubStr(std::string &mainStr, const std::string &toErase) {
        // Search for the substring in string
        size_t pos = mainStr.find(toErase);
        if (pos != std::string::npos) {
            // If found then erase it from string
            mainStr.erase(pos, toErase.length());
        }
    }

    std::vector<std::string> findTagIfAlreadyThere(const std::string &tagName,
                                                   const std::string &content) {
        std::stringstream ss{content};
        std::vector<std::string> resultV;
        bool inTag = false;

        for (std::string line; std::getline(ss, line);) {
            if (isTagLine(line, tagName)) {
                inTag = !inTag;
                resultV.push_back(line);
            } else if (caps_log::utils::trim(line).empty() && inTag) {
                inTag = false;
            } else {
                if (inTag)
                    resultV.back() += line;
            }
        }

        return resultV;
    }

    bool isTagLine(const std::string &line, const std::string &targetTag) {
        auto tags = caps_log::model::LogFile::readTagTitles(line);
        return std::find(tags.begin(), tags.end(), targetTag) != tags.end();
    }

    std::string::size_type getInsertionDestinationIter(const std::string &content) {
        if (m_config.addLogsToSpecificsection()) {
            return findIterAfterFistMatchingSection(content, m_config.tagDestinationSections);
        } else {
            return content.length();
        }
    }

    std::string::size_type
    findIterAfterFistMatchingSection(const std::string &content,
                                     const std::vector<std::string> &sections) {
        int pos = 0;
        std::stringstream ss{content};
        for (std::string line; std::getline(ss, line);) {
            pos += line.length();
            if (isTargetLine(line, m_config.tagDestinationSections)) {
                return pos;
            }
        }

        return content.length();
    }

    bool isTargetLine(const std::string &line, const std::vector<std::string> &targets) {
        auto sections = caps_log::model::LogFile::readSectionTitles(line);
        return std::find(targets.begin(), targets.end(), sections.front()) != targets.end();
    }

    std::string makeTagString(const std::string &str) { return fmt::format("* {}", str); }

    std::string write(const std::string &section, const std::string &tag) {
        return fmt::format("# {}\n* {}", section, tag);
    }
};
