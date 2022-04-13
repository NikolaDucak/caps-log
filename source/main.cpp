#include "app.hpp"

#include "model/DefaultLogRepository.hpp"

int main() {
    clog::model::DefaultLogRepository repo;
    clog::view::YearlyView view { clog::model::Date::getToday() };
    clog::App clog { view, repo };
    clog.run();
    return 0;
}
