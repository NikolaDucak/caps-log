#include "date/date.hpp"

#include <array>
#include <gtest/gtest.h>

using clog::date::Date;

TEST(Date, ConstructorThrowsOnBadArgs) {
    EXPECT_THROW(Date(0, 0, 0), std::invalid_argument);
    EXPECT_THROW(Date(6, 0, 0), std::invalid_argument);
    EXPECT_THROW(Date(30, 2, 1), std::invalid_argument);
    EXPECT_THROW(Date(0, 11, 2021), std::invalid_argument);
    EXPECT_THROW(Date(1000, 1000, 999999999), std::invalid_argument);
    EXPECT_THROW(Date(0, 1000, 999999999), std::invalid_argument);

    EXPECT_NO_THROW(Date(1, 1, 1));
    EXPECT_NO_THROW(Date(2, 2, 2));
    EXPECT_NO_THROW(Date(28, 2, 1));
    EXPECT_NO_THROW(Date(29, 2, 4));
    EXPECT_NO_THROW(Date(6, 11, 0));
    EXPECT_NO_THROW(Date(6, 11, 2021));
}

TEST(Date, GetWeekday) {
    std::array<Date, 7> test_data {
        // nov 2021 - monday/0
        Date { 1, 11, 2021 },
        //  - tuesday/1
        Date { 1, 6, 2021 },
        //  - wednesdayy/2
        Date { 1, 12, 2021 },
        //  - thursday/3
        Date { 1, 7, 2021 },
        //  - friday/4
        Date { 1, 1, 2021 },
        //  - saturday/5
        Date { 1, 5, 2021 },
        //  - sunday/6
        Date { 1, 5, 2022 },
    };

    for (int i = 0; i < 7; i++) {
        EXPECT_NO_THROW(test_data[i]);
        EXPECT_EQ(i, test_data[i].getWeekday());
    }
}

TEST(Date, GetNumberOfDaysForMonth) {
    struct test_data {
        Date month;
        unsigned days;
    };
    std::vector<test_data> data { { { 1, 1, 1 }, 31 },  { { 1, 2, 1 }, 28 },
                                  { { 1, 2, 4 }, 29 },  { { 1, 3, 1 }, 31 },
                                  { { 1, 4, 1 }, 30 },  { { 1, 5, 1 }, 31 },
                                  { { 1, 6, 1 }, 30 },  { { 1, 7, 1 }, 31 },
                                  { { 1, 8, 1 }, 31 },  { { 1, 9, 1 }, 30 },
                                  { { 1, 10, 1 }, 31 }, { { 1, 11, 1 }, 30 },
                                  { { 1, 12, 1 }, 31 }, { { 1, 2, 2021 }, 28 }
    };

    for (const auto& test : data) {
        EXPECT_EQ(clog::date::getNumberOfDaysForMonth(test.month.month,
                                                            test.month.year),
                  test.days)
            << "Error on month: " << test.month.month;
    }
}

TEST(Date, GetStartingDayOfMonth) {
    std::vector<unsigned> expectations { 5, 1, 1, 4, 6, 2, 4, 7, 3, 5, 1, 3 };
    for (int i = 0; i < 12; i++) {
        EXPECT_EQ(expectations[i],
                  ::clog::date::getStartingWeekdayForMonth(i + 1, 2021));
    }
}

TEST(Date, GetStringNameForMonth) {
    std::string expectations[] { "january",   "february", "march",   "april",
                                 "may",       "june",     "jully",   "august",
                                 "september", "october",  "november" };
    for (int i = 0; i < 12; i++) {
        EXPECT_EQ(expectations[i],
                  ::clog::date::getStringNameForMonth(i + 1));
    }
    EXPECT_EQ("", ::clog::date::getStringNameForMonth(13));
    EXPECT_EQ("", ::clog::date::getStringNameForMonth(200));
}
