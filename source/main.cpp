#include <iostream>

#include "app.hpp"
#include "date/date.hpp"
#include "editor/disabled_editor.hpp"
#include "editor/env_based_editor.hpp"
#include "model/local_log_repository.hpp"
#include "model/dummy_repository.hpp"
#include "nlohmann/json.hpp"

/**
 * Temporary WASM impl with no permanent/real storage or editor capabilities
 */
auto makeWASMClog() {
    auto repo = std::make_shared<clog::model::DummyLogRepository>();
    auto view = std::make_shared<clog::view::YearView>(clog::date::Date::getToday());
    auto editor = std::make_shared<clog::editor::DisabledEditor>();
    return clog::App{view, repo, editor};
}

auto makeTUIClog() {
    auto repo = std::make_shared<clog::model::LocalLogRepository>(clog::model::LocalFSLogFilePathProvider{});
    auto view = std::make_shared<clog::view::YearView>(clog::date::Date::getToday());
    auto editor = std::make_shared<clog::editor::EnvBasedEditor>(clog::model::LocalFSLogFilePathProvider{});
    return clog::App{view, repo, editor};
}

auto makeClog() {
#ifdef WASM_BUILD
    return makeWASMClog();
#else
    return makeTUIClog();
#endif
}

int main() try {
    makeClog().run();
    return 0;
} catch (std::exception &e) {
    std::cout << "Error: " << e.what() << '\0' << std::endl;
    return 1;
}
