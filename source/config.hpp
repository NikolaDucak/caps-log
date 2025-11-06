#pragma once

#include "app.hpp"
#include "log/local_log_repository.hpp"
#include "log/log_repository_crypto_applier.hpp"
#include "utils/git_repo.hpp"
#include "view/annual_view_layout_base.hpp"
#include "view/view.hpp"
#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <optional>
#include <string>

#include <boost/program_options.hpp>

namespace caps_log {

class ConfigParsingException : public std::runtime_error {
  public:
    explicit ConfigParsingException(const std::string &what) : std::runtime_error{what} {}
};

class Configuration {
  public:
    static const std::string kDefaultConfigLocation;
    static const std::string kDefaultLogDirPath;
    static const std::string kDefaultScratchpadFolderName;
    static const std::string kDefaultLogFilenameFormat;
    static const bool kDefaultSundayStart;
    static const bool kDefaultAcceptSectionsOnFirstLine;
    static const unsigned kDefaultRecentEventsWindow = 14;
    static std::function<std::string(const std::filesystem::path &)> makeDefaultReadFileFunc();

    Configuration(const std::vector<std::string> &cliArgs,
                  const std::function<std::string(const std::filesystem::path &)> &readFileFunc =
                      makeDefaultReadFileFunc());

    [[nodiscard]] const view::ViewConfig &getViewConfig() const;

    [[nodiscard]] const std::optional<utils::GitRepoConfig> &getGitRepoConfig() const;

    [[nodiscard]] bool shouldRunApplication() const;

    [[nodiscard]] std::string getLogDirPath() const;

    [[nodiscard]] std::string getPassword() const;

    [[nodiscard]] std::string getLogFilenameFormat() const;

    [[nodiscard]] log::LocalFSLogFilePathProvider getLogFilePathProvider() const;

    [[nodiscard]] std::string getScratchpadDirPath() const;

    [[nodiscard]] bool isPasswordProvided() const;

    [[nodiscard]] std::optional<Crypto> getCryptoApplicationType() const;

    void setPassword(const std::string &password) { m_password = password; }

    [[nodiscard]] AppConfig getAppConfig() const;

    [[nodiscard]] std::filesystem::path getConfigFilePath() const;

    void verify() const;

  private:
    bool m_cryptoTaskEnabled{};
    view::ViewConfig m_viewConfig{};
    std::string m_password;
    std::optional<utils::GitRepoConfig> m_gitRepoConfig;
    std::string m_logDirPath;
    std::string m_logFilenameFormat;
    std::optional<Crypto> m_cryptoApplicationType;
    bool m_acceptSectionsOnFirstLine{};
    view::CalendarEvents m_calendarEvents;
    std::filesystem::path m_configFilePath;
    void applyDefaults();
    void overrideFromConfigFile(const boost::property_tree::ptree &ptree);
    void overrideFromCommandLine(const boost::program_options::variables_map &vmap);
};

} // namespace caps_log
