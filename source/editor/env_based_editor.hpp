#pragma once

#include <cstdlib>
#include <string>

#include "editor_base.hpp"
#include "model/local_log_repository.hpp"

namespace clog::editor {

class EnvBasedEditor : public EditorBase {
    clog::model::LocalFSLogFilePathProvider m_pathProvider;

  public:
    EnvBasedEditor(clog::model::LocalFSLogFilePathProvider pathProvider)
        : m_pathProvider{std::move(pathProvider)} {}

    void openEditor(const clog::model::LogFile &log) override {
        if (std::getenv("EDITOR") != nullptr) {
            std::system(("$EDITOR " + m_pathProvider.path(log.getDate())).c_str());
        }
    }
};

} // namespace clog::editor
