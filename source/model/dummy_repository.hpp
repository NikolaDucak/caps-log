#pragma once

#include "log_repository_base.hpp"

namespace clog::model {

class DummyLogRepository : public clog::model::LogRepositoryBase {
  public:
    std::optional<LogFile> read(const clog::date::Date &date) const override {
        return LogFile{date, "Some dummy content..."};
    };

    void remove(const clog::date::Date &date) override {}

    void write(const clog::model::LogFile &date) override {}
};

} // namespace clog::model
