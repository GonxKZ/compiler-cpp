#pragma once

#include <string>
#include <vector>
#include <sstream>

namespace cpp20::compiler::common::utils {

/**
 * @brief Utilidades para manipulación de strings
 */

// Case conversion
std::string toLower(const std::string& str);
std::string toUpper(const std::string& str);

// Trimming
std::string trim(const std::string& str);
std::string trimLeft(const std::string& str);
std::string trimRight(const std::string& str);

// Prefix/suffix checking
bool startsWith(const std::string& str, const std::string& prefix);
bool endsWith(const std::string& str, const std::string& suffix);

// Splitting and joining
std::vector<std::string> split(const std::string& str, char delimiter);
std::string join(const std::vector<std::string>& parts, const std::string& separator);

// String replacement
std::string replace(const std::string& str, const std::string& from, const std::string& to);

} // namespace cpp20::compiler::common::utils
