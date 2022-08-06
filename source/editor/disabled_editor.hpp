#pragma once

#include "editor_base.hpp"

namespace clog::editor {

class DisabledEditor : public EditorBase {
public:
    void openEditor(const clog::model::LogFile& log) {}
};

}

