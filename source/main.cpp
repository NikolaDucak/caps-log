#include <iostream>

#include "app.hpp"
#include "disabled_editor.hpp"
#include "env_based_editor.hpp"
#include "model/default_log_repository.hpp"
#include "model/dummy_repository.hpp"

/**
 * Temporary WASM impl with no permanent/real storage or editor capabilities
 */
auto makeWASMClog() {
    auto repo = std::make_shared<clog::model::DummyRepo>();
    auto view = std::make_shared<clog::view::YearView>(clog::model::Date::getToday());
    auto editor = std::make_shared<clog::DisabledEditor>();
    return clog::App { view, repo, editor };
}

auto makeTUIClog() {
    auto repo = std::make_shared<clog::model::DefaultLogRepository>();
    auto view = std::make_shared<clog::view::YearView>(clog::model::Date::getToday());
    auto editor = std::make_shared<clog::EnvBasedEditor>();
    return clog::App {view, repo, editor};
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
} catch(std::exception& e) {
    std::cout << "Error: " << e.what() << '\0'<< std::endl;
    return 1;
}
