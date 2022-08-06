#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <gtest/gtest.h>
#include "ftxui/component/component_options.hpp"
#include "date/date.hpp"
#include "view/calendar_component.hpp"
#include "view/yearly_view.hpp"

using namespace ftxui;
using namespace clog::view;
using namespace clog::date;

/*
TEST(YearViewTest, t) {
    YearView (mockScreen);
    v.setInputHandler(mockInputHandler);
    mockScreen.onCall(run()) {
        rootComponent.onEvent(Event::Character('l'));
    }
}
*/

TEST(CalendarComponent, GetFocusedDate) {
    Date today = {6,6,2022};
    Date tomorrow = {7,6,2022};
    CalendarOption options { };
    Calendar calendar{today, options};

    EXPECT_EQ(calendar.getFocusedDate(), today);
    calendar.OnEvent(Event::Character('l'));
    EXPECT_EQ(calendar.getFocusedDate(), tomorrow);
}

TEST(CalendarComponent, OnFocusedDateChange) {
}

TEST(CalendarComponent, OnEnter) {
}

TEST(MenuComponent, OnFocusedItemChange) {
}
