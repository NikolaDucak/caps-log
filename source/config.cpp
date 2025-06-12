#include "config.hpp"
#include "log/log_repository_crypto_applier.hpp"
#include "view/view.hpp"
#include <boost/program_options/config.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <fmt/format.h>
#include <iostream>

namespace caps_log {
using boost::program_options::variables_map;

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

view::CalendarEvents parseCalendarEvents(const boost::property_tree::ptree &ptree) {
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
            throw ConfigParsingException("Invalid calendar event key: " + key);
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
            throw ConfigParsingException("Invalid date format: " + dateStr);
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
void setIfValue(const boost::property_tree::ptree &ptree, const std::string &key, U &destination) {
    if (const auto value = ptree.get_optional<T>(key)) {
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

variables_map parseCLIOptions(const std::vector<std::string> &argv) {
    namespace po = boost::program_options;

    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
      ("help,h", "show this message")
      ("config,c", po::value<std::string>(), ("override the default config file path (" + Configuration::kDefaultConfigLocation + ")").c_str())
      ("log-dir-path", po::value<std::string>(), "path where log files are stored")
      ("log-name-format", po::value<std::string>(), "format in which log entry markdown files are saved")
      ("sunday-start", "have the calendar display sunday as first day of the week")
      ("first-line-section", "override the default behaviour of ignoring sections (lines starting with `#`) in the first line of a log entry file")
      ("password", po::value<std::string>(), "password for encrypted log repositories or to be used with --encrypt/--decrypt")
      ("encrypt", "apply encryption to all logs in log dir path (needs --password)")
      ("decrypt", "apply decryption to all logs in log dir path (needs --password)");
    // clang-format on

    std::vector<const char *> args;
    args.reserve(argv.size());
    for (const auto &arg : argv) {
        args.push_back(arg.c_str());
    }

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

} // namespace

const std::string Configuration::kDefaultConfigLocation =
    std::getenv("HOME") + std::string{"/.caps-log/config.ini"};
const std::string Configuration::kDefaultLogDirPath =
    std::getenv("HOME") + std::string{"/.caps-log/day"};
const std::string Configuration::kDefaultScratchpadFolderName = "scratchpads";
const std::string Configuration::kDefaultLogFilenameFormat = "d%Y_%m_%d.md";
const bool Configuration::kDefaultSundayStart = false;
const bool Configuration::kDefaultAcceptSectionsOnFirstLine = false;

std::function<std::string(const std::filesystem::path &)> Configuration::makeDefaultReadFileFunc() {
    return [](const std::filesystem::path &path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw ConfigParsingException{"Failed to open file: " + path.string()};
        }
        return std::string((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    };
}

Configuration::Configuration(
    const std::vector<std::string> &cliArgs,
    const std::function<std::string(const std::filesystem::path &)> &readFileFunc) {

    applyDefaults();
    boost::program_options::variables_map vmap = parseCLIOptions(cliArgs);

    m_configFilePath =
        vmap.contains("config") ? vmap["config"].as<std::string>() : kDefaultConfigLocation;

    auto content = readFileFunc(m_configFilePath);

    boost::property_tree::ptree ptree;
    try {
        std::istringstream iss(content);
        boost::property_tree::read_ini(iss, ptree);
    } catch (const boost::property_tree::ini_parser_error &e) {
        throw ConfigParsingException{"Failed to parse configuration file: " +
                                     std::string(e.what())};
    }
    overrideFromConfigFile(ptree);
    overrideFromCommandLine(vmap);
}

void Configuration::applyDefaults() {
    m_viewConfig = view::ViewConfig{
        .sundayStart = Configuration::kDefaultSundayStart,
        .recentEventsWindow = Configuration::kDefaultRecentEventsWindow,
    };
    m_password = "";
    m_cryptoApplicationType = std::nullopt;
    m_gitRepoConfig = std::nullopt;
    m_cryptoEnabled = false;
    m_acceptSectionsOnFirstLine = Configuration::kDefaultAcceptSectionsOnFirstLine;
    m_logDirPath = expandTilde(Configuration::kDefaultLogDirPath);
    m_logFilenameFormat = Configuration::kDefaultLogFilenameFormat;
    m_calendarEvents = view::CalendarEvents{};
}

void Configuration::overrideFromConfigFile(const boost::property_tree::ptree &ptree) {
    setIfValue<std::string>(ptree, "log-dir-path", m_logDirPath);
    setIfValue<std::string>(ptree, "log-filename-format", m_logFilenameFormat);
    setIfValue<bool>(ptree, "sunday-start", m_viewConfig.sundayStart);
    setIfValue<bool>(ptree, "first-line-section", m_acceptSectionsOnFirstLine);
    setIfValue<std::string>(ptree, "password", m_password);
    setIfValue<unsigned>(ptree, "calendar-events.recent-events-window",
                         m_viewConfig.recentEventsWindow);

    // Parse and update calendar events
    m_calendarEvents = parseCalendarEvents(ptree);

    // sanitize log dir path
    m_logDirPath = expandTilde(m_logDirPath);

    if (ptree.get_optional<bool>("git.enable-git-log-repo").value_or(false)) {
        utils::GitRepoConfig gitConf;
        setIfValue<std::string>(ptree, "git.ssh-pub-key-path", gitConf.sshPubKeyPath);
        setIfValue<std::string>(ptree, "git.ssh-key-path", gitConf.sshKeyPath);
        setIfValue<std::string>(ptree, "git.main-branch-name", gitConf.mainBranchName);
        setIfValue<std::string>(ptree, "git.remote-name", gitConf.remoteName);
        setIfValue<std::string>(ptree, "git.repo-root", gitConf.root);

        // sanitize paths
        gitConf.sshKeyPath = expandTilde(gitConf.sshKeyPath);
        gitConf.sshPubKeyPath = expandTilde(gitConf.sshPubKeyPath);

        m_gitRepoConfig = gitConf;
    }
}

void Configuration::overrideFromCommandLine(const boost::program_options::variables_map &vmap) {
    if (vmap.contains("log-dir-path")) {
        m_logDirPath = expandTilde(vmap["log-dir-path"].as<std::string>());
    }
    if (vmap.contains("log-name-format")) {
        m_logFilenameFormat = vmap["log-name-format"].as<std::string>();
    }
    if (vmap.contains("sunday-start")) {
        m_viewConfig.sundayStart = true;
    }
    if (vmap.contains("first-line-section")) {
        m_acceptSectionsOnFirstLine = true;
    }
    if (vmap.contains("password")) {
        m_password = vmap["password"].as<std::string>();
    }

    if (vmap.contains("encrypt")) {
        if (m_password.empty()) {
            throw ConfigParsingException{"Password must be provided when encrypting logs!"};
        }
        m_cryptoEnabled = true;
        m_cryptoApplicationType = Crypto::Encrypt;
    } else if (vmap.contains("decrypt")) {
        if (m_password.empty()) {
            throw ConfigParsingException{"Password must be provided when decrypting logs!"};
        }
        m_cryptoEnabled = true;
        m_cryptoApplicationType = Crypto::Decrypt;
    }
}

void Configuration::verify() const {
    if (m_logDirPath.empty()) {
        throw ConfigParsingException{"Log directory path can not be empty!"};
    }
    if (m_logFilenameFormat.empty()) {
        throw ConfigParsingException{"Log filename format can not be empty!"};
    }
    if (m_cryptoEnabled && m_password.empty()) {
        throw ConfigParsingException{"Password must be provided when encryption is enabled!"};
    }
    if (m_gitRepoConfig.has_value()) {
        const auto &gitConf = m_gitRepoConfig.value();
        if (gitConf.sshKeyPath.empty()) {
            throw ConfigParsingException{"SSH key path can not be empty!"};
        }
        if (gitConf.sshPubKeyPath.empty()) {
            throw ConfigParsingException{"SSH public key path can not be empty!"};
        }
        if (gitConf.mainBranchName.empty()) {
            throw ConfigParsingException{"Main branch name can not be empty!"};
        }
        if (gitConf.remoteName.empty()) {
            throw ConfigParsingException{"Remote name can not be empty!"};
        }
        if (gitConf.root.empty()) {
            throw ConfigParsingException{"Git repository root can not be empty!"};
        }

        if (not isParentOf(gitConf.root, m_logDirPath) && gitConf.root != m_logDirPath) {
            throw ConfigParsingException{"Error! Git repo root (" + gitConf.root.string() +
                                         ") is not parent of log dir (" + m_logDirPath + ")!"};
        }

        // expect that  sshKeyPath, sshPubKeyPath, root exist
        if (not std::filesystem::exists(gitConf.sshKeyPath)) {
            throw ConfigParsingException{"SSH key does not exist: " + gitConf.sshKeyPath.string()};
        }
        if (not std::filesystem::exists(gitConf.sshPubKeyPath)) {
            throw ConfigParsingException{"SSH public key does not exist: " +
                                         gitConf.sshPubKeyPath.string()};
        }
        if (not std::filesystem::exists(gitConf.root)) {
            throw ConfigParsingException{"Git repository root does not exist: " +
                                         gitConf.root.string()};
        }
    }
}

[[nodiscard]] const view::ViewConfig &Configuration::getViewConfig() const { return m_viewConfig; }

[[nodiscard]] const std::optional<utils::GitRepoConfig> &Configuration::getGitRepoConfig() const {
    return m_gitRepoConfig;
}

[[nodiscard]] bool Configuration::isCryptoEnabled() const { return m_cryptoEnabled; }

[[nodiscard]] bool Configuration::shouldRunApplication() const {
    return not m_cryptoApplicationType.has_value();
}

[[nodiscard]] std::string Configuration::getLogDirPath() const { return m_logDirPath; }

[[nodiscard]] std::string Configuration::getPassword() const { return m_password; }

[[nodiscard]] std::string Configuration::getLogFilenameFormat() const {
    return m_logFilenameFormat;
}

[[nodiscard]] log::LocalFSLogFilePathProvider Configuration::getLogFilePathProvider() const {
    return log::LocalFSLogFilePathProvider{m_logDirPath, m_logFilenameFormat};
}

[[nodiscard]] std::string Configuration::getScratchpadDirPath() const {
    return m_logDirPath + '/' + Configuration::kDefaultScratchpadFolderName;
}

[[nodiscard]] bool Configuration::isPasswordProvided() const { return !m_password.empty(); }

[[nodiscard]] std::optional<Crypto> Configuration::getCryptoApplicationType() const {
    return m_cryptoApplicationType;
}

[[nodiscard]] std::filesystem::path Configuration::getConfigFilePath() const {
    return m_configFilePath;
}

[[nodiscard]] AppConfig Configuration::getAppConfig() const {
    return AppConfig{
        // TODO: align
        .skipFirstLine = !m_acceptSectionsOnFirstLine,
        .events = m_calendarEvents,
    };
}
} // namespace caps_log
