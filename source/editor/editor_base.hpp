#pragma once

#include <string>

#include "log/local_log_repository.hpp"

/**
 * An EditorBase implementation is supposed to open any type of editor (or not at all) with the
 * provided log. Caps log so provides 2 implementations:
 *  - "EDITOR" environment variable based and
 *  - disabled editor that does nothing currently used for WASM caps-log version (in progress).
 */
namespace caps_log::editor {

class EditorBase {
  public:
    virtual void openEditor(const caps_log::model::LogFile &log) = 0;
};

} // namespace caps_log::editor
