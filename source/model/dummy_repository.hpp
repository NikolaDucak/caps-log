#pragma once

#include "log_repository_base.hpp"

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
