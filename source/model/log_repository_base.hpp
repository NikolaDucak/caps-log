#pragma once

#include "date/date.hpp"
#include "log_file.hpp"

#include "utils/map.hpp"
#include "utils/string.hpp"
#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <regex>
#include <sstream>

namespace clog::model {

/*
 * Only class that actualy interacts with physical files on the drive
 */
class LogRepositoryBase {
  public:
    virtual ~LogRepositoryBase(){};

    virtual std::optional<LogFile> read(const date::Date &date) const = 0;
    virtual void write(const LogFile &log) = 0;
    virtual void remove(const date::Date &date) = 0;
};

} // namespace clog::model
