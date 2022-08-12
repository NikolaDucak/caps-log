#include "date/date.hpp"

#include <gmock/gmock-spec-builders.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdexcept>

using namespace clog::date;
using namespace testing;

TEST(DateTest, InvalidDatesThrow) {
    // one bellow minimal valid date
    EXPECT_THROW(Date(31, DECEMBER, 1800), std::invalid_argument);
    EXPECT_THROW(Date(6, JANUARY - 1, 2000), std::invalid_argument);
    EXPECT_THROW(Date(15, MARCH, 1), std::invalid_argument);
    EXPECT_THROW(Date(0, DECEMBER, 2021), std::invalid_argument);
    EXPECT_THROW(Date(1000, DECEMBER + 100, 999999999), std::invalid_argument);
}

TEST(DateTest, ValidDatesDontThrow) {
    // minimal valid date
    EXPECT_NO_THROW(Date(1, JANUARY, 1900));
    EXPECT_NO_THROW(Date(1, JANUARY, 2030));
    EXPECT_NO_THROW(Date(2, FEBRUARY, 1999));
    EXPECT_NO_THROW(Date(28, FEBRUARY, 1970));
    EXPECT_NO_THROW(Date(1, JANUARY, 1970));
    EXPECT_NO_THROW(Date(6, NOVEMBER, 2000));
    EXPECT_NO_THROW(Date(6, DECEMBER, 2300));
    EXPECT_NO_THROW(Date(31, DECEMBER, 999999999));
}

TEST(DateTest, IsWeekend) {
    EXPECT_EQ(Date(1, 11, 2021).isWeekend(), false);
    EXPECT_EQ(Date(1, 6, 2021).isWeekend(), false);
    EXPECT_EQ(Date(1, 12, 2021).isWeekend(), false);
    EXPECT_EQ(Date(1, 7, 2021).isWeekend(), false);
    EXPECT_EQ(Date(1, 1, 2021).isWeekend(), false);
    EXPECT_EQ(Date(1, 5, 2021).isWeekend(), true);
    EXPECT_EQ(Date(1, 5, 2022).isWeekend(), true);
}

TEST(DateTest, GetWeekday) {
    EXPECT_EQ(Date(1, 11, 2021).getWeekday(), MONDAY);
    EXPECT_EQ(Date(1, 6, 2021).getWeekday(), TUESDAY);
    EXPECT_EQ(Date(1, DECEMBER, 2021).getWeekday(), WEDNESSDAY);
    EXPECT_EQ(Date(1, 7, 2021).getWeekday(), THURSDAY);
    EXPECT_EQ(Date(1, 1, 2021).getWeekday(), FRIDAY);
    EXPECT_EQ(Date(1, 5, 2021).getWeekday(), SATURDAY);
    EXPECT_EQ(Date(1, 5, 2022).getWeekday(), SUNDAY);
}

TEST(DateTest, GetStringNameForMonth) {
    EXPECT_EQ(getStringNameForMonth(JANUARY), "january");
    EXPECT_EQ(getStringNameForMonth(FEBRUARY), "february");
    EXPECT_EQ(getStringNameForMonth(MARCH), "march");
    EXPECT_EQ(getStringNameForMonth(APRIL), "april");
    EXPECT_EQ(getStringNameForMonth(MAY), "may");
    EXPECT_EQ(getStringNameForMonth(JUNE), "june");
    EXPECT_EQ(getStringNameForMonth(JULY), "jully"); // TODO: spelling, sigh...
    EXPECT_EQ(getStringNameForMonth(AUGUST), "august");
    EXPECT_EQ(getStringNameForMonth(SEPTEMBER), "september");
    EXPECT_EQ(getStringNameForMonth(OCTOBER), "october");
    EXPECT_EQ(getStringNameForMonth(NOVEMBER), "november");
    EXPECT_EQ(getStringNameForMonth(DECEMBER), "december");
}
