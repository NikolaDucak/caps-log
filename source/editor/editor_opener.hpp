#pragma once

#include <string>
#include <utility>

namespace caps_log::editor {

class EditorOpener {
  public:
    virtual void open(std::string path) = 0;
    virtual ~EditorOpener() {}
};

class XdgOpenEditorOpener : public EditorOpener {
  public:
    void open(std::string path) override {
        std::ignore = std::system(("/usr/bin/xdg-open " + path).c_str());
    }
};

class EnvVarEditorOpener : public EditorOpener {
  public:
    void open(std::string path) override { std::ignore = std::system(("$EDITOR " + path).c_str()); }
};


}
