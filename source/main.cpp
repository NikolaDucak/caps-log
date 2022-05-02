#include "app.hpp"

#include "model/DefaultLogRepository.hpp"

int main() {
    auto repo = std::make_shared<clog::model::DefaultLogRepository>();
    auto view = std::make_shared<clog::view::YearlyView>(clog::model::Date::getToday());
    clog::App clog { view, repo };
    clog.run();
    return 0;
}
