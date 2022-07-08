#include <filesystem>
#include <fstream>
#include "model/default_log_repository.hpp"
#include "date/date.hpp"
#include <gtest/gtest.h>

using namespace clog::model;

static const std::string TEST_LOG_DIRECTORY
    {std::filesystem::temp_directory_path().string() + "/clog_test_dir"};

TEST(DefaultLogRepositoryTest, Remove) {
    const auto selectedDate = Date{25,5,2005};
    auto repo = DefaultLogRepository(TEST_LOG_DIRECTORY);

    auto l = repo.readOrMakeLogFile(selectedDate);
    {
        std::ofstream of(repo.path(selectedDate));
        of << "Dummy text";
    }
    ASSERT_TRUE(repo.collectDataForYear(2005).logAvailabilityMap.get(selectedDate));

    repo.removeLog(selectedDate);
    ASSERT_FALSE(repo.collectDataForYear(2005).logAvailabilityMap.get(selectedDate));
}
