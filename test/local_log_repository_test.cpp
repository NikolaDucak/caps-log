#include "config.hpp"
#include "log/local_log_repository.hpp"
#include "log/log_file.hpp"
#include "log/log_repository_crypto_applier.hpp"
#include "mocks.hpp"
#include "utils/crypto.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>

using namespace caps_log::log;
using namespace ::testing;

namespace {
const std::filesystem::path kTestLogDirectory = std::filesystem::current_path() / "test_dir";
std::string readFile(const std::string &path) {
    std::ifstream ifs{path};
    EXPECT_TRUE(ifs.is_open());
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

constexpr std::chrono::year_month_day kSelectedDate{std::chrono::year{2005}, std::chrono::month{5},
                                                    std::chrono::day{25}};
constexpr std::chrono::year_month_day kOtherSelectedDate{
    std::chrono::year{2005}, std::chrono::month{5}, std::chrono::day{25}};
} // namespace

class LocalLogRepositoryTest : public ::testing::Test {
  public:
    LocalFSLogFilePathProvider TMPDirPathProvider{kTestLogDirectory,
                                                  caps_log::Config::kDefaultLogFilenameFormat};

    const std::string kScratchpadFolderName = caps_log::Config::kDefaultScratchpadFolderName;

    void SetUp() override { std::filesystem::create_directory(kTestLogDirectory); }
    void TearDown() override { std::filesystem::remove_all(kTestLogDirectory); }
    void writeDummyLog(const std::chrono::year_month_day &date, const std::string &content) const {
        const auto path = TMPDirPathProvider.path(date);
        std::filesystem::create_directories(path.parent_path());
        std::ofstream ofs{path};
        ofs << content;
    }

    static void writeDummyFile(const std::string &path, const std::string &content) {
        std::ofstream ofs(path);
        ofs << content;
    }

    static void writeDummyScratchpad(const std::string &name, const std::string &content) {
        const std::filesystem::path kTestScratchpadDirectory =
            kTestLogDirectory / caps_log::Config::kDefaultScratchpadFolderName;

        if (!std::filesystem::exists(kTestScratchpadDirectory)) {
            std::filesystem::create_directories(kTestScratchpadDirectory);
        }
        const auto path = kTestScratchpadDirectory / name;
        std::ofstream ofs(path);
        ofs << content;
    }
};

TEST_F(LocalLogRepositoryTest, Read) {
    auto repo = LocalLogRepository(TMPDirPathProvider);
    const std::string logContent = "Dummy string";

    ASSERT_FALSE(repo.read(kSelectedDate).has_value());
    writeDummyLog(kSelectedDate, logContent);

    auto log = repo.read(kSelectedDate);
    ASSERT_TRUE(log.has_value());
    ASSERT_EQ(log->getDate(), kSelectedDate);
    ASSERT_EQ(log->getContent(), logContent);
}

TEST_F(LocalLogRepositoryTest, Remove) {
    auto repo = LocalLogRepository(TMPDirPathProvider);
    const std::string logContent = "Dummy string";

    writeDummyLog(kSelectedDate, logContent);
    ASSERT_TRUE(std::filesystem::exists(TMPDirPathProvider.path(kSelectedDate)));
    repo.remove(kSelectedDate);
    ASSERT_FALSE(std::filesystem::exists(TMPDirPathProvider.path(kSelectedDate)));
}

TEST_F(LocalLogRepositoryTest, Write) {
    auto repo = LocalLogRepository(TMPDirPathProvider);
    const std::string logContent = "Dummy string";

    repo.write(LogFile{kSelectedDate, logContent});
    ASSERT_TRUE(std::filesystem::exists(TMPDirPathProvider.path(kSelectedDate)));
}

class EncryptedLocalLogRepositoryTest : public LocalLogRepositoryTest {
  public:
    void SetUp() override {
        LocalLogRepositoryTest::SetUp();
        writeDummyEncryptedRepoMarkerFile(TMPDirPathProvider.getLogDirPath(), kDummyPassword);
    }

