#include "config.hpp"

#include <boost/program_options/config.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <iostream>

namespace caps_log {

const std::string Config::kDefaultConfigLocation =
    std::getenv("HOME") + std::string{"/.caps-log/config.ini"};
const std::string Config::kDefaultLogDirPath = std::getenv("HOME") + std::string{"/.caps-log/day"};
const std::string Config::kDefaultLogFilenameFormat = "d%Y_%m_%d.md";
const bool Config::kDefaultSundayStart = false;
const bool Config::kDefaultIgnoreFirstLineWhenParsingSections = true;

namespace {

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
    auto nParent = removeTrailingSlash(parent.lexically_normal());
    auto nChild = removeTrailingSlash(child.lexically_normal());
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

// Your existing function with modifications to use Boost Program Options' variables_map
void applyCommandlineOverrides(Config &config,
                               const boost::program_options::variables_map &commandLineArgs) {

    if (not commandLineArgs["log-dir-path"].defaulted()) {
        config.logDirPath = commandLineArgs["log-dir-path"].as<std::string>();
    }
    if (not commandLineArgs["log-name-format"].defaulted()) {
        config.logFilenameFormat = commandLineArgs["log-name-format"].as<std::string>();
    }
    if (commandLineArgs.count("sunday-start") != 0U) {
        config.sundayStart = true;
    }
    if (commandLineArgs.count("first-line-section") != 0U) {
        config.ignoreFirstLineWhenParsingSections = true;
    }
    if (commandLineArgs.count("password") != 0U) {
        config.password = commandLineArgs["password"].as<std::string>();
    }
}

// Refactored function that modifies the passed config reference
void applyConfigFileOverrides(Config &config, boost::property_tree::ptree &ptree) {
    // Update the config object with values from the file if they are present
    auto logDirPathOpt = ptree.get_optional<std::string>("log-dir-path");
    if (logDirPathOpt) {
        config.logDirPath = logDirPathOpt.get();
    }

    auto logFilenameFormatOpt = ptree.get_optional<std::string>("log-filename-format");
    if (logFilenameFormatOpt) {
        config.logFilenameFormat = logFilenameFormatOpt.get();
    }

    auto sundayStartOpt = ptree.get_optional<bool>("sunday-start");
    if (sundayStartOpt) {
        config.sundayStart = sundayStartOpt.get();
    }

    auto ignoreFirstLineWhenParsingSectionsOpt = ptree.get_optional<bool>("first-line-section");
    if (ignoreFirstLineWhenParsingSectionsOpt) {
        config.ignoreFirstLineWhenParsingSections = ignoreFirstLineWhenParsingSectionsOpt.get();
    }

    auto passwordOpt = ptree.get_optional<std::string>("password");
    if (passwordOpt) {
        config.password = passwordOpt.get();
    }
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

        config.repoConfig = gitConf;
    }
}

} // namespace

Config Config::make(const FileReader &fileReader,
                    const boost::program_options::variables_map &cmdLineArgs) {
    auto selectedConfigFilePath = cmdLineArgs.count("config") != 0U
                                      ? cmdLineArgs["config"].as<std::string>()
                                      : Config::kDefaultConfigLocation;

    auto config = Config{};

    if (auto configFile = fileReader(selectedConfigFilePath)) {
        boost::property_tree::ptree ptree;
        boost::property_tree::ini_parser::read_ini(*configFile, ptree);

        applyConfigFileOverrides(config, ptree);
        applyGitConfigIfEnabled(config, ptree);
    }

    applyCommandlineOverrides(config, cmdLineArgs);

    return config;
}

boost::program_options::variables_map parseCLIOptions(int argc, const char **argv) {
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
      ("password", po::value<std::string>(), "password for encrypted log repositores or to be used with --encrypt/--decrypt")
      ("encrypt", "apply encryption to all logs in log dir path (needs --password)")
      ("decrypt", "apply decryption to all logs in log dir path (needs --password)");
    // clang-format on

    po::variables_map vmap;
    po::store(po::parse_command_line(argc, argv, desc), vmap);
    po::notify(vmap);

    if (vmap.count("help") != 0U) {
        std::cout << "Capstains Log (caps-log)! A CLI journalig tool." << '\n';
        std::cout << "Version: " << CAPS_LOG_VERSION_STRING << std::endl;
        std::cout << desc;
        std::cout.flush();
        exit(0);
    }

    return vmap;
}

} // namespace caps_log
