#pragma once 

#include <string>

/**
 * An EditorBase implementation is supposed to open any type of editor (or not at all) 
 * with the provided file loaded. Clog so provides 2 implementations: "EDITOR" environment 
 * variable based and disabled editor that does nothing currently used for WASM clog version.
 */
namespace clog {

class EditorBase {
public:
    virtual void openEditor(const std::string &path) = 0;
};

}
