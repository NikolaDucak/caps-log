#pragma once 

#include <string>
#include <istream>

namespace clog::utils {

std::string encryptFile(const std::string& password, std::istream& file); 
std::string decryptFile(const std::string& password, std::istream& file);

}

