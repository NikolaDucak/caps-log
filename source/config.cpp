#include "config.hpp"

#include <boost/program_options/config.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace caps_log {

const std::string Config::DEFAULT_CONFIG_LOCATION =
    std::getenv("HOME") + std::string{"/.caps-log/config.ini"};
const std::string Config::DEFAULT_LOG_DIR_PATH =
    std::getenv("HOME") + std::string{"/.caps-log/day"};
const std::string Config::DEFAULT_LOG_FILENAME_FORMAT = "d%Y_%m_%d.md";
const bool Config::DEFAULT_SUNDAY_START = false;
const bool Config::DEFAULT_IGNORE_FIRST_LINE_WHEN_PARSING_SECTIONS = true;

namespace {
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
void applyConfigFileOverrides(Config &config, std::istream &istream) {
    boost::property_tree::ptree ptree;
    boost::property_tree::ini_parser::read_ini(istream, ptree);

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

} // namespace

Config Config::make(const FileReader &fileReader,
                    const boost::program_options::variables_map &cmdLineArgs) {
    auto selectedConfigFilePath = cmdLineArgs.count("config") != 0U
                                      ? cmdLineArgs["config"].as<std::string>()
                                      : Config::DEFAULT_CONFIG_LOCATION;

    auto config = Config{};

    if (auto configFile = fileReader(selectedConfigFilePath)) {
        applyConfigFileOverrides(config, *configFile);
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
        std::cout << "Capstains Log (caps-log)! A CLI journalig tool." << std::endl;
        std::cout << "Version: " << CAPS_LOG_VERSION << std::endl;
        std::cout << desc;
        exit(0);
    }

    return vmap;
}

} // namespace caps_log
