#pragma once 

#include "date/date.hpp"
#include "log_repository_base.hpp"
#include "nlohmann/json.hpp"

#include <optional>

extern "C" {
    extern const char* getOverviewData(int year);
    extern const char* getContentForLogEntry(int year, int month, int day);
    extern const char* removeLog(int year, int month, int day);
    extern const char* writeLog(int year, int month, int day, const char* data);
}

namespace nlohmann::json {

    clog::date::Date parse_date();

    void from_json(nlohmann::json& json, clog::date::YearMap<bool>& data) {
        for (const auto& kv : json.items()) {
            data.set(parse_date(kv.key()), true);
        }
    }

    void from_json(nlohmann::json& json, clog::date::StringYearMap& data) {
        for (const auto& kv : json) {
            for (const auto& date : kv.value()) {
                date[kv.key()].set(parse_date(kv.value()));
            }
        }
    }

    void from_json(nlohmann::json& json, clog::model::YearOverviewData& data) {
       json.at("logAvailabilityMap").get_to(data.logAvailabilityMap);
       json.at("tagMap").get_to(data.tagMap);
       json.at("taskMap").get_to(data.taskMap);
       json.at("sectionMap").get_to(data.sectionMap);
    }
}

namespace clog::model {


class OnlineRepository : public LogRepositoryBase {
public:

    YearOverviewData collectYearOverviewData(unsigned year) const override {
        auto response = getOverviewData(year);
    }

    void injectOverviewDataForDate(YearOverviewData &data, const Date &date) const override { }

    void std::optional<LogFile> read(const Date &date) const override {
        return LogFile{"dummy content"};
    }

    void remove(const Date &date) override {
        remove(date);
    }

    void write(const Date &date) override { 
    }

    std::string path(const Date &date) override { return ""; }

};

}
