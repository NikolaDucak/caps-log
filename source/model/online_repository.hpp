#pragma once 

#include "date/date.hpp"
#include "log_repository_base.hpp"
#include "nlohmann/json.hpp"
#include "utils/http_client.hpp"

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

class OnlineRepository : LogRepositoryBase{
    std::string m_apiBaseUrl;
    std::string m_apiPort;
    std::string m_sessionId;
    utils::HTTPClient client;

public:
    OnlineRepository(
            const std::string& username, 
            const std::string& password, 
            const std::string& apiBaseUrl, 
            const std::string& apiPort) 
        : m_apiBaseUrl{apiBaseUrl}, m_apiPort{apiPort} { auth(); }

    bool isLoggedIn() {
        return not m_sessionId.empty();
    }

    YearOverviewData collectYearOverviewData(unsigned year) const override {
        auto response = client.get(m_apiBaseUrl, m_apiPort, "/getOverivewData", "");
    }

    void injectOverviewDataForDate(YearOverviewData &data, const Date &date) const override { }

    std::optional<LogFile> read(const Date &date) const override {
        return LogFile{"dummy content"};
    }

    void remove(const Date &date) override {  }

    void write(const LogFile& entry) override {  }

    std::string path(const Date &date) override { return ""; }
private:
    void auth() {

    }
};


class OnlineWasmRepository : public LogRepositoryBase {
public:

    YearOverviewData collectYearOverviewData(unsigned year) const override {
        auto response = getOverviewData(year);
    }

    void injectOverviewDataForDate(YearOverviewData &data, const Date &date) const override { }

    std::optional<LogFile> read(const Date &date) const override {
        return LogFile{"dummy content"};
    }

    void remove(const Date &date) override { }

    void write(const LogFile &entry) override { }

    std::string path(const Date &date) override { return ""; }
};

}
