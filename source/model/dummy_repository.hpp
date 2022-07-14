#pragma once

#include "log_repository_base.hpp"

namespace clog::model {

class DummyRepo: public clog::model::LogRepositoryBase {
public:
     YearOverviewData collectYearOverviewData(unsigned year) const override {
         YearMap<bool> map;
         map.set(Date::getToday(), true);
         return {
             .logAvailabilityMap = map,
             .sectionMap{},
             .taskMap{},
             .tagMap{},
         };
    }

     void injectOverviewDataForDate(YearOverviewData& data, const Date& date) const override {
         data.logAvailabilityMap.set(date, true);
     }

     std::optional<LogFile> read(const Date& date) const override {
         return LogFile{"Some dummy content..."};
     };

     void remove(const Date& date) override { }

     void write(const LogFile& date) override { }

     std::string path(const Date& date) override { return "/dummy/path"; }
};

}
