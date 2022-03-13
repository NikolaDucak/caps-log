#pragma once

#include "../model/Date.hpp"
#include "../model/LogRepository.hpp"
#include "./ftxui_ext/extended_containers.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>

namespace clog::view {


using namespace ftxui;

class CalendarComponent : public ComponentBase {
    friend class CalendarHighlighMenuComponent;
    friend class YearlyView;

    model::Date m_today;
    Component m_root;
    std::function<void(const model::Date&)> m_onEnterCallback;
    const model::YearMap<bool>* m_logAvailabilityMap;

    model::YearMap<bool>* m_selectedHighlightMap = nullptr;
    unsigned currentlyDisplayedYear;

public:
    CalendarComponent(const model::Date& today, const model::YearMap<bool>* datesWithLogEntries, 
        std::function<void(const model::Date&)> onEnterCallback);
    Component getFTXUIComponent();
    void update() { }

private:

    Component createYear(unsigned year);
    Component createMonth(unsigned month, unsigned year);
    Component createDay(const model::Date& date);
};

}  // namespace clog::view
