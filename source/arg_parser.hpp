#pragma once

#include <optional>
#include <stdexcept>
#include <string>

namespace clog {

/**
 * A simple wrapper around command line arguments providing simpler arg lookup functionality.
 */
class ArgParser {
    int argc;
    const char **argv;

  public:
    ArgParser() : argc{0}, argv{nullptr} {}
    ArgParser(int argc, const char **argv) : argc{argc}, argv{argv} {}

    bool has(const std::string &opt, const std::string &optAlt = "") const {
        auto it = (optAlt.empty()) ? std::find(argv, argv + argc, opt)
                                   : std::find_if(argv, argv + argc, [&](auto arg) {
                                         return arg == opt || arg == optAlt;
                                     });
        return it < argv + argc;
    }

    std::optional<std::string> get(const std::string &opt, const std::string &optAlt = "") const {
        auto it = (optAlt.empty()) ? std::find(argv, argv + argc, opt)
                                   : std::find_if(argv, argv + argc, [&](auto arg) {
                                         return arg == opt || arg == optAlt;
                                     });

        if (not(it < (argv + argc)))
            throw std::runtime_error{*it + std::string{" - option required but not provided"}};

        if (not((it + 1) < (argv + argc)))
            throw std::runtime_error{*it + std::string{" - has no value set, but requires one"}};

        return std::string{*(it + 1)};
    }

    std::optional<std::string> getIfHas(const std::string &opt,
                                        const std::string &optAlt = "") const {
        auto it = (optAlt.empty()) ? std::find(argv, argv + argc, opt)
                                   : std::find_if(argv, argv + argc, [&](auto arg) {
                                         return arg == opt || arg == optAlt;
                                     });

        if (not(it < (argv + argc)))
            return {};

        if (not((it + 1) < (argv + argc)))
            throw std::runtime_error{*it + std::string{" - has no value set, but requires one"}};

        return std::string{*(it + 1)};
    }
};

} // namespace clog
