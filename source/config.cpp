#include "config.hpp"

#include <boost/program_options/config.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <fmt/format.h>
#include <iostream>

namespace caps_log {

const std::string Config::kDefaultConfigLocation =
    std::getenv("HOME") + std::string{"/.caps-log/config.ini"};
const std::string Config::kDefaultLogDirPath = std::getenv("HOME") + std::string{"/.caps-log/day"};
const std::string Config::kDefaultLogFilenameFormat = "d%Y_%m_%d.md";
const bool Config::kDefaultSundayStart = false;
const bool Config::kDefaultIgnoreFirstLineWhenParsingSections = true;

namespace {

std::filesystem::path expandTilde(const std::filesystem::path &input) {
    std::string str = input.string();

    if (!str.empty() && str[0] == '~') {
        const char *home = std::getenv("HOME");
        if (home == nullptr) {
#ifdef _WIN32
            home = std::getenv("USERPROFILE");
#endif
        }
        if (home != nullptr) {
            // drop `~/` (note: must drop '/', otherwise it is an absolute path and the path
            // concatenation wont work)
            return std::filesystem::path(home) / str.substr(2);
        }
    }
    return input;
}

std::optional<std::chrono::month_day> parseDate(const std::string &date_str) {
    int day = 0;
    int month = 0;
    char dot1 = 0;
    char dot2 = 0;

    std::istringstream iss(date_str);
    static constexpr auto kMaxMonths = 12;
    static constexpr auto kMaxDays = 31;
    if (iss >> std::setw(2) >> day >> dot1 >> std::setw(2) >> month >> dot2 && dot1 == '.' &&
        dot2 == '.' && day >= 1 && day <= kMaxDays && month >= 1 && month <= kMaxMonths) {
        return std::chrono::month_day{std::chrono::month(month), std::chrono::day(day)};
    }
    return std::nullopt; // Return empty optional if parsing fails
}

view::CalendarEvents parseCalendarEvents(boost::property_tree::ptree &ptree) {
    // CalendarEvents structure to store all events
    view::CalendarEvents calendarEvents;

    // Iterate over each section in the property tree
    for (const auto &section : ptree) {
        const std::string &key = section.first; // e.g., "calendar-events.birthdays.0"
        const boost::property_tree::ptree &data = section.second;

        // Check if the key starts with "calendar-events."
        if (key.rfind("calendar-events.", 0) != 0) {
            // Not a calendar event section, skip it
            continue;
        }

        // Split the key to extract event type
        std::stringstream sstream(key);
        std::string item;
        std::vector<std::string> tokens;
        while (std::getline(sstream, item, '.')) {
            tokens.push_back(item);
        }

        if (tokens.size() != 3) {
            throw std::runtime_error("Invalid calendar event key: " + key);
        }

        // Extract the category (e.g., "birthdays", "holidays", "anniversaries")
        std::string category = tokens[1];

        // Get the name and date from the section
        auto name = data.get<std::string>("name");
        auto dateStr = data.get<std::string>("date");

        // Parse the date string into day and month
        int day{};
        int month{};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        if (sscanf(dateStr.c_str(), "%02d.%02d.", &day, &month) != 2) {
            throw std::runtime_error("Invalid date format: " + dateStr);
        }

        // Create a month_day object
        std::chrono::month_day date{std::chrono::month{static_cast<unsigned>(month)},
                                    std::chrono::day{static_cast<unsigned>(day)}};

        // Create a CalendarEvent and insert it into the map
        view::CalendarEvent event{name, date};
        calendarEvents[category].insert(event);
    }

    return calendarEvents;
}

std::filesystem::path removeTrailingSlash(const std::filesystem::path &path) {
    std::string pathStr = path.string();
    if (!pathStr.empty() && pathStr.back() == '/') {
        pathStr.pop_back();
    }
    return pathStr;
}

template <class T, class U>
void setIfValue(boost::property_tree::ptree &ptree, const std::string &key, U &destination) {
    if (auto value = ptree.get_optional<T>(key)) {
        destination = value.get();
    }
}

bool isParentOf(const std::filesystem::path &parent, const std::filesystem::path &child) {
    const auto nParent = removeTrailingSlash(parent.lexically_normal());
    const auto nChild = removeTrailingSlash(child.lexically_normal());
    auto parentIter = nParent.begin();
    auto childIter = nChild.begin();

    while (parentIter != nParent.end() && childIter != nChild.end()) {
        if (*parentIter != *childIter) {
            return false;
        }
        ++parentIter;
        ++childIter;
    }

    return parentIter == nParent.end() && childIter != nChild.end();
}

void applyCommandlineOverrides(Config &config,
                               const boost::program_options::variables_map &commandLineArgs) {

    if (not commandLineArgs["log-dir-path"].defaulted()) {
        config.logDirPath = expandTilde(commandLineArgs["log-dir-path"].as<std::string>());
    }
    if (not commandLineArgs["log-name-format"].defaulted()) {
        config.logFilenameFormat = commandLineArgs["log-name-format"].as<std::string>();
    }
    if (commandLineArgs.contains("sunday-start")) {
        config.sundayStart = true;
    }
    if (commandLineArgs.contains("first-line-section")) {
        config.ignoreFirstLineWhenParsingSections = true;
    }
    if (commandLineArgs.contains("password")) {
        config.password = commandLineArgs["password"].as<std::string>();
    }
}

void applyConfigFileOverrides(Config &config, boost::property_tree::ptree &ptree) {
    setIfValue<std::string>(ptree, "log-dir-path", config.logDirPath);
    setIfValue<std::string>(ptree, "log-filename-format", config.logFilenameFormat);
    setIfValue<bool>(ptree, "sunday-start", config.sundayStart);
    setIfValue<bool>(ptree, "first-line-section", config.ignoreFirstLineWhenParsingSections);
    setIfValue<std::string>(ptree, "password", config.password);
    setIfValue<unsigned>(ptree, "calendar-events.recent-events-window", config.recentEventsWindow);

    // Parse and update calendar events
    config.calendarEvents = parseCalendarEvents(ptree);

    // sanitize log dir path
    config.logDirPath = expandTilde(config.logDirPath);
}

void applyGitConfigIfEnabled(Config &config, boost::property_tree::ptree &ptree) {
    if (ptree.get_optional<bool>("git.enable-git-log-repo").value_or(false)) {
        GitRepoConfig gitConf;
        setIfValue<std::string>(ptree, "git.ssh-pub-key-path", gitConf.sshPubKeyPath);
        setIfValue<std::string>(ptree, "git.ssh-key-path", gitConf.sshKeyPath);
        setIfValue<std::string>(ptree, "git.main-branch-name", gitConf.mainBranchName);
        setIfValue<std::string>(ptree, "git.remote-name", gitConf.remoteName);
        setIfValue<std::string>(ptree, "git.repo-root", gitConf.root);

        if (not isParentOf(gitConf.root, config.logDirPath) && gitConf.root != config.logDirPath) {
            throw std::invalid_argument{"Error! Git repo root (" + gitConf.root.string() +
                                        ") is not parent of log dir (" +
                                        config.logDirPath.string() + ")!"};
        }

        // sanitize paths
        gitConf.sshKeyPath = expandTilde(gitConf.sshKeyPath);
        gitConf.sshPubKeyPath = expandTilde(gitConf.sshPubKeyPath);

        config.repoConfig = gitConf;
    }
}

} // namespace

Config Config::make(const FileReader &fileReader,
                    const boost::program_options::variables_map &cmdLineArgs) {
    auto selectedConfigFilePath = cmdLineArgs.contains("config")
                                      ? cmdLineArgs["config"].as<std::string>()
                                      : Config::kDefaultConfigLocation;
    try {
        auto config = Config{};

        if (auto configFile = fileReader(selectedConfigFilePath)) {
            boost::property_tree::ptree ptree;
            boost::property_tree::ini_parser::read_ini(*configFile, ptree);

            applyConfigFileOverrides(config, ptree);
            applyCommandlineOverrides(config, cmdLineArgs);
            applyGitConfigIfEnabled(config, ptree);
        } else {
            applyCommandlineOverrides(config, cmdLineArgs);
        }

        return config;
    } catch (const std::exception &e) {
        throw ConfigParsingException{
            fmt::format("Error parsing config file ({}): {}", selectedConfigFilePath, e.what())};
    }
}

boost::program_options::variables_map parseCLIOptions(std::span<const char *> args) {
    namespace po = boost::program_options;

    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
      ("help,h", "show this message")
      ("config,c", po::value<std::string>(), "override the default config file path (~/.caps-log/config.ini)")
      ("log-dir-path", po::value<std::string>()->default_value("~/.caps-log/day/"), "path where log files are stored")
      ("log-name-format", po::value<std::string>()->default_value("d%Y_%m_%d.md"), "format in which log entry markdown files are saved")
      ("sunday-start", "have the calendar display sunday as first day of the week")
      ("first-line-section", "if a section mark is placed on the first line, by default it is ignored as it's left for log title, this overrides this behaviour")
      ("password", po::value<std::string>(), "password for encrypted log repositories or to be used with --encrypt/--decrypt")
      ("encrypt", "apply encryption to all logs in log dir path (needs --password)")
      ("decrypt", "apply decryption to all logs in log dir path (needs --password)");
    // clang-format on

    po::variables_map vmap;
    po::store(po::parse_command_line(static_cast<int>(args.size()), args.data(), desc), vmap);
    po::notify(vmap);

    if (vmap.contains("help")) {
        std::cout << "Captain's Log (caps-log)! A CLI journalig tool." << '\n';
        std::cout << "Version: " << CAPS_LOG_VERSION_STRING << '\n';
        std::cout << desc;
        std::cout << std::flush;
        exit(0);
    }

    return vmap;
}

} // namespace caps_log