  private:
    static void writeDummyEncryptedRepoMarkerFile(const std::filesystem::path &path,
                                                  const std::string &password) {
        std::ofstream cle{path / caps_log::LogRepositoryCryptoApplier::kEncryptedLogRepoMarkerFile,
                          std::ios::binary};
        if (not cle.is_open()) {
            throw std::runtime_error{"Failed writing encryption marker file"};
        }
        std::istringstream oss{caps_log::LogRepositoryCryptoApplier::kEncryptedLogRepoMarker};
        cle << caps_log::utils::encrypt(password, oss);
    }

  protected:
    static constexpr auto kDummyLogContent = "Dummy string";
    static constexpr auto kDummyPassword = "dummy";
    static constexpr auto kEncryptedDummyLogContent = "\x16\x1A/l\x1\x1"
                                                      "7\a\x1\xEE\xD2n";
};

TEST_F(EncryptedLocalLogRepositoryTest, EncryptedRead) {
    auto repo = LocalLogRepository(TMPDirPathProvider, kDummyPassword);

    ASSERT_FALSE(repo.read(kSelectedDate).has_value());
    writeDummyLog(kSelectedDate, kEncryptedDummyLogContent);

    auto log = repo.read(kSelectedDate);
    ASSERT_TRUE(log.has_value());
    ASSERT_EQ(log->getDate(), kSelectedDate);
    ASSERT_EQ(log->getContent(), kDummyLogContent);

    EXPECT_EQ(readFile(TMPDirPathProvider.path(kSelectedDate)), kEncryptedDummyLogContent);
}

TEST_F(EncryptedLocalLogRepositoryTest, EncryptedWrite) {
    auto repo = LocalLogRepository(TMPDirPathProvider, kDummyPassword);

    repo.write(LogFile{kSelectedDate, kDummyLogContent});
    ASSERT_TRUE(std::filesystem::exists(TMPDirPathProvider.path(kSelectedDate)));
    EXPECT_EQ(readFile(TMPDirPathProvider.path(kSelectedDate)), kEncryptedDummyLogContent);
}

TEST_F(EncryptedLocalLogRepositoryTest, EncryptionRoundtrip) {
    auto repo = LocalLogRepository(TMPDirPathProvider, kDummyPassword);

    repo.write(LogFile{kSelectedDate, kDummyLogContent});
    ASSERT_TRUE(std::filesystem::exists(TMPDirPathProvider.path(kSelectedDate)));

    auto log = repo.read(kSelectedDate);
    ASSERT_TRUE(log.has_value());
    ASSERT_EQ(log->getDate(), kSelectedDate);
    ASSERT_EQ(log->getContent(), kDummyLogContent);
}

const std::filesystem::path kTestDataDir = std::filesystem::path{CAPS_LOG_TEST_DATA_DIR};

TEST_F(EncryptedLocalLogRepositoryTest, LongContentEncryptionRoundtrip) {
    auto repo = LocalLogRepository(TMPDirPathProvider, kDummyPassword);
    const auto longContentPath = kTestDataDir / "lorem_ipsum.txt";
    const auto longContent = readFile(longContentPath);
    const auto encryptedLongContentPath = kTestDataDir / "lorem_ipsum_ecrypted_with_word_dummy.bin";
    const auto encryptedLogContent = readFile(encryptedLongContentPath);

    repo.write(LogFile{kSelectedDate, longContent});
    ASSERT_TRUE(std::filesystem::exists(TMPDirPathProvider.path(kSelectedDate)));
    EXPECT_EQ(readFile(TMPDirPathProvider.path(kSelectedDate)), encryptedLogContent);

    auto log = repo.read(kSelectedDate);
    ASSERT_TRUE(log.has_value());
    ASSERT_EQ(log->getDate(), kSelectedDate);
    ASSERT_EQ(log->getContent(), longContent);
}

class LogRepositoryCryptoApplierTest : public LocalLogRepositoryTest {
  protected:
    static constexpr auto kDummyContent = "Dummy string";
    static constexpr auto kDummyPassword = "dummy";
    static constexpr auto kEncryptedDummyContent = "\x16\x1A/l\x1\x1"
                                                   "7\a\x1\xEE\xD2n";
};

TEST_F(LogRepositoryCryptoApplierTest, RoundtripCryptoApplier) {
    const auto &date1 = kSelectedDate;
    const auto &date2 = kOtherSelectedDate;

    // create unencrypted files
    writeDummyLog(date1, kDummyContent);
    writeDummyLog(date2, kDummyContent);
    EXPECT_EQ(kDummyContent, readFile(TMPDirPathProvider.path(date1)));
    EXPECT_EQ(kDummyContent, readFile(TMPDirPathProvider.path(date2)));

    // encrypt them & verify encryption
    caps_log::LogRepositoryCryptoApplier::apply(
        kDummyPassword, TMPDirPathProvider.getLogDirPath(), kScratchpadFolderName,
        TMPDirPathProvider.getLogFilenameFormat(), caps_log::Crypto::Encrypt);
    EXPECT_EQ(kEncryptedDummyContent, readFile(TMPDirPathProvider.path(date1)));
    EXPECT_EQ(kEncryptedDummyContent, readFile(TMPDirPathProvider.path(date2)));

    // decrypt them & verify encryption
    caps_log::LogRepositoryCryptoApplier::apply(
        kDummyPassword, TMPDirPathProvider.getLogDirPath(), kScratchpadFolderName,
        TMPDirPathProvider.getLogFilenameFormat(), caps_log::Crypto::Decrypt);
    EXPECT_EQ(kDummyContent, readFile(TMPDirPathProvider.path(date1)));
    EXPECT_EQ(kDummyContent, readFile(TMPDirPathProvider.path(date2)));
}

TEST_F(LogRepositoryCryptoApplierTest, RoundtripCryptoApplierWithScratchpads) {
    const auto kTestScratchpadDirectory =
        kTestLogDirectory / caps_log::Config::kDefaultScratchpadFolderName;
    const auto &date1 = kSelectedDate;
    const auto &date2 = kOtherSelectedDate;
    const auto &kDummyScratchpadName = "dummy.md";

    // create unencrypted files
    writeDummyLog(date1, kDummyContent);
    writeDummyLog(date2, kDummyContent);
    writeDummyScratchpad("dummy.md", kDummyContent);
    EXPECT_EQ(kDummyContent, readFile(TMPDirPathProvider.path(date1)));
    EXPECT_EQ(kDummyContent, readFile(TMPDirPathProvider.path(date2)));
    EXPECT_EQ(kDummyContent, readFile(kTestScratchpadDirectory / kDummyScratchpadName));

    // encrypt them & verify encryption
    caps_log::LogRepositoryCryptoApplier::apply(
        kDummyPassword, TMPDirPathProvider.getLogDirPath(), kScratchpadFolderName,
        TMPDirPathProvider.getLogFilenameFormat(), caps_log::Crypto::Encrypt);
    EXPECT_EQ(kEncryptedDummyContent, readFile(TMPDirPathProvider.path(date1)));
    EXPECT_EQ(kEncryptedDummyContent, readFile(TMPDirPathProvider.path(date2)));
    EXPECT_EQ(kEncryptedDummyContent, readFile(kTestScratchpadDirectory / kDummyScratchpadName));

    // decrypt them & verify encryption
    caps_log::LogRepositoryCryptoApplier::apply(
        kDummyPassword, TMPDirPathProvider.getLogDirPath(), kScratchpadFolderName,
        TMPDirPathProvider.getLogFilenameFormat(), caps_log::Crypto::Decrypt);
    EXPECT_EQ(kDummyContent, readFile(TMPDirPathProvider.path(date1)));
    EXPECT_EQ(kDummyContent, readFile(TMPDirPathProvider.path(date2)));
    EXPECT_EQ(kDummyContent, readFile(kTestScratchpadDirectory / kDummyScratchpadName));
}

TEST_F(LogRepositoryCryptoApplierTest, IgnoresScratchpadsNotMatchingTheLogFilenamePattern) {
    const auto kTestScratchpadDirectory =
        kTestLogDirectory / caps_log::Config::kDefaultScratchpadFolderName;

    const auto *const dummyfileName = "dummy.txt";
    const auto *const dummyScratchpadName = "dummy.doc";

    // create unencrypted files
    writeDummyLog(kSelectedDate, kDummyContent);
    writeDummyFile(kTestLogDirectory / dummyfileName, kDummyContent);
    writeDummyScratchpad(dummyScratchpadName, kDummyContent);
    EXPECT_EQ(kDummyContent, readFile(TMPDirPathProvider.path(kSelectedDate)));
    EXPECT_EQ(kDummyContent, readFile(kTestLogDirectory / dummyfileName));
    EXPECT_EQ(kDummyContent, readFile(kTestScratchpadDirectory / dummyScratchpadName));

    // encrypt them & verify encryption
    caps_log::LogRepositoryCryptoApplier::apply(
        kDummyPassword, TMPDirPathProvider.getLogDirPath(), kScratchpadFolderName,
        TMPDirPathProvider.getLogFilenameFormat(), caps_log::Crypto::Encrypt);
    EXPECT_EQ(kEncryptedDummyContent, readFile(TMPDirPathProvider.path(kSelectedDate)));
    EXPECT_EQ(kDummyContent, readFile(kTestLogDirectory / dummyfileName));
    EXPECT_EQ(kDummyContent, readFile(kTestScratchpadDirectory / dummyScratchpadName));
}

TEST_F(LogRepositoryCryptoApplierTest, DoesNotApplyAnOperationTwice) {
    writeDummyLog(kSelectedDate, kDummyContent);
    caps_log::LogRepositoryCryptoApplier::apply(kDummyPassword, TMPDirPathProvider.getLogDirPath(),
                                                TMPDirPathProvider.getLogFilenameFormat(),
                                                kScratchpadFolderName, caps_log::Crypto::Encrypt);
    ASSERT_THROW(caps_log::LogRepositoryCryptoApplier::apply(
                     kDummyPassword, TMPDirPathProvider.getLogDirPath(), kScratchpadFolderName,
                     TMPDirPathProvider.getLogFilenameFormat(), caps_log::Crypto::Encrypt),
                 caps_log::CryptoAlreadyAppliedError);
}

class LogRepoConstructionAfterCryptoApplier : public LocalLogRepositoryTest {
  protected:
    static constexpr auto kDummyLogContent = "Dummy string";
    static constexpr auto kDummyPassword = "dummy";
};

TEST_F(LogRepoConstructionAfterCryptoApplier, ErrorOnEncryptedRepoWithNoPassword) {
    writeDummyLog(kSelectedDate, kDummyLogContent);
    caps_log::LogRepositoryCryptoApplier::apply(
        kDummyPassword, TMPDirPathProvider.getLogDirPath(), kScratchpadFolderName,
        TMPDirPathProvider.getLogFilenameFormat(), caps_log::Crypto::Encrypt);

    EXPECT_THROW({ const auto repo = LocalLogRepository(TMPDirPathProvider); }, std::runtime_error);
    EXPECT_NO_THROW({ const auto repo = LocalLogRepository(TMPDirPathProvider, kDummyPassword); });
}

TEST_F(LogRepoConstructionAfterCryptoApplier, ErrorOnEncryptedRepoWithBadPassword) {
    const auto *const badPassword = "bad dummy";

    writeDummyLog(kSelectedDate, kDummyLogContent);
    caps_log::LogRepositoryCryptoApplier::apply(
        kDummyPassword, TMPDirPathProvider.getLogDirPath(), kScratchpadFolderName,
        TMPDirPathProvider.getLogFilenameFormat(), caps_log::Crypto::Encrypt);

    EXPECT_THROW(
        { const auto repo = LocalLogRepository(TMPDirPathProvider, badPassword); },
        std::runtime_error);
}
