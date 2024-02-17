#include "config.hpp"
#include "date/date.hpp"
#include "log/local_log_repository.hpp"
#include "log/log_file.hpp"
#include "log/log_repository_base.hpp"
#include "log/log_repository_crypto_applyer.hpp"
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

class EncryptedLocalLogRepositoryTest : public LocalLogRepositoryTest {
  public:
    void SetUp() override {
        LocalLogRepositoryTest::SetUp();
        writeDummyEncryptedRepoMarkerFile(TMPDirPathProvider.getLogDirPath(), dummyPassword);
    }

  private:
    static void writeDummyEncryptedRepoMarkerFile(const std::filesystem::path &path,
                                                  const std::string &password) {
        std::ofstream cle{path / caps_log::LogRepositoryCryptoApplier::encryptetLogRepoMarkerFile,
                          std::ios::binary};
        if (not cle.is_open()) {
            throw std::runtime_error{"Failed writing encryption marker file"};
        }
        std::istringstream oss{caps_log::LogRepositoryCryptoApplier::encryptetLogRepoMarker};
        cle << caps_log::utils::encrypt(password, oss);
    }

  protected:
    static constexpr auto dummyLogContent = "Dummy string";
    static constexpr auto dummyPassword = "dummy";
    static constexpr auto encryptedDummyLogContent = "\x16\x1A/l\x1\x1"
                                                     "7\a\x1\xEE\xD2n";
};

TEST_F(EncryptedLocalLogRepositoryTest, EncryptedRead) {
    const auto selectedDate = Date{25, 5, 2005};

    auto repo = LocalLogRepository(TMPDirPathProvider, dummyPassword);

    ASSERT_FALSE(repo.read(selectedDate).has_value());
    writeDummyLog(selectedDate, encryptedDummyLogContent);

    auto log = repo.read(selectedDate);
    ASSERT_TRUE(log.has_value());
    ASSERT_EQ(log->getDate(), selectedDate);
    ASSERT_EQ(log->getContent(), dummyLogContent);

    EXPECT_EQ(readFile(TMPDirPathProvider.path(selectedDate)), encryptedDummyLogContent);
}

TEST_F(EncryptedLocalLogRepositoryTest, EncryptedWrite) {
    const auto date = Date{25, 5, 2005};

    auto repo = LocalLogRepository(TMPDirPathProvider, dummyPassword);

    repo.write(LogFile{date, dummyLogContent});
    ASSERT_TRUE(std::filesystem::exists(TMPDirPathProvider.path(date)));
    EXPECT_EQ(readFile(TMPDirPathProvider.path(date)), encryptedDummyLogContent);
}

TEST_F(EncryptedLocalLogRepositoryTest, EncryptionRoundtrip) {
    const auto date = Date{25, 5, 2005};

    auto repo = LocalLogRepository(TMPDirPathProvider, dummyPassword);

    repo.write(LogFile{date, dummyLogContent});
    ASSERT_TRUE(std::filesystem::exists(TMPDirPathProvider.path(date)));

    auto log = repo.read(date);
    ASSERT_TRUE(log.has_value());
    ASSERT_EQ(log->getDate(), date);
    ASSERT_EQ(log->getContent(), dummyLogContent);
}

class LogRepositoryCryptoApplierTest : public LocalLogRepositoryTest {
  protected:
    static constexpr auto dummyLogContent = "Dummy string";
    static constexpr auto dummyPassword = "dummy";
    static constexpr auto encryptedDummyLogContent = "\x16\x1A/l\x1\x1"
                                                     "7\a\x1\xEE\xD2n";
    static const Date dummyDate;
};
const Date LogRepositoryCryptoApplierTest::dummyDate{25, 5, 2005};

