#pragma once

#include "caps_log.hpp"
#include "config.hpp"
#include "editor/editor_base.hpp"
#include "log/local_log_repository.hpp"

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <fstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <regex>

namespace caps_log::test::e2e {

// When enabled, rendered elements that are checked agains the expected renders in data files
// are refreshed. This is useful when the expected renders change.
constexpr static const bool kRefreshDataFiles = false;

static const std::filesystem::path kTestDataDirectory =
    std::filesystem::path{CAPS_LOG_TEST_DATA_DIR} / "e2e";

static const std::filesystem::path kTestDirectoryBase =
    std::filesystem::current_path() / "test_dir_base";
static const std::filesystem::path kTestLogDirectory = kTestDirectoryBase / "test_dir";

static const std::filesystem::path kTestConfigPath = kTestLogDirectory / "caps_log_test_config.ini";

inline std::filesystem::path getTestScratchpadDirectoryName() {
    return kTestLogDirectory / Configuration::kDefaultScratchpadFolderName;
}

namespace detail {
static bool isRenderedElementEqual(std::string element, const std::string &fileName,
                                   testing::MatchResultListener *result_listener,
                                   bool includeDates) {
    const auto removeDateSubstrings = [](const std::string &input) -> std::string {
        // Remove date substrings like "24-06-15" from the input string with 'xx-xx-xx' format
        return std::regex_replace(input, std::regex(R"(\d{2}-\d{2}-\d{2})"), "xx-xx-xx");
    };
    if (kRefreshDataFiles) {
        std::ofstream ofs{kTestDataDirectory / fileName};
        if (!ofs.is_open()) {
            throw std::runtime_error{
                "Failed to open file for writing: " + kTestDataDirectory.string() + "/" + fileName};
        }
        ofs << element;
        return true; // Refreshing data files, so we don't compare
    }

    std::ifstream ifs{kTestDataDirectory / fileName};
    if (!ifs.is_open()) {
        throw std::runtime_error{"Failed to open file for reading: " + kTestDataDirectory.string() +
                                 "/" + fileName};
    }
    std::string expectedContent((std::istreambuf_iterator<char>(ifs)),
                                std::istreambuf_iterator<char>());
    if (not includeDates) {
        expectedContent = removeDateSubstrings(expectedContent);
        element = removeDateSubstrings(element);
    }
    const auto areEqual = element == expectedContent;
    if (!areEqual) {
        *result_listener << "Expected content: " << expectedContent
                         << "\nActual element: " << element;
    }
    return areEqual;
}
} // namespace detail

// NOLINTNEXTLINE
MATCHER_P(RenderedElementEqual, fileName, "") {
    return detail::isRenderedElementEqual(arg, fileName, result_listener, true);
}

// NOLINTNEXTLINE
MATCHER_P(RenderedElementWithoutDatesEqual, fileName, "") {
    return detail::isRenderedElementEqual(arg, fileName, result_listener, false);
}

class FakeEditor : public editor::EditorBase {
  public:
    FakeEditor() = default;

    void setPathProvider(const log::LocalFSLogFilePathProvider &pathProvider) {
        // Simulate setting the path provider for the editor
        m_pathProvider = pathProvider;
        // Verify path provider is pointing to the kTestLogDirectory
        if (!m_pathProvider || m_pathProvider->getLogDirPath() != kTestLogDirectory) {
            throw std::runtime_error{"Path provider is not set correctly."};
        }
    }

    void openLog(const log::LogFile &log) override {
        // Simulate opening a log file in an editor
        m_openedLog = log;
        auto logPath = m_pathProvider->path(log.getDate());

        std::filesystem::create_directories(logPath.parent_path());
        std::ofstream ofs{logPath};
        if (!ofs.is_open()) {
            throw std::runtime_error{"Failed to open log file for writing: " + logPath.string()};
        }
        ofs << m_contentToBeEdited;
    }

    void openScratchpad(const std::string &scratchpadName) override {
        // Simulate opening a scratchpad file in an editor
        m_openedScratchpad = scratchpadName;
        const auto scratchpadPath = m_pathProvider->getLogDirPath() /
                                    Configuration::kDefaultScratchpadFolderName / scratchpadName;

        // if path exists, throw
        if (std::filesystem::exists(scratchpadPath)) {
            throw std::runtime_error{"Scratchpad file already exists: " + scratchpadPath.string()};
        }

        // expect the scratchpad directory to exist
        const auto scratchpadDir =
            m_pathProvider->getLogDirPath() / Configuration::kDefaultScratchpadFolderName;
        if (!std::filesystem::exists(scratchpadDir)) {
            throw std::runtime_error{"Scratchpad directory does not exist: " +
                                     scratchpadDir.string()};
        }

        // write the scratchpad content to the file
        std::ofstream ofs{scratchpadPath};
        ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        if (!ofs.is_open()) {
            // find error message
            throw std::runtime_error{"Failed to open scratchpad file for writing: " +
                                     scratchpadPath.string()};
        }
        ofs << m_contentToBeEdited;

        ofs.close();
    }

