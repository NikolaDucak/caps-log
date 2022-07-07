#include "app.hpp"

#include "model/Date.hpp"
#include "model/DefaultLogRepository.hpp"
#include "model/LogFile.hpp"
#include "model/LogRepositoryBase.hpp"
#include <iostream>

// #include <emscripten/bind.h>


namespace clog::model {

class DummyRepo: public clog::model::LogRepositoryBase {
public:
     YearLogEntryData collectDataForYear(unsigned year) override {
         YearMap<bool> map;
         map.set(Date::getToday(), true);
         return {
             .logAvailabilityMap = map,
             .sectionMap{},
             .taskMap{},
             .tagMap{},
         };
    }

     void injectDataForDate(YearLogEntryData& data, const Date& date) override {
         data.logAvailabilityMap.set(date, true);
     }

     std::optional<LogFile> readLogFile(const Date& date) override{
         return LogFile{"Some dummy content"};
     };

     LogFile readOrMakeLogFile(const Date& date) override {
         return LogFile{"newly created or read log file"};
     }

     void removeLog(const Date& date) override { }
     std::string path(const Date& date) override { return "/dummy/path"; }
};

}

auto makeWASMClog() {
    // TODO: instead of dummy repo, add api repo
    auto repo = std::make_shared<clog::model::DummyRepo>();
    auto view = std::make_shared<clog::view::YearlyView>(clog::model::Date::getToday());
    return clog::App { view, repo };
}

auto makeTUIClog() {
    auto repo = std::make_shared<clog::model::DefaultLogRepository>();
    auto view = std::make_shared<clog::view::YearlyView>(clog::model::Date::getToday());
    return clog::App { view, repo };
}

int main() try {
    makeWASMClog().run();
} catch(std::exception& e) {
    std::cout << "Error: " << e.what() << std::endl;
    const auto processor_count = std::thread::hardware_concurrency();
    std::cout << "count: " << processor_count << std::endl;
    std::cout << '\0'<< std::endl;
}
