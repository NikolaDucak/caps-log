#pragma once 

#include <string>
#include <istream>

namespace clog::utils {

std::string encrypt(const std::string& password, std::istream& file); 
std::string decrypt(const std::string& password, std::istream& file);

}

