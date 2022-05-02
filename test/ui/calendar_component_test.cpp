#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <gtest/gtest.h>
#include "ftxui/component/component_options.hpp"
#include "model/Date.hpp"

/*
using namespace ftxui;
using namespace clog::model;

TEST(Calendar, GetFocusedDate) {
    Date today = {6,6,2022};
    Date tomorrow = {7,6,2022};
    Calendar calendar{2022, today};

    EXPECT_EQ(calendar.getFocusedDate(), today);
    calendar.OnEvent(Event::Character('l'));
    EXPECT_EQ(calendar.getFocusedDate(), tomorrow);
}

TEST(CalendarComponent, OnFocusedDateChange) {
    CalendarComponent calendar;

    EXPECT_EQ(calendar.getFocusedDate(), today);
    calendar.OnEvent(Event::Character('l'))
    EXPECT_EQ(calendar.getFocusedDate(), today + 1);
}

TEST(CalendarComponent, OnEnter) {
    CalendarComponent calendar;

    EXPECT_EQ(calendar.getFocusedDate(), today);
    calendar.OnEvent(Event::Character('l'));
    EXPECT_EQ(calendar.getFocusedDate(), today + 1);
}

class MenuComponent : public ComponentBase{
};

TEST(MenuComponent, OnFocusedItemChange) {
};

class YearOverviewUI : public ComponentBase {
};

TEST(YearOverviewUI, PropagatesInputToControler) {
    YearOverviewUI ui;
    ui.OnEvent(Event::Tab);
    ui.OnEvent(Event::Tab);

    // TODO: Expect call on mock controller for on focused date change
    ui.OnEvent(Event::ArrowRight);

    // TODO: Expect call on mock controller for on enter

    ui.OnEvent(Event::Return);
}

*/

