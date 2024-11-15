#pragma once

#include "log_file.hpp"

#include <chrono>
#include <optional>

namespace caps_log::log {

/*
 * Only class that actually interacts with physical files on the drive
 */
class LogRepositoryBase {
  public:
    virtual ~LogRepositoryBase() = default;

    virtual std::optional<LogFile> read(const std::chrono::year_month_day &date) const = 0;
    virtual void write(const LogFile &log) = 0;
    virtual void remove(const std::chrono::year_month_day &date) = 0;
};

} // namespace caps_log::log
