#pragma once

#include <algorithm>
#include <cstdlib>
#include <string>

namespace caps_log::utils {

[[nodiscard]] inline std::string trim(std::string str, const std::string &whitespace = " \t\n") {
    auto notSpace = [](int chah) { return !std::isspace(chah); };
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), notSpace));
    str.erase(std::find_if(str.rbegin(), str.rend(), notSpace).base(), str.end());
    return str;
}

[[nodiscard]] inline std::string lowercase(std::string data) {
    std::ranges::transform(data, data.begin(), [](char letter) { return std::tolower(letter); });
    return data;
}

bool parseInt(const std::string &str, int &out) {
    static constexpr int kBase10 = 10;
    static constexpr long kIntMax = 0x7fffffff;
    char *chr = nullptr;
    long val = std::strtol(str.c_str(), &chr, kBase10);
    if (chr == str.c_str() || *chr != '\0') {
        return false;
    }

    if (val < 0 || val > kIntMax) {
        return false;
    }
    out = static_cast<int>(val);
    return true;
}

} // namespace caps_log::utils
