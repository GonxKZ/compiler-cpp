/**
 * @file EnvironmentDetector.cpp
 * @brief Implementación de la detección del entorno SDK/CRT de Windows
 */

#include <compiler/common/EnvironmentDetector.h>
#include <algorithm>
#include <sstream>
#include <regex>
#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <knownfolders.h>
#include <comutil.h>
#pragma comment(lib, "shell32.lib")
#endif

namespace cpp20::compiler {

// ============================================================================
// EnvironmentDetector - Implementación
// ============================================================================

EnvironmentDetector::EnvironmentDetector() : preferredArch_("x64") {
}

EnvironmentDetector::~EnvironmentDetector() = default;

DetectedEnvironment EnvironmentDetector::detectEnvironment(const std::string& targetArch) {
    DetectedEnvironment env;
    env.targetArchitecture = getCanonicalArchitecture(targetArch);

    // Detectar MSVC
    auto msvcOpt = findMSVCInstallation(env.targetArchitecture);
    if (msvcOpt) {
        env.msvcVersion = *msvcOpt;
        env.msvcInstallPath = msvcOpt->installPath;
    }

    // Detectar Windows SDK
    auto sdkOpt = findWindowsSDK();
    if (sdkOpt) {
        env.windowsSDK = *sdkOpt;
        env.sdkInstallPath = sdkOpt->installPath;
    }

    // Verificar compatibilidad
    if (msvcOpt && sdkOpt && areVersionsCompatible(*msvcOpt, *sdkOpt)) {
        env.isValid = true;

        // Obtener rutas estándar
        env.includePaths = getStandardIncludePaths(*msvcOpt, *sdkOpt, env.targetArchitecture);
        env.libraryPaths = getStandardLibraryPaths(*msvcOpt, *sdkOpt, env.targetArchitecture);
        env.preprocessorDefinitions = getStandardPreprocessorDefinitions(*msvcOpt, *sdkOpt);
    } else {
        env.isValid = false;
    }

    return env;
}

std::optional<MSVCVersion> EnvironmentDetector::findMSVCInstallation(const std::string& targetArch) {
    auto versions = listAvailableMSVCVersions();
    if (versions.empty()) {
        return std::nullopt;
    }

    return selectBestMSVCVersion(versions);
}

std::optional<SDKVersion> EnvironmentDetector::findWindowsSDK() {
    auto versions = listAvailableSDKVersions();
    if (versions.empty()) {
        return std::nullopt;
    }

    return selectBestSDKVersion(versions);
}

std::vector<MSVCVersion> EnvironmentDetector::listAvailableMSVCVersions() {
    std::vector<MSVCVersion> versions;

    // Escanear registro
    auto registryVersions = scanRegistryForMSVC();
    versions.insert(versions.end(), registryVersions.begin(), registryVersions.end());

    // Escanear directorios
    auto directoryVersions = scanDirectoriesForMSVC();
    versions.insert(versions.end(), directoryVersions.begin(), directoryVersions.end());

    // Eliminar duplicados
    std::sort(versions.begin(), versions.end());
    auto last = std::unique(versions.begin(), versions.end());
    versions.erase(last, versions.end());

    return versions;
}

std::vector<SDKVersion> EnvironmentDetector::listAvailableSDKVersions() {
    return scanDirectoriesForSDK();
}

std::vector<MSVCVersion> EnvironmentDetector::scanRegistryForMSVC() {
    std::vector<MSVCVersion> versions;

#ifdef _WIN32
    // En una implementación real, aquí se escanearía el registro de Windows
    // para encontrar instalaciones de Visual Studio

    // Placeholder: buscar en ubicaciones estándar
    std::vector<std::filesystem::path> searchPaths = {
        "C:/Program Files/Microsoft Visual Studio/2022",
        "C:/Program Files (x86)/Microsoft Visual Studio/2019",
        "C:/Program Files (x86)/Microsoft Visual Studio/2017"
    };

    for (const auto& basePath : searchPaths) {
        if (std::filesystem::exists(basePath)) {
            for (const auto& entry : std::filesystem::directory_iterator(basePath)) {
                if (entry.is_directory()) {
                    auto version = extractMSVCVersionFromPath(entry.path());
                    if (version.major > 0 && validateMSVCInstallation(entry.path(), preferredArch_)) {
                        versions.push_back(version);
                    }
                }
            }
        }
    }
#endif

    return versions;
}

std::vector<MSVCVersion> EnvironmentDetector::scanDirectoriesForMSVC() {
    std::vector<MSVCVersion> versions;

    std::vector<std::filesystem::path> searchPaths = {
        "C:/Program Files/Microsoft Visual Studio",
        "C:/Program Files (x86)/Microsoft Visual Studio"
    };

    for (const auto& basePath : searchPaths) {
        if (std::filesystem::exists(basePath)) {
            try {
                for (const auto& entry : std::filesystem::directory_iterator(basePath)) {
                    if (entry.is_directory()) {
                        auto version = extractMSVCVersionFromPath(entry.path());
                        if (version.major > 0 && validateMSVCInstallation(entry.path(), preferredArch_)) {
                            versions.push_back(version);
                        }
                    }
                }
            } catch (const std::exception&) {
                // Ignorar errores de acceso
            }
        }
    }

    return versions;
}

std::vector<SDKVersion> EnvironmentDetector::scanDirectoriesForSDK() {
    std::vector<SDKVersion> versions;

    std::vector<std::filesystem::path> searchPaths = {
        "C:/Program Files (x86)/Windows Kits/10",
        "C:/Program Files/Windows Kits/10"
    };

    for (const auto& basePath : searchPaths) {
        if (std::filesystem::exists(basePath)) {
            try {
                std::filesystem::path includePath = basePath / "Include";
                if (std::filesystem::exists(includePath)) {
                    for (const auto& entry : std::filesystem::directory_iterator(includePath)) {
                        if (entry.is_directory()) {
                            auto version = extractSDKVersionFromPath(entry.path());
                            if (version.major > 0 && validateSDKInstallation(entry.path())) {
                                versions.push_back(version);
                            }
                        }
                    }
                }
            } catch (const std::exception&) {
                // Ignorar errores de acceso
            }
        }
    }

    return versions;
}

MSVCVersion EnvironmentDetector::extractMSVCVersionFromPath(const std::filesystem::path& path) {
    MSVCVersion version;
    version.installPath = path;

    std::string filename = path.filename().string();

    // Intentar extraer versión del nombre del directorio
    std::regex versionRegex(R"((\d{4})/?)");
    std::smatch match;
    if (std::regex_search(filename, match, versionRegex)) {
        version.major = std::stoi(match[1].str());
        version.fullVersion = match[1].str();

        // Buscar toolchain más reciente
        std::filesystem::path vcPath = path / "VC/Tools/MSVC";
        if (std::filesystem::exists(vcPath)) {
            std::filesystem::path latestToolchain;
            for (const auto& entry : std::filesystem::directory_iterator(vcPath)) {
                if (entry.is_directory() && entry.path().filename().string().find(".") != std::string::npos) {
                    if (latestToolchain.empty() || entry.path() > latestToolchain) {
                        latestToolchain = entry.path();
                    }
                }
            }

            if (!latestToolchain.empty()) {
                version.toolchainVersion = latestToolchain.filename().string();
                // Parsear versión del toolchain
                std::string toolchainStr = version.toolchainVersion;
                std::regex toolchainRegex(R"((\d+)\.(\d+)\.(\d+)\.?(\d+)?)");
                if (std::regex_search(toolchainStr, match, toolchainRegex)) {
                    version.minor = std::stoi(match[1].str());
                    version.build = std::stoi(match[2].str());
                    if (match.size() > 4 && match[4].matched) {
                        version.revision = std::stoi(match[4].str());
                    }
                }
            }
        }
    }

    return version;
}

SDKVersion EnvironmentDetector::extractSDKVersionFromPath(const std::filesystem::path& path) {
    SDKVersion version;
    version.installPath = path;

    std::string filename = path.filename().string();
    version.fullVersion = filename;

    // Parsear versión del SDK
    std::regex sdkRegex(R"((\d+)\.(\d+)\.?(\d+)?\.?(\d+)?)");
    std::smatch match;
    if (std::regex_search(filename, match, sdkRegex)) {
        version.major = std::stoi(match[1].str());
        version.minor = std::stoi(match[2].str());
        if (match.size() > 3 && match[3].matched) {
            version.build = std::stoi(match[3].str());
        }
    }

    return version;
}

bool EnvironmentDetector::validateMSVCInstallation(const std::filesystem::path& path,
                                                 const std::string& targetArch) {
    // Verificar archivos clave
    std::vector<std::string> requiredFiles = {
        "VC/Tools/MSVC",
        "VC/Auxiliary/Build",
        "Common7/Tools"
    };

    for (const auto& file : requiredFiles) {
        if (!std::filesystem::exists(path / file)) {
            return false;
        }
    }

    // Verificar toolchain específico de arquitectura
    std::filesystem::path toolchainPath = path / "VC/Tools/MSVC";
    if (std::filesystem::exists(toolchainPath)) {
        // Encontrar toolchain
        std::filesystem::path latestToolchain;
        for (const auto& entry : std::filesystem::directory_iterator(toolchainPath)) {
            if (entry.is_directory()) {
                latestToolchain = entry.path();
                break; // Usar el primero encontrado
            }
        }

        if (!latestToolchain.empty()) {
            std::string archDir = (targetArch == "x64") ? "x64" : "x86";
            std::filesystem::path binPath = latestToolchain / "bin" / ("Host" + archDir) / archDir;
            return std::filesystem::exists(binPath / "cl.exe");
        }
    }

    return false;
}

bool EnvironmentDetector::validateSDKInstallation(const std::filesystem::path& path) {
    // Verificar archivos clave del SDK
    std::vector<std::string> requiredFiles = {
        "Include/um/windows.h",
        "Include/shared/winerror.h",
        "Lib/um/x64/kernel32.lib"
    };

    for (const auto& file : requiredFiles) {
        if (!std::filesystem::exists(path / file)) {
            return false;
        }
    }

    return true;
}

std::vector<std::filesystem::path> EnvironmentDetector::getStandardIncludePaths(
    const MSVCVersion& msvc,
    const SDKVersion& sdk,
    const std::string& targetArch) {

    std::vector<std::filesystem::path> paths;

    // MSVC includes
    if (!msvc.installPath.empty()) {
        std::filesystem::path vcPath = msvc.installPath / "VC/Tools/MSVC";
        if (!msvc.toolchainVersion.empty()) {
            vcPath /= msvc.toolchainVersion;
        }

        paths.push_back(vcPath / "include");
        paths.push_back(vcPath / "atlmfc/include");
    }

    // Windows SDK includes
    if (!sdk.installPath.empty()) {
        std::filesystem::path sdkPath = sdk.installPath;
        paths.push_back(sdkPath / "Include" / "um");
        paths.push_back(sdkPath / "Include" / "ucrt");
        paths.push_back(sdkPath / "Include" / "shared");
        paths.push_back(sdkPath / "Include" / "winrt");
    }

    return paths;
}

std::vector<std::filesystem::path> EnvironmentDetector::getStandardLibraryPaths(
    const MSVCVersion& msvc,
    const SDKVersion& sdk,
    const std::string& targetArch) {

    std::vector<std::filesystem::path> paths;

    // MSVC libraries
    if (!msvc.installPath.empty()) {
        std::filesystem::path vcPath = msvc.installPath / "VC/Tools/MSVC";
        if (!msvc.toolchainVersion.empty()) {
            vcPath /= msvc.toolchainVersion;
        }

        std::string archDir = (targetArch == "x64") ? "x64" : "x86";
        paths.push_back(vcPath / "lib" / archDir);
        paths.push_back(vcPath / "atlmfc/lib" / archDir);
    }

    // Windows SDK libraries
    if (!sdk.installPath.empty()) {
        std::string archDir = (targetArch == "x64") ? "x64" : "x86";
        std::filesystem::path sdkPath = sdk.installPath;
        paths.push_back(sdkPath / "Lib" / sdk.fullVersion / "um" / archDir);
        paths.push_back(sdkPath / "Lib" / sdk.fullVersion / "ucrt" / archDir);
    }

    return paths;
}

std::vector<std::string> EnvironmentDetector::getStandardPreprocessorDefinitions(
    const MSVCVersion& msvc,
    const SDKVersion& sdk) {

    std::vector<std::string> defines = {
        "_WIN32",
        "_MSC_VER=" + std::to_string(msvc.major * 100 + msvc.minor),
        "_MSC_FULL_VER=" + std::to_string(msvc.major * 10000000 + msvc.minor * 100000 +
                                         msvc.build * 100 + msvc.revision),
        "_WIN64",  // Para x64
        "WIN32_LEAN_AND_MEAN",
        "NOMINMAX"
    };

    // Definiciones específicas de SDK
    if (sdk.major >= 10) {
        defines.push_back("NTDDI_VERSION=NTDDI_WIN10_RS5");
        defines.push_back("WINVER=_WIN32_WINNT_WIN10");
        defines.push_back("_WIN32_WINNT=_WIN32_WINNT_WIN10");
    }

    return defines;
}

bool EnvironmentDetector::validateEnvironment(const DetectedEnvironment& env) {
    if (!env.isValid) return false;

    // Verificar que las rutas existen
    for (const auto& path : env.includePaths) {
        if (!std::filesystem::exists(path)) {
            return false;
        }
    }

    for (const auto& path : env.libraryPaths) {
        if (!std::filesystem::exists(path)) {
            return false;
        }
    }

    // Verificar archivos clave
    std::vector<std::string> keyFiles = {
        "windows.h", "stdio.h", "stdlib.h", "string.h"
    };

    for (const auto& file : keyFiles) {
        if (!findFileInEnvironment(file, env).has_value()) {
            return false;
        }
    }

    return true;
}

std::string EnvironmentDetector::generateCompilerConfig(const DetectedEnvironment& env) {
    std::stringstream ss;

    ss << "# Auto-generated compiler configuration\n";
    ss << "# Generated for architecture: " << env.targetArchitecture << "\n";
    ss << "# MSVC Version: " << env.msvcVersion.toString() << "\n";
    ss << "# Windows SDK: " << env.windowsSDK.toString() << "\n\n";

    // Include paths
    ss << "INCLUDE_PATHS=";
    for (size_t i = 0; i < env.includePaths.size(); ++i) {
        if (i > 0) ss << ";";
        ss << env.includePaths[i].string();
    }
    ss << "\n";

    // Library paths
    ss << "LIBRARY_PATHS=";
    for (size_t i = 0; i < env.libraryPaths.size(); ++i) {
        if (i > 0) ss << ";";
        ss << env.libraryPaths[i].string();
    }
    ss << "\n";

    // Preprocessor definitions
    ss << "PREPROCESSOR_DEFINITIONS=";
    for (size_t i = 0; i < env.preprocessorDefinitions.size(); ++i) {
        if (i > 0) ss << ";";
        ss << env.preprocessorDefinitions[i];
    }
    ss << "\n";

    return ss.str();
}

std::optional<std::filesystem::path> EnvironmentDetector::findFileInEnvironment(
    const std::string& filename,
    const DetectedEnvironment& env,
    const std::vector<std::string>& searchExtensions) {

    for (const auto& includePath : env.includePaths) {
        for (const auto& ext : searchExtensions) {
            std::filesystem::path filePath = includePath / (filename + ext);
            if (std::filesystem::exists(filePath)) {
                return filePath;
            }
        }

        // También buscar sin extensión
        std::filesystem::path filePath = includePath / filename;
        if (std::filesystem::exists(filePath)) {
            return filePath;
        }
    }

    return std::nullopt;
}

std::string EnvironmentDetector::getCompilerVersionString(const std::filesystem::path& compilerPath) {
    // En una implementación real, ejecutaría cl.exe /? y parsearía la salida
    return "Microsoft (R) C/C++ Optimizing Compiler Version 19.30.30709 for x64";
}

MSVCVersion EnvironmentDetector::selectBestMSVCVersion(const std::vector<MSVCVersion>& versions) {
    if (versions.empty()) {
        return MSVCVersion();
    }

    // Seleccionar la versión más reciente
    MSVCVersion best = versions[0];
    for (const auto& version : versions) {
        if (version > best) {
            best = version;
        }
    }

    return best;
}

SDKVersion EnvironmentDetector::selectBestSDKVersion(const std::vector<SDKVersion>& versions) {
    if (versions.empty()) {
        return SDKVersion();
    }

    // Seleccionar la versión más reciente
    SDKVersion best = versions[0];
    for (const auto& version : versions) {
        if (version > best) {
            best = version;
        }
    }

    return best;
}

bool EnvironmentDetector::areVersionsCompatible(const MSVCVersion& msvc, const SDKVersion& sdk) {
    // MSVC 2019+ requiere SDK 10.0+
    if (msvc.major >= 16 && sdk.major < 10) {
        return false;
    }

    // MSVC 2017 requiere SDK 10.0.14393+
    if (msvc.major == 15 && sdk.major == 10 && sdk.build < 14393) {
        return false;
    }

    return true;
}

std::string EnvironmentDetector::getCanonicalArchitecture(const std::string& arch) {
    if (arch == "x86_64" || arch == "amd64") {
        return "x64";
    } else if (arch == "i386" || arch == "i686") {
        return "x86";
    } else if (arch == "arm64") {
        return "arm64";
    }

    return arch;
}

// ============================================================================
// EnvironmentUtils - Implementación
// ============================================================================

std::string EnvironmentUtils::executeCommand(const std::string& command) {
#ifdef _WIN32
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);

