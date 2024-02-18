#pragma once

#include <algorithm>
#include <string>

namespace caps_log::utils {

inline std::string trim(const std::string &str, const std::string &whitespace = " \t\n") {
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos) {
        return ""; // no content
    }

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

inline std::string lowercase(std::string data) {
    std::ranges::transform(data, data.begin(), [](char letter) { return std::tolower(letter); });
    return data;
}

} // namespace caps_log::utils
