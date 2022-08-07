#include <iostream>

#include "app.hpp"
#include "date/date.hpp"
#include "editor/disabled_editor.hpp"
#include "editor/env_based_editor.hpp"
#include "model/dummy_repository.hpp"
#include "model/local_log_repository.hpp"
#include "nlohmann/json.hpp"

auto makeClog() {
    auto pathProvider = clog::model::LocalFSLogFilePathProvider{};
    auto repo = std::make_shared<clog::model::LocalLogRepository>(pathProvider);
    auto view = std::make_shared<clog::view::YearView>(clog::date::Date::getToday());
    auto editor = std::make_shared<clog::editor::EnvBasedEditor>(pathProvider);
    return clog::App{view, repo, editor};
}

int main() try {
    makeClog().run();
    return 0;
} catch (std::exception &e) {
    std::cout << "clog encountered an error: \n " << e.what() << std::endl;
    return 1;
}
