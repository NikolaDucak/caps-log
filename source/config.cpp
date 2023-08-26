#include "config.hpp"
#include "arg_parser.hpp"

#include <boost/program_options/config.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <fstream>

namespace caps_log {

const std::string Config::DEFAULT_CONFIG_LOCATION =
    std::getenv("HOME") + std::string{"/.caps-log/config.ini"};
const std::string Config::DEFAULT_LOG_DIR_PATH = std::getenv("HOME") + std::string{"/.caps-log/day"};
const std::string Config::DEFAULT_LOG_FILENAME_FORMAT = "d%Y_%m_%d.md";
const bool Config::DEFAULT_SATURDAY_START = false;
const bool Config::DEFAULT_IGNORE_FIRST_LINE_WHEN_PARSING_SECTIONS = true;

namespace {
Config applyCommandlineOverrides(const Config &config, const ArgParser &commandLineArgs) {
    return Config{
        .logDirPath = commandLineArgs.getIfHas("--log-dir-path").value_or(config.logDirPath),
        .logFilenameFormat =
            commandLineArgs.getIfHas("--log-filename-format").value_or(config.logFilenameFormat),
        .sundayStart = commandLineArgs.has("--sunday-start") ? true : config.sundayStart,
        .ignoreFirstLineWhenParsingSections = commandLineArgs.has("--first-line-section")
                                                  ? true
                                                  : config.ignoreFirstLineWhenParsingSections,
        .password = commandLineArgs.getIfHas("--password").value_or(config.password)};
}

Config applyConfigFileOverrides(const Config &config, std::istream &istream) {
    boost::property_tree::ptree ptree;
    boost::property_tree::ini_parser::read_ini(istream, ptree);

    return Config{
        .logDirPath =
            ptree.get_optional<std::filesystem::path>("log-dir-path").value_or(config.logDirPath),
        .logFilenameFormat = ptree.get_optional<std::string>("log-filename-format")
                                 .value_or(config.logFilenameFormat),
        .sundayStart = ptree.get_optional<bool>("sunday-start").value_or(config.sundayStart),
        .ignoreFirstLineWhenParsingSections =
            ptree.get_optional<bool>("first-line-section")
                .value_or(config.ignoreFirstLineWhenParsingSections),
        .password = ptree.get_optional<std::string>("password").value_or(config.password),
    };
}

} // namespace

Config Config::make(const FileReader &fileReader, const ArgParser &cmdLineArgs) {
    auto selectedConfigFilePath =
        cmdLineArgs.getIfHas("-c", "--config").value_or(Config::DEFAULT_CONFIG_LOCATION);
    auto configFile = fileReader(selectedConfigFilePath);

    Config defaultConfig{
        .logDirPath = Config::DEFAULT_LOG_DIR_PATH,
        .logFilenameFormat = Config::DEFAULT_LOG_FILENAME_FORMAT,
        .sundayStart = Config::DEFAULT_SATURDAY_START,
        .ignoreFirstLineWhenParsingSections =
            Config::DEFAULT_IGNORE_FIRST_LINE_WHEN_PARSING_SECTIONS,
        .password = "",
    };

    auto config = configFile ? applyConfigFileOverrides(defaultConfig, *configFile) : defaultConfig;

    return applyCommandlineOverrides(config, cmdLineArgs);
}

} // namespace caps_log