    if (!pipe) {
        return "";
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result;
#else
    return ""; // Placeholder para otros sistemas
#endif
}

bool EnvironmentUtils::isExecutableInPath(const std::string& executable) {
    std::string command = "where " + executable + " 2>nul";
    std::string result = executeCommand(command);
    return !result.empty();
}

std::optional<std::string> EnvironmentUtils::getEnvironmentVariable(const std::string& name) {
#ifdef _WIN32
    char* value = nullptr;
    size_t size = 0;
    if (_dupenv_s(&value, &size, name.c_str()) == 0 && value != nullptr) {
        std::string result(value);
        free(value);
        return result;
    }
#endif
    return std::nullopt;
}

bool EnvironmentUtils::setEnvironmentVariable(const std::string& name, const std::string& value) {
#ifdef _WIN32
    return _putenv_s(name.c_str(), value.c_str()) == 0;
#else
    return false;
#endif
}

std::string EnvironmentUtils::expandEnvironmentVariables(const std::string& input) {
    std::string result = input;

#ifdef _WIN32
    char buffer[4096];
    if (ExpandEnvironmentStringsA(input.c_str(), buffer, sizeof(buffer))) {
        result = buffer;
    }
#endif

    return result;
}

std::filesystem::path EnvironmentUtils::getExecutableDirectory() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
#else
    return std::filesystem::current_path();
#endif
}

