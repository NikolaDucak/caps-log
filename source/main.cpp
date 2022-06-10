#include "app.hpp"

#include "model/default_log_repository.hpp"
#include <cstdlib>

class EnvBasedEditor : public clog::EditorBase {
  public:
    void openEditor(const std::string &path) {
        if (std::getenv("EDITOR")) {
            std::system(("$EDITOR" + path).c_str());
        }
    }
};

int main() {
    auto repo = std::make_shared<clog::model::DefaultLogRepository>();
    auto view = std::make_shared<clog::view::YearView>(clog::model::Date::getToday());
    auto editor = std::make_shared<EnvBasedEditor>();
    clog::App clog{view, repo, editor};
    clog.run();
    return 0;
}
