#pragma once

#include <istream>
#include <string>

namespace caps_log::utils {

std::string encrypt(const std::string &password, std::istream &file);
std::string decrypt(const std::string &password, std::istream &file);

} // namespace caps_log::utils
