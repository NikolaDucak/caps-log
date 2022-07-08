#pragma once

#include "editor_base.hpp"

namespace clog {

class DisabledEditor : public clog::EditorBase {
public:
    void openEditor(const std::string &path) { }
};

}

