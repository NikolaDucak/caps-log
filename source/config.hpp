#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include "fmt/format.h"
#include "version.hpp"

namespace clog {

using FileReader = std::function<std::unique_ptr<std::istream>(std::string)>;

struct Config {
    static Config make(const FileReader &fileReader, const class ArgParser &);

    // TODO: migrate from string to path where apropirate
    const std::string logDirPath;
    const std::string logFilenameFormat;
    const bool sundayStart;
    const bool ignoreFirstLineWhenParsingSections;
    const std::string password;
    // TODO: const bool useOldTaskSyntaxAsTags;
    // TODO: colors?

    static const std::string DEFAULT_CONFIG_LOCATION;
    static const std::string DEFAULT_LOG_DIR_PATH;
    static const std::string DEFAULT_LOG_FILENAME_FORMAT;
    static const bool DEFAULT_SATURDAY_START;
    static const bool DEFAULT_IGNORE_FIRST_LINE_WHEN_PARSING_SECTIONS;
};

inline std::string helpString() {
    // TODO: embed version
    // clang-format-off
    static const std::string HELP_STRING_BASE{R"(
clog (Captains Log)
A small TUI journaling tool.
Version {}

 -h --help                     - show this message
 -c --config <path             - override the default config file path (~/.clog/config.ini)
 --log-dir-path <path>         - path where log files are stored (default: ~/.clog/day/)
 --log-name-format <format>    - format in which log entry markdown files are saved (default: d%Y_%m_%d.md)
 --sunday-start                - have the calendar display sunday as first day of the week
 --first-line-section          - if a section mark is placed on the first line, 
                                 by default it is ignored as it's left for log title, this overrides this behaviour
 --password                    - password for encrypted log repositores or to be used with --encrypt/--decrypt
 --encrypt                     - apply encryption to all logs in log dir path (needs --password)
 --decrypt                     - apply decryption to all logs in log dir path (needs --password)
 )"};
    // clang-format-on
    return fmt::format(HELP_STRING_BASE, CLOG_VERSION);
}

} // namespace clog
