#include "view/annual_view_layout_base.hpp"
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>

#include <boost/program_options.hpp>

namespace caps_log {

using FileReader = std::function<std::unique_ptr<std::istream>(std::string)>;

struct GitRepoConfig {
    std::filesystem::path root;
    std::filesystem::path sshKeyPath;
    std::filesystem::path sshPubKeyPath;
    std::string mainBranchName = "master";
    std::string remoteName = "origin";
};

class ConfigParsingException : public std::runtime_error {
  public:
    explicit ConfigParsingException(const std::string &what) : std::runtime_error{what} {}
};

struct Config {
    static Config make(const FileReader &fileReader,
                       const boost::program_options::variables_map &cmdLineArgs);

    static const std::string kDefaultConfigLocation;
    static const std::string kDefaultLogDirPath;
    // TODO: make this configurable, or not?
    static const std::string kDefaultScratchpadFolderName;
    static const std::string kDefaultLogFilenameFormat;
    static const bool kDefaultSundayStart;
    static const bool kDefaultIgnoreFirstLineWhenParsingSections;
    static const unsigned kDefaultRecentEventsWindow = 14;

    std::filesystem::path logDirPath = kDefaultLogDirPath;
    std::filesystem::path logFilenameFormat = kDefaultLogFilenameFormat;
    bool sundayStart = kDefaultSundayStart;
    bool ignoreFirstLineWhenParsingSections = kDefaultIgnoreFirstLineWhenParsingSections;
    std::string password;

    std::optional<GitRepoConfig> repoConfig;

    unsigned recentEventsWindow = kDefaultRecentEventsWindow;
    view::CalendarEvents calendarEvents;
};

boost::program_options::variables_map parseCLIOptions(std::span<const char *> argv);

} // namespace caps_log
