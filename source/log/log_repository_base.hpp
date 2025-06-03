#pragma once

#include "log_file.hpp"

#include <chrono>
#include <optional>
#include <vector>

namespace caps_log::log {

struct Scratchpad {
    std::string title;
    std::string content;
    std::chrono::year_month_day dateModified;
};

using Scratchpads = std::vector<Scratchpad>;

class ScratchpadRepositoryBase {
  public:
    ScratchpadRepositoryBase() = default;
    ScratchpadRepositoryBase(const ScratchpadRepositoryBase &) = default;
    ScratchpadRepositoryBase(ScratchpadRepositoryBase &&) = default;
    ScratchpadRepositoryBase &operator=(const ScratchpadRepositoryBase &) = default;
    ScratchpadRepositoryBase &operator=(ScratchpadRepositoryBase &&) = default;
    virtual ~ScratchpadRepositoryBase() = default;

    [[nodiscard]] virtual Scratchpads read() const = 0;
    virtual void remove(std::string name) = 0;
    virtual void rename(std::string oldName, std::string newName) = 0;
};

/*
 * Only class that actually interacts with physical files on the drive
 */
class LogRepositoryBase {
  public:
    LogRepositoryBase() = default;
    LogRepositoryBase(const LogRepositoryBase &) = default;
    LogRepositoryBase(LogRepositoryBase &&) = default;
    LogRepositoryBase &operator=(const LogRepositoryBase &) = default;
    LogRepositoryBase &operator=(LogRepositoryBase &&) = default;
    virtual ~LogRepositoryBase() = default;

    [[nodiscard]] virtual std::optional<LogFile>
    read(const std::chrono::year_month_day &date) const = 0;
    virtual void write(const LogFile &log) = 0;
    virtual void remove(const std::chrono::year_month_day &date) = 0;
};

} // namespace caps_log::log
