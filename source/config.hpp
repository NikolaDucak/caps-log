#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include "fmt/format.h"
#include "version.hpp"
#include <boost/program_options.hpp>

namespace caps_log {

using FileReader = std::function<std::unique_ptr<std::istream>(std::string)>;

struct Config {
    static Config make(const FileReader &fileReader,
                       const boost::program_options::variables_map &cmdLineArgs);

    static const std::string DEFAULT_CONFIG_LOCATION;
    static const std::string DEFAULT_LOG_DIR_PATH;
    static const std::string DEFAULT_LOG_FILENAME_FORMAT;
    static const bool DEFAULT_SUNDAY_START;
    static const bool DEFAULT_IGNORE_FIRST_LINE_WHEN_PARSING_SECTIONS;

    std::filesystem::path logDirPath = DEFAULT_LOG_DIR_PATH;
    std::filesystem::path logFilenameFormat = DEFAULT_LOG_FILENAME_FORMAT;
    bool sundayStart = DEFAULT_SUNDAY_START;
    bool ignoreFirstLineWhenParsingSections = DEFAULT_IGNORE_FIRST_LINE_WHEN_PARSING_SECTIONS;
    std::string password;
};

// NOLINTNEXTLINE
boost::program_options::variables_map parseCLIOptions(int argc, const char *argv[]);

} // namespace caps_log
