#include "config.hpp"
#include "date/date.hpp"
#include "mocks.hpp"
#include "model/local_log_repository.hpp"
#include "model/log_file.hpp"
#include "model/log_repository_base.hpp"
#include "model/year_overview_data.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <string>

using namespace caps_log::date;
using namespace caps_log::model;

namespace {
const std::string TEST_LOG_DIRECTORY{std::filesystem::temp_directory_path().string() +
                                     "caps_log_test_dir"};
std::string readFile(const std::string &path) {
    std::ifstream ifs{path};
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

} // namespace

class LocalLogRepositoryTest : public ::testing::Test {
  protected:
    LocalFSLogFilePathProvider TMPDirPathProvider{TEST_LOG_DIRECTORY,
                                                  caps_log::Config::DEFAULT_LOG_FILENAME_FORMAT};

  public:
    void SetUp() override { std::filesystem::remove_all(TMPDirPathProvider.getLogDirPath()); }
    void writeDummyLog(const Date &date, const std::string &content) {
        std::ofstream of(TMPDirPathProvider.path(date));
        of << content;
    }
};

TEST_F(LocalLogRepositoryTest, Read) {
    const auto selectedDate = Date{25, 5, 2005};
    auto repo = LocalLogRepository(TMPDirPathProvider);
    const std::string logContent = "Dummy string";

    ASSERT_FALSE(repo.read(selectedDate).has_value());
    writeDummyLog(selectedDate, logContent);

    auto log = repo.read(selectedDate);
    ASSERT_TRUE(log.has_value());
    ASSERT_EQ(log->getDate(), selectedDate);
    ASSERT_EQ(log->getContent(), logContent);
}

TEST_F(LocalLogRepositoryTest, Remove) {
    const auto date = Date{25, 5, 2005};
    auto repo = LocalLogRepository(TMPDirPathProvider);
    const std::string logContent = "Dummy string";

    writeDummyLog(date, logContent);
    ASSERT_TRUE(std::filesystem::exists(TMPDirPathProvider.path(date)));
    repo.remove(date);
    ASSERT_FALSE(std::filesystem::exists(TMPDirPathProvider.path(date)));
}

TEST_F(LocalLogRepositoryTest, Write) {
    const auto date = Date{25, 5, 2005};
    auto repo = LocalLogRepository(TMPDirPathProvider);
    const std::string logContent = "Dummy string";

    repo.write(LogFile{date, logContent});
    ASSERT_TRUE(std::filesystem::exists(TMPDirPathProvider.path(date)));
}

TEST_F(LocalLogRepositoryTest, EncryptedRead) {
    const auto selectedDate = Date{25, 5, 2005};
    const auto dummyPassword = "dummy";

    auto repo = LocalLogRepository(TMPDirPathProvider, dummyPassword);
    const std::string logContent = "Dummy string";
    const std::string encLogContent = "\x16\x1A/l\x1\x1"
                                      "7\a\x1\xEE\xD2n";

    ASSERT_FALSE(repo.read(selectedDate).has_value());
    writeDummyLog(selectedDate, encLogContent);

    auto log = repo.read(selectedDate);
    ASSERT_TRUE(log.has_value());
    ASSERT_EQ(log->getDate(), selectedDate);
    ASSERT_EQ(log->getContent(), logContent);

    EXPECT_EQ(readFile(TMPDirPathProvider.path(selectedDate)), encLogContent);
}

TEST_F(LocalLogRepositoryTest, EncryptedWrite) {
    const auto date = Date{25, 5, 2005};
    const auto dummyPassword = "dummy";

    auto repo = LocalLogRepository(TMPDirPathProvider, dummyPassword);
    const std::string logContent = "Dummy string";
    const std::string encLogContent = "\x16\x1A/l\x1\x1"
                                      "7\a\x1\xEE\xD2n";

    repo.write(LogFile{date, logContent});
    ASSERT_TRUE(std::filesystem::exists(TMPDirPathProvider.path(date)));
    EXPECT_EQ(readFile(TMPDirPathProvider.path(date)), encLogContent);
}

TEST_F(LocalLogRepositoryTest, EncryptionRoundtrip) {
    const auto date = Date{25, 5, 2005};
    const auto dummyPassword = "dummy";

    auto repo = LocalLogRepository(TMPDirPathProvider, dummyPassword);
    const std::string logContent = "Dummy string";

    repo.write(LogFile{date, logContent});
    ASSERT_TRUE(std::filesystem::exists(TMPDirPathProvider.path(date)));

    auto log = repo.read(date);
    ASSERT_TRUE(log.has_value());
    ASSERT_EQ(log->getDate(), date);
    ASSERT_EQ(log->getContent(), logContent);
}
