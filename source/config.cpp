#include "config.hpp"

#include <boost/program_options/config.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <fstream>

namespace clog {

const std::string Config::DEFAULT_CONFIG_LOCATION =
    std::getenv("HOME") + std::string{"/.clog/config.ini"};
const std::string Config::DEFAULT_LOG_DIR_PATH = std::getenv("HOME") + std::string{"/.clog/day"};
const std::string Config::DEFAULT_LOG_FILENAME_FORMAT = "d%Y_%m_%d.md";
const bool Config::DEFAULT_SATURDAY_START = false;

namespace {
Config applyCommandlineOverrides(const Config &config, const ArgParser &commandLineArgs) {
    return Config{
        .logDirPath = commandLineArgs.getIfHas("--log-dir-path").value_or(config.logDirPath),
        .logFilenameFormat =
            commandLineArgs.getIfHas("--log-filename-format").value_or(config.logFilenameFormat),
        .sundayStart = commandLineArgs.has("--sunday-start") ? true : config.sundayStart,
        .ignoreFirstLineWhenParsingSections = commandLineArgs.has("--ignore-first-line-section")
                                                  ? true
                                                  : config.ignoreFirstLineWhenParsingSections};
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
            ptree.get_optional<bool>("ignore-first-line-section")
                .value_or(config.ignoreFirstLineWhenParsingSections),
    };
}

} // namespace

Config Config::make(const FileReader &fileReader, const ArgParser &cmdLineArgs) {
    auto selectedConfigFilePath =
        cmdLineArgs.getIfHas("-c", "--config").value_or(Config::DEFAULT_CONFIG_LOCATION);
    auto configFile = fileReader(selectedConfigFilePath);

    Config defaultConfig{
        .logDirPath = (Config::DEFAULT_LOG_DIR_PATH),
        .logFilenameFormat = (Config::DEFAULT_LOG_FILENAME_FORMAT),
        .sundayStart = (Config::DEFAULT_SATURDAY_START),
        .ignoreFirstLineWhenParsingSections = (Config::DEFAULT_SATURDAY_START),
    };

    auto config = configFile ? applyConfigFileOverrides(defaultConfig, *configFile) : defaultConfig;

    return applyCommandlineOverrides(config, cmdLineArgs);
}

} // namespace clog
