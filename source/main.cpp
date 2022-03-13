#include "ftxui/component/component_base.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "model/Date.hpp"
#include "model/DefaultLogRepository.hpp"
#include "model/LogFile.hpp"
#include "model/LogModel.hpp"
#include "view/yearly_view.hpp"

#include <iostream>
#include <memory>
#include <vector>

using namespace clog::model;
using namespace clog::view;

class app : public ControlerBase {
    clog::view::YearlyView& m_view;
    clog::model::LogModel& m_model;
    unsigned m_current_display_year;

public:
    app(clog::view::YearlyView& y, clog::model::LogModel& m) : m_view(y), m_model(m) {
        m_view.setContoler(this);
    }

    void run() { m_view.run(); }

    void openLogEntryInEditorForDade(const Date& d) override {
        auto log_path = m_model.getFilePathForLogEntry(d);
        std::system(("$EDITOR " + log_path).c_str());
        LogEntryFile log { m_model.m_filesystem, d };
        if (not log.hasData()) {
            log.removeFile();
        }
        m_model.injectDataForDate(m_view.getDisplayedData(), d);
        m_view.updateComponents();
    }

    void display_year(unsigned year) override {
        static auto current_display_year = 0;
        current_display_year             = year;
        m_view.setDisplayData(m_model.getDataforYear(current_display_year));
    }
};

int main() {
    clog::model::LogModel model { std::make_shared<clog::model::DefaultLogRepository>() };

    // TODO: :(
    auto a = model.getDataforYear(Date::getToday().year);

    clog::view::YearlyView view { Date::getToday(), a };

    app clog { view, model };

    clog.run();
    return 0;
}
