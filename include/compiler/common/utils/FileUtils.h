#pragma once

#include <string>
#include <vector>

namespace cpp20::compiler::common::utils {

/**
 * @brief Utilidades para manipulación de archivos
 */

// File/directory operations
bool fileExists(const std::string& path);
bool directoryExists(const std::string& path);

// Path manipulation
std::string getFileExtension(const std::string& path);
std::string getFileName(const std::string& path);
std::string getFileNameWithoutExtension(const std::string& path);
std::string getDirectory(const std::string& path);

// File I/O
std::string readFile(const std::string& path);
void writeFile(const std::string& path, const std::string& content);

// Directory listing
std::vector<std::string> listFiles(const std::string& directory, const std::string& extension = "");

} // namespace cpp20::compiler::common::utils
