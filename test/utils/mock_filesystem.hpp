#pragma once 

#include "../source/model/filesystem_accessor_base.hpp"
#include "../source/model/log_file.hpp"

#include <filesystem>
#include <map>


struct log {
    clog::model::date d;
    std::string contents;
};

class comparator {
public:
    bool operator()(const clog::model::date& l, const clog::model::date& r) const {
        std::tm tl {};
        std::tm tr {};
        tl.tm_mon  = l.month - 1;
        tl.tm_mday = l.day;
        tl.tm_year = l.year - 1900;
        tr.tm_mon  = r.month - 1;
        tr.tm_mday = r.day;
        tr.tm_year = r.year - 1900;

        auto time_r = std::mktime(&tr);
        auto time_l = std::mktime(&tl);

        return time_l < time_r;
    }
};

class mock_filesystem_accessor : public clog::model::filesystem_accessor_base {
public:
    std::map<clog::model::date, std::string, comparator> logs;

    mock_filesystem_accessor(std::map<clog::model::date, std::string, comparator> logs) :
        clog::model::filesystem_accessor_base {}, logs(logs) {}

    virtual bool exists(const clog::model::date& d) const override {
        return logs.find(d) != logs.end();
    }

    virtual std::string read_file(const clog::model::date& d) const override {
        return exists(d) ? logs.at(d) : "";
    }

    virtual void write_file(const clog::model::date& d, const std::string& data) override {
        if (exists(d))
            return;
        logs[d] = data;
    }

    virtual void remove_file(const clog::model::date& d) override {
        logs.erase(logs.find(d));
    }

    virtual std::string get_file_path(const clog::model::date& ) const override {
        return "";
    }
};

class mock_single_file_fiflesystem : public clog::model::filesystem_accessor_base {
public:
    std::optional<clog::model::date> m_date;
    std::string m_content;

    mock_single_file_fiflesystem(const clog::model::date& ad, std::string c) 
        : m_date(ad), m_content(c) {}

    virtual bool exists(const clog::model::date& date) const override {
        return m_date == date;
    }

    virtual std::string read_file(const clog::model::date& d) const override {
        return exists(d) ? m_content : "";
    }

    virtual void write_file(const clog::model::date& date, const std::string& data) override {
        if (exists(date))
            return;
        m_content = data;
    }

    virtual void remove_file(const clog::model::date& date) override {
        if (exists(date)) {
            m_date = {};
            m_content = "";
        }
    }

    virtual std::string get_file_path(const clog::model::date& ) const override {
        return "";
    }
};
