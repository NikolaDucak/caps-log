#pragma once

#include "../model/Date.hpp"

namespace clog::view {

class ControlerBase {
public:
    virtual void openLogEntryInEditorForDade(const model::Date& d) = 0;
    virtual void display_year(unsigned year) = 0;
    virtual ~ControlerBase()                          = default;
};

}  // namespace clog::view
