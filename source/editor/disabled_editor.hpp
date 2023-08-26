#pragma once

#include "editor_base.hpp"

namespace caps_log::editor {

class DisabledEditor : public EditorBase {
  public:
    void openEditor(const caps_log::model::LogFile &log) {}
};

} // namespace caps_log::editor
