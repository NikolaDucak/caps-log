#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace clog {

using FileReader = std::function<std::unique_ptr<std::istream>(std::string)>;

struct Config {
    // TODO: migrate from string to path where apropirate
    const std::string logDirPath;
    const std::string logFilenameFormat;
    const bool sundayStart;
    const bool ignoreFirstLineWhenParsingSections;
    // TODO: const bool useOldTaskSyntaxAsTags;
    // TODO: colors?

    static const std::string DEFAULT_CONFIG_LOCATION;

    static const std::string DEFAULT_LOG_DIR_PATH;
    static const std::string DEFAULT_LOG_FILENAME_FORMAT;
    static const bool DEFAULT_SATURDAY_START;
    static const bool DEFAULT_IGNORE_FIRST_LINE_WHEN_PARSING_SECTIONS;

    static Config make(const FileReader &fileReader, const class ArgParser &);
};

class ArgParser {
    int argc;
    const char **argv;

  public:
    ArgParser() : argc{0}, argv{nullptr} {}
    ArgParser(int argc, const char **argv) : argc{argc}, argv{argv} {}

    bool has(const std::string &opt) const {
        auto it = std::find(argv, argv + argc, opt);
        return it < argv + argc;
    }

    bool has(const std::string &opt, const std::string &optLong) const {
        return has(opt) || has(optLong);
    }

    // TODO simplify
    std::optional<std::string> get(const std::string &opt, const std::string &optLong) const {
        auto it =
            std::find_if(argv, argv + argc, [&](auto arg) { return arg == opt || arg == optLong; });
        if (it < (argv + argc)) {
            if ((it + 1) < (argv + argc))
                return std::string{*(it + 1)};
            else
                throw std::runtime_error{opt + std::string{" - has no value set"}};
        } else {
            throw std::runtime_error{opt + std::string{" - has no value set"}};
        }
    }

    std::optional<std::string> getIfHas(const std::string &opt) const {
        auto it = std::find(argv, argv + argc, opt);
        if (it < (argv + argc)) {
            if ((it + 1) < (argv + argc))
                return std::string{*(it + 1)};
            else
                throw std::runtime_error{opt + std::string{" - has no value set"}};
        } else {
            return {};
        }
    }

    std::optional<std::string> getIfHas(const std::string &opt, const std::string &optLong) const {
        auto it =
            std::find_if(argv, argv + argc, [&](auto arg) { return arg == opt || arg == optLong; });
        if (it < (argv + argc)) {
            if ((it + 1) < (argv + argc))
                return std::string{*(it + 1)};
            else
                throw std::runtime_error{opt + std::string{" - has no value set"}};
        } else {
            return {};
        }
    }
};

inline std::string helpString() {
    // TODO: embed version
    // clang-format-off
    static const std::string HELP_STRING{R"(
clog (Captains Log)
A small TUI journaling tool.

 -h --help                     - show this message
 -c --config <path             - override the default config file path (~/.clog/config.ini)
 --log-dir-path <path>         - path where log files are stored (default: ~/.clog/day/)
 --log-name-format <format>    - format in which log entry markdown files are saved (default: d%Y_%m_%d.md)
 --sunday-start                - have the calendar display sunday as first day of the week)"};
    // clang-format-on
    return HELP_STRING;
}

} // namespace clog
