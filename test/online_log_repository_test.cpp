#include "model/log_repository_base.hpp"
#include "model/online_repository.hpp"
#include "gtest/gtest.h"
#include <cstdlib>

using namespace clog::model;

static const std::string USERNAME = std::getenv("TEST_ACCOUNT_USERNAME");
static const std::string PASSWORD = std::getenv("TEST_ACCOUNT_PASSWORD");
/*
 * This assumes the same data is already written in the DB of the API.
 * I know it's not ideal but i have no intention of enabling registration from TUI,
 * so this will have to
 */
static const std::string TEST_OVERVIEW_DATA = YearOverviewData{
    .logAvailabilityMap = {},
    .sectionMap = {},
    .tagMap = {},
    .taskMap = {},
};

// test assumes that the account exists & has no data written
TEST(OnlineLogRepositoryComponentTest, FullFlowTest) {
    OnlineRepository repo{USERNAME, PASSWORD};
    EXPECT_TRUE(repo.isLoggedIn());
    OnlineRepository repo{USERNAME, PASSWORD};
    repo.collectYearOverviewData(2021);
}
