#include "config.hpp"
#include "date/date.hpp"
#include "log/local_log_repository.hpp"
#include "log/log_file.hpp"
#include "log/log_repository_base.hpp"
#include "log/log_repository_crypto_applyer.hpp"
#include "log/year_overview_data.hpp"
#include "mocks.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

using namespace caps_log::date;
using namespace caps_log::log;
using namespace ::testing;

namespace {
const std::filesystem::path TEST_LOG_DIRECTORY = std::filesystem::current_path() / "test_dir";
std::string readFile(const std::string &path) {
    std::ifstream ifs{path};
    EXPECT_TRUE(ifs.is_open());
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

} // namespace

class LocalLogRepositoryTest : public ::testing::Test {
  public:
    LocalFSLogFilePathProvider TMPDirPathProvider{TEST_LOG_DIRECTORY,
                                                  caps_log::Config::DEFAULT_LOG_FILENAME_FORMAT};

    void SetUp() override { std::filesystem::create_directory(TEST_LOG_DIRECTORY); }
    void TearDown() override { std::filesystem::remove_all(TEST_LOG_DIRECTORY); }
    void writeDummyLog(const Date &date, const std::string &content) const {
        std::ofstream ofs(TMPDirPathProvider.path(date));
        ofs << content;
    }
    static void writeDummyFile(const std::string &path, const std::string &content) {
        std::ofstream ofs(path);
        ofs << content;
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
    const auto *const dummyPassword = "dummy";

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
    const auto *const dummyPassword = "dummy";

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
    const auto *const dummyPassword = "dummy";

    auto repo = LocalLogRepository(TMPDirPathProvider, dummyPassword);
    const std::string logContent = "Dummy string";

    repo.write(LogFile{date, logContent});
    ASSERT_TRUE(std::filesystem::exists(TMPDirPathProvider.path(date)));

    auto log = repo.read(date);
    ASSERT_TRUE(log.has_value());
    ASSERT_EQ(log->getDate(), date);
    ASSERT_EQ(log->getContent(), logContent);
}

TEST_F(LocalLogRepositoryTest, RoundtripCryptoApplier) {
    const auto date1 = Date{25, 5, 2005};
    const auto date2 = Date{25, 5, 2015};
    const std::string logContent = "Dummy string";
    const auto *const dummyPassword = "dummy";
    const std::string encLogContent = "\x16\x1A/l\x1\x1"
                                      "7\a\x1\xEE\xD2n";

    // create unencrypted files
    writeDummyLog(date1, logContent);
    writeDummyLog(date2, logContent);
    EXPECT_EQ(logContent, readFile(TMPDirPathProvider.path(date1)));
    EXPECT_EQ(logContent, readFile(TMPDirPathProvider.path(date2)));

    // encrypt them & verify encription
    caps_log::LogRepositoryCrypoApplier::apply(dummyPassword, TMPDirPathProvider.getLogDirPath(),
                                               TMPDirPathProvider.getLogFilenameFormat(),
                                               caps_log::Crypto::Encrypt);
    EXPECT_EQ(encLogContent, readFile(TMPDirPathProvider.path(date1)));
    EXPECT_EQ(encLogContent, readFile(TMPDirPathProvider.path(date2)));

    // decrypt them & verify encription
    caps_log::LogRepositoryCrypoApplier::apply(dummyPassword, TMPDirPathProvider.getLogDirPath(),
                                               TMPDirPathProvider.getLogFilenameFormat(),
                                               caps_log::Crypto::Decrypt);
    EXPECT_EQ(logContent, readFile(TMPDirPathProvider.path(date1)));
    EXPECT_EQ(logContent, readFile(TMPDirPathProvider.path(date2)));
}

TEST_F(LocalLogRepositoryTest, CryptoApplier_IgnoresFilesNotMatchingTheLogFilenamePattern) {
    const auto date1 = Date{25, 5, 2005};
    const std::string logContent = "Dummy string";
    const auto *const dummyPassword = "dummy";
    const auto *const dummyfileName = "dummy.txt";
    const std::string encLogContent = "\x16\x1A/l\x1\x1"
                                      "7\a\x1\xEE\xD2n";

    // create unencrypted files
    writeDummyLog(date1, logContent);
    writeDummyFile(TEST_LOG_DIRECTORY / dummyfileName, logContent);
    EXPECT_EQ(logContent, readFile(TMPDirPathProvider.path(date1)));
    EXPECT_EQ(logContent, readFile(TEST_LOG_DIRECTORY / dummyfileName));

    // encrypt them & verify encription
    caps_log::LogRepositoryCrypoApplier::apply(dummyPassword, TMPDirPathProvider.getLogDirPath(),
                                               TMPDirPathProvider.getLogFilenameFormat(),
                                               caps_log::Crypto::Encrypt);
    EXPECT_EQ(encLogContent, readFile(TMPDirPathProvider.path(date1)));
    EXPECT_EQ(logContent, readFile(TEST_LOG_DIRECTORY / dummyfileName));
}

TEST_F(LocalLogRepositoryTest, CryptoApplier_DoesNotApplyAnOperationTwice) {
    const auto date1 = Date{25, 5, 2005};
    const std::string logContent = "Dummy string";
    const auto *const dummyPassword = "dummy";
    const std::string encLogContent = "\x16\x1A/l\x1\x1"
                                      "7\a\x1\xEE\xD2n";

    writeDummyLog(date1, logContent);
    caps_log::LogRepositoryCrypoApplier::apply(dummyPassword, TMPDirPathProvider.getLogDirPath(),
                                               TMPDirPathProvider.getLogFilenameFormat(),
                                               caps_log::Crypto::Encrypt);
    ASSERT_THROW(caps_log::LogRepositoryCrypoApplier::apply(
                     dummyPassword, TMPDirPathProvider.getLogDirPath(),
                     TMPDirPathProvider.getLogFilenameFormat(), caps_log::Crypto::Encrypt),
                 caps_log::CryptoAlreadyAppliedError);
}
