#pragma once

/**
 * An EditorBase implementation is supposed to open any type of editor (or not at all) with the
 * provided log. Caps log so provides 2 implementations:
 *  - "EDITOR" environment variable based and
 *  - disabled editor that does nothing currently used for WASM caps-log version (in progress).
 */
#include "log/log_file.hpp"

namespace caps_log::editor {

class EditorBase {
  public:
    virtual ~EditorBase() = default;
    virtual void openEditor(const caps_log::log::LogFile &log) = 0;
};

} // namespace caps_log::editor
