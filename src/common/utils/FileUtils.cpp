/**
 * @file FileUtils.cpp
 * @brief Utilidades para manipulación de archivos
 */

#include <compiler/common/utils/FileUtils.h>
#include <filesystem>
#include <fstream>

namespace cpp20::compiler::common::utils {
namespace fs = std::filesystem;

// ========================================================================
// Funciones de manipulación de archivos
// ========================================================================

bool fileExists(const std::string& path) {
    return fs::exists(path);
}

bool directoryExists(const std::string& path) {
    return fs::exists(path) && fs::is_directory(path);
}

std::string getFileExtension(const std::string& path) {
    fs::path p(path);
    return p.extension().string();
}

std::string getFileName(const std::string& path) {
    fs::path p(path);
    return p.filename().string();
}

std::string getFileNameWithoutExtension(const std::string& path) {
    fs::path p(path);
    return p.stem().string();
}

std::string getDirectory(const std::string& path) {
    fs::path p(path);
    return p.parent_path().string();
}

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + path);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string content(static_cast<size_t>(size), '\0');
    if (!file.read(content.data(), size)) {
        throw std::runtime_error("Cannot read file: " + path);
    }

    return content;
}

void writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot create file: " + path);
    }

    file.write(content.data(), content.size());
    if (!file) {
        throw std::runtime_error("Cannot write to file: " + path);
    }
}

std::vector<std::string> listFiles(const std::string& directory, const std::string& extension) {
    std::vector<std::string> files;

    if (!directoryExists(directory)) {
        return files;
    }

    for (const auto& entry : fs::directory_iterator(directory)) {
        if (fs::is_regular_file(entry)) {
            std::string path = entry.path().string();
            if (extension.empty() || endsWith(path, extension)) {
                files.push_back(path);
            }
        }
    }

    return files;
}

} // namespace cpp20::compiler::common::utils