TEST_F(LogRepositoryCryptoApplierTest, RoundtripCryptoApplier) {
    const auto date1 = Date{25, 5, 2005};
    const auto date2 = Date{25, 5, 2015};

    // create unencrypted files
    writeDummyLog(date1, dummyLogContent);
    writeDummyLog(date2, dummyLogContent);
    EXPECT_EQ(dummyLogContent, readFile(TMPDirPathProvider.path(date1)));
    EXPECT_EQ(dummyLogContent, readFile(TMPDirPathProvider.path(date2)));

    // encrypt them & verify encription
    caps_log::LogRepositoryCryptoApplier::apply(dummyPassword, TMPDirPathProvider.getLogDirPath(),
                                                TMPDirPathProvider.getLogFilenameFormat(),
                                                caps_log::Crypto::Encrypt);
    EXPECT_EQ(encryptedDummyLogContent, readFile(TMPDirPathProvider.path(date1)));
    EXPECT_EQ(encryptedDummyLogContent, readFile(TMPDirPathProvider.path(date2)));

    // decrypt them & verify encription
    caps_log::LogRepositoryCryptoApplier::apply(dummyPassword, TMPDirPathProvider.getLogDirPath(),
                                                TMPDirPathProvider.getLogFilenameFormat(),
                                                caps_log::Crypto::Decrypt);
    EXPECT_EQ(dummyLogContent, readFile(TMPDirPathProvider.path(date1)));
    EXPECT_EQ(dummyLogContent, readFile(TMPDirPathProvider.path(date2)));
}

TEST_F(LogRepositoryCryptoApplierTest, IgnoresFilesNotMatchingTheLogFilenamePattern) {
    const auto *const dummyfileName = "dummy.txt";
    // create unencrypted files
    writeDummyLog(dummyDate, dummyLogContent);
    writeDummyFile(TEST_LOG_DIRECTORY / dummyfileName, dummyLogContent);
    EXPECT_EQ(dummyLogContent, readFile(TMPDirPathProvider.path(dummyDate)));
    EXPECT_EQ(dummyLogContent, readFile(TEST_LOG_DIRECTORY / dummyfileName));

    // encrypt them & verify encription
    caps_log::LogRepositoryCryptoApplier::apply(dummyPassword, TMPDirPathProvider.getLogDirPath(),
                                                TMPDirPathProvider.getLogFilenameFormat(),
                                                caps_log::Crypto::Encrypt);
    EXPECT_EQ(encryptedDummyLogContent, readFile(TMPDirPathProvider.path(dummyDate)));
    EXPECT_EQ(dummyLogContent, readFile(TEST_LOG_DIRECTORY / dummyfileName));
}

TEST_F(LogRepositoryCryptoApplierTest, DoesNotApplyAnOperationTwice) {

    writeDummyLog(dummyDate, dummyLogContent);
    caps_log::LogRepositoryCryptoApplier::apply(dummyPassword, TMPDirPathProvider.getLogDirPath(),
                                                TMPDirPathProvider.getLogFilenameFormat(),
                                                caps_log::Crypto::Encrypt);
    ASSERT_THROW(caps_log::LogRepositoryCryptoApplier::apply(
                     dummyPassword, TMPDirPathProvider.getLogDirPath(),
                     TMPDirPathProvider.getLogFilenameFormat(), caps_log::Crypto::Encrypt),
                 caps_log::CryptoAlreadyAppliedError);
}

class LogRepoConstructionAfterCryptoApplier : public LocalLogRepositoryTest {
  protected:
    static constexpr auto dummyLogContent = "Dummy string";
    static constexpr auto dummyPassword = "dummy";
};

TEST_F(LogRepoConstructionAfterCryptoApplier, ErrorOnEncryptedRepoWithNoPassword) {
    const auto date = Date{25, 5, 2005};

    writeDummyLog(date, dummyLogContent);
    caps_log::LogRepositoryCryptoApplier::apply(dummyPassword, TMPDirPathProvider.getLogDirPath(),
                                                TMPDirPathProvider.getLogFilenameFormat(),
                                                caps_log::Crypto::Encrypt);

    EXPECT_THROW({ const auto repo = LocalLogRepository(TMPDirPathProvider); }, std::runtime_error);
    EXPECT_NO_THROW({ const auto repo = LocalLogRepository(TMPDirPathProvider, dummyPassword); });
}

TEST_F(LogRepoConstructionAfterCryptoApplier, ErrorOnEncryptedRepoWithBadPassword) {
    const auto date = Date{25, 5, 2005};
    const auto *const badPassword = "bad dummy";

    writeDummyLog(date, dummyLogContent);
    caps_log::LogRepositoryCryptoApplier::apply(dummyPassword, TMPDirPathProvider.getLogDirPath(),
                                                TMPDirPathProvider.getLogFilenameFormat(),
                                                caps_log::Crypto::Encrypt);

    EXPECT_THROW({ const auto repo = LocalLogRepository(TMPDirPathProvider, badPassword); },
                 std::runtime_error);
}