std::filesystem::path EnvironmentUtils::getTempDirectory() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetTempPathA(MAX_PATH, buffer);
    return std::filesystem::path(buffer);
#else
    return std::filesystem::temp_directory_path();
#endif
}

std::filesystem::path EnvironmentUtils::generateTempFileName(const std::string& prefix,
                                                           const std::string& extension) {
    auto tempDir = getTempDirectory();
    std::string filename = prefix + "XXXXXX" + extension;

    // En una implementación real, usaríamos mkstemp o similar
    return tempDir / filename;
}

bool EnvironmentUtils::hasWritePermission(const std::filesystem::path& directory) {
    if (!std::filesystem::exists(directory)) {
        return false;
    }

    try {
        auto testFile = directory / "test_write.tmp";
        std::ofstream file(testFile);
        bool canWrite = file.is_open();
        file.close();
        std::filesystem::remove(testFile);
        return canWrite;
    } catch (const std::exception&) {
        return false;
    }
}

std::string EnvironmentUtils::getOSVersion() {
#ifdef _WIN32
    // En una implementación real, usaríamos GetVersionEx o similar
    return "Windows 10/11";
#else
    return "Unknown";
#endif
}

bool EnvironmentUtils::isCIEnvironment() {
    // Verificar variables de entorno comunes de CI
    std::vector<std::string> ciVars = {
        "CI", "CONTINUOUS_INTEGRATION", "TRAVIS", "APPVEYOR",
        "CIRCLECI", "GITHUB_ACTIONS", "GITLAB_CI", "JENKINS_HOME"
    };

    for (const auto& var : ciVars) {
        if (getEnvironmentVariable(var).has_value()) {
            return true;
        }
    }

    return false;
}

