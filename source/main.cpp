#include "app.hpp"
#include "model/default_log_repository.hpp"
#include <iostream>
#include <cstdlib>

class DisabledEditor : public clog::EditorBase {
  public:
    void openEditor(const std::string &path) { }
};

class EnvBasedEditor : public clog::EditorBase {
  public:
    void openEditor(const std::string &path) {
        if (std::getenv("EDITOR")) {
            std::system(("$EDITOR" + path).c_str());
        }
    }
};

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
    auto editor = std::make_shared<DisabledEditor>();
    return clog::App { view, repo, editor };
}

auto makeTUIClog() {
    auto repo = std::make_shared<clog::model::DefaultLogRepository>();
    auto view = std::make_shared<clog::view::YearView>(clog::model::Date::getToday());
    auto editor = std::make_shared<EnvBasedEditor>();
    clog::App clog{view, repo, editor};
    return clog;
}

int main() try {
    makeWASMClog().run();
} catch(std::exception& e) {
    std::cout << "Error: " << e.what() << std::endl;
    std::cout << '\0'<< std::endl;
}