    void setContentToBeEdited(const std::string &content) {
        // Simulate setting content to be edited in the editor
        m_contentToBeEdited = content;
    }

  private:
    std::optional<log::LogFile> m_openedLog;
    std::string m_openedScratchpad;
    std::string m_contentToBeEdited;
    std::optional<log::LocalFSLogFilePathProvider> m_pathProvider;
};

class CapsLogE2ETest : public testing::Test {
  protected:
    constexpr static const ftxui::Dimensions kDefaultScreenSize{.dimx = 220, .dimy = 40};
    constexpr static const std::chrono::year_month_day kToday{
        std::chrono::year{2024}, std::chrono::June, std::chrono::day{15}};

  public:
    void SetUp() override {
        auto currentTerminalColorSupport = ftxui::Terminal::ColorSupport();
        if (currentTerminalColorSupport != ftxui::Terminal::Color::Palette256) {
            std::string currentTerminalColorSupportStr;
            switch (currentTerminalColorSupport) {
            case ftxui::Terminal::Color::Palette1:
                currentTerminalColorSupportStr = "Palette1";
                break;
            case ftxui::Terminal::Color::Palette16:
                currentTerminalColorSupportStr = "Palette16";
                break;
            case ftxui::Terminal::Color::Palette256:
                currentTerminalColorSupportStr = "Palette256";
                break;
            case ftxui::Terminal::Color::TrueColor:
                currentTerminalColorSupportStr = "TrueColor";
                break;
            }
            std::cerr << "Warning: Test requires terminal color support to be set to Palette256. "
                         "Current setting is: " +
                             currentTerminalColorSupportStr
                      << std::endl; // NOLINT
            ftxui::Terminal::SetColorSupport(ftxui::Terminal::Color::Palette256);
        }

        if (std::filesystem::exists(kTestLogDirectory)) {
            throw std::runtime_error{"Test log directory already exists: " +
                                     kTestLogDirectory.string()};
        }
    }

    void TearDown() override {
        // Clean up the test log directory after each test
        if (std::filesystem::exists(kTestLogDirectory)) {
            std::filesystem::remove_all(kTestLogDirectory);
        }
    }

    [[nodiscard]] CapsLog::Context createTestContext(std::vector<std::string> cliArgs) const {
        CapsLog::Context context;
        context.today = kToday;
        // append "--log-dir-path" and the test log directory to the CLI args
        std::vector<std::string> args(cliArgs.begin(), cliArgs.end());
        args.push_back("--log-dir-path");
        args.push_back(kTestLogDirectory.string());
        // append --config" to point to the test config file"
        args.push_back("--config");
        args.push_back(kTestConfigPath.string());
        if (not std::filesystem::exists(kTestConfigPath)) {
            writeFile(kTestConfigPath, "");
        }

        context.cliArgs = std::move(args);

        context.terminalSizeProvider = []() { return kDefaultScreenSize; };
        context.editorFactory =
            [this](const log::LocalFSLogFilePathProvider &logFilePath,
                   const std::string &scratchpadDirPath,
                   const std::string &password) -> std::shared_ptr<editor::EditorBase> {
            return m_fakeEditor; // Use the fake editor for testing
        };

        return context;
    }

    void editorWillWriteLog(const std::string &content,
                            const log::LocalFSLogFilePathProvider &logFilePathProvider) const {
        // Simulate the editor writing a log file
        m_fakeEditor->setPathProvider(logFilePathProvider);
        m_fakeEditor->setContentToBeEdited(content);
    }

    static void writeFile(const std::filesystem::path &filePath, const std::string &content) {
        assert(!filePath.empty() && filePath.is_absolute());
        if (!filePath.string().starts_with(kTestDirectoryBase.string())) {
            throw std::runtime_error{"File path is not a child of the test directory: " +
                                     filePath.string()};
        }

        // create directories if they do not exist
        std::filesystem::create_directories(filePath.parent_path());
        // Write the log content to the specified file path
        std::ofstream ofs{filePath};
        if (!ofs.is_open()) {
            throw std::runtime_error{"Failed to open file for writing: " + filePath.string()};
        }
        ofs << content;
    }

    std::shared_ptr<FakeEditor> m_fakeEditor = std::make_shared<FakeEditor>();
};
} // namespace caps_log::test::e2e