// ============================================================================
// CompilerConfigManager - Implementación
// ============================================================================

CompilerConfigManager::CompilerConfigManager() = default;

bool CompilerConfigManager::loadConfig(const std::filesystem::path& configFile) {
    // En una implementación real, parsearía JSON o similar
    return false; // Placeholder
}

bool CompilerConfigManager::saveConfig(const std::filesystem::path& configFile,
                                     const DetectedEnvironment& env) {
    try {
        std::ofstream file(configFile);
        if (!file.is_open()) {
            return false;
        }

        file << serializeConfigToJSON(env);
        file.close();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

DetectedEnvironment CompilerConfigManager::createDefaultConfig() {
    EnvironmentDetector detector;
    return detector.detectEnvironment();
}

bool CompilerConfigManager::validateConfig(const DetectedEnvironment& env) {
    EnvironmentDetector detector;
    return detector.validateEnvironment(env);
}

void CompilerConfigManager::updateConfig(DetectedEnvironment& existing,
                                       const DetectedEnvironment& detected) {
    // Actualizar campos detectados
    existing.msvcVersion = detected.msvcVersion;
    existing.windowsSDK = detected.windowsSDK;
    existing.msvcInstallPath = detected.msvcInstallPath;
    existing.sdkInstallPath = detected.sdkInstallPath;
    existing.includePaths = detected.includePaths;
    existing.libraryPaths = detected.libraryPaths;
    existing.preprocessorDefinitions = detected.preprocessorDefinitions;
    existing.isValid = detected.isValid;
}

std::string CompilerConfigManager::serializeConfigToJSON(const DetectedEnvironment& env) {
    std::stringstream ss;

    ss << "{\n";
    ss << "  \"msvc_version\": \"" << env.msvcVersion.toString() << "\",\n";
    ss << "  \"windows_sdk\": \"" << env.windowsSDK.toString() << "\",\n";
    ss << "  \"target_architecture\": \"" << env.targetArchitecture << "\",\n";
    ss << "  \"is_valid\": " << (env.isValid ? "true" : "false") << ",\n";

    ss << "  \"include_paths\": [\n";
    for (size_t i = 0; i < env.includePaths.size(); ++i) {
        ss << "    \"" << env.includePaths[i].string() << "\"";
        if (i < env.includePaths.size() - 1) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";

    ss << "  \"library_paths\": [\n";
    for (size_t i = 0; i < env.libraryPaths.size(); ++i) {
        ss << "    \"" << env.libraryPaths[i].string() << "\"";
        if (i < env.libraryPaths.size() - 1) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";

    ss << "  \"preprocessor_definitions\": [\n";
    for (size_t i = 0; i < env.preprocessorDefinitions.size(); ++i) {
        ss << "    \"" << env.preprocessorDefinitions[i] << "\"";
        if (i < env.preprocessorDefinitions.size() - 1) ss << ",";
        ss << "\n";
    }
    ss << "  ]\n";
    ss << "}\n";

    return ss.str();
}

} // namespace cpp20::compiler
