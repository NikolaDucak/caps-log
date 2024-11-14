#include "view/annual_view_base.hpp"
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
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

struct Config {
    static Config make(const FileReader &fileReader,
                       const boost::program_options::variables_map &cmdLineArgs);

    static const std::string kDefaultConfigLocation;
    static const std::string kDefaultLogDirPath;
    static const std::string kDefaultLogFilenameFormat;
    static const bool kDefaultSundayStart;
    static const bool kDefaultIgnoreFirstLineWhenParsingSections;

    std::filesystem::path logDirPath = kDefaultLogDirPath;
    std::filesystem::path logFilenameFormat = kDefaultLogFilenameFormat;
    bool sundayStart = kDefaultSundayStart;
    bool ignoreFirstLineWhenParsingSections = kDefaultIgnoreFirstLineWhenParsingSections;
    std::string password;

    std::optional<GitRepoConfig> repoConfig;

    unsigned recentEventsWindow = 14;
    view::CalendarEvents calendarEvents;
};

// NOLINTNEXTLINE
boost::program_options::variables_map parseCLIOptions(int argc, const char *argv[]);

} // namespace caps_log
