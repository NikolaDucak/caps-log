#include <iostream>
#include <vector>

#include "caps_log.hpp"

int main(int argc, const char **argv) try {
    using namespace caps_log;

    std::vector<std::string> args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        args.emplace_back(argv[i]);
    }

    CapsLog capsLog{{
        .today = utils::date::getToday(),
        .cliArgs = args,
    }};

    auto task = capsLog.getTask();

    if (task.type == CapsLog::Task::Type::kRunAppplication) {
        task.action();
    } else if (task.type == CapsLog::Task::Type::kApplyCrypto) {
        std::cout << "Applying crypto...\n";
        task.action();
    } else {
        std::cerr << "Invalid command line arguments provided.\n";
        return 1;
    }

    return 0;
} catch (const std::exception &e) {
    std::cerr << "Captain's log encountered an error: \n " << e.what() << '\n' << std::flush;
    return 1;
}
