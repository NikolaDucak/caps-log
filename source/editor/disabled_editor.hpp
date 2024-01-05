#pragma once

#include "editor_base.hpp"

namespace caps_log::editor {

class DisabledEditor : public EditorBase {
  public:
    void openEditor(const caps_log::log::LogFile &log) override {}
};

} // namespace caps_log::editor
