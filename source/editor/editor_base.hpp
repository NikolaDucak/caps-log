#pragma once 

#include <string>

#include "model/local_log_repository.hpp"

/**
 * An EditorBase implementation is supposed to open any type of editor (or not at all) with the provided log. 
 * Clog so provides 2 implementations: 
 *  - "EDITOR" environment variable based and 
 *  - disabled editor that does nothing currently used for WASM clog version.
 */
namespace clog::editor {

class EditorBase {
public:
    virtual void openEditor(const clog::model::LogFile& log) = 0;
};
 
}
