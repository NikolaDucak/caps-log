#pragma once

#include "editor_base.hpp"

namespace clog::editor {

class DisabledEditor : public EditorBase {
public:
    void openEditor(const std::string &path) { }
};

}

