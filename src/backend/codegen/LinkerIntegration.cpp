/**
 * @file LinkerIntegration.cpp
 * @brief Implementación de la integración con Microsoft Linker
 */

#include <compiler/backend/codegen/LinkerIntegration.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <array>
#include <cstdio>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace cpp20::compiler::backend {

// ============================================================================
// LinkerIntegration - Implementación
// ============================================================================

LinkerIntegration::LinkerIntegration(const LinkerConfig& config)
    : config_(config) {
    // Intentar detectar VS automáticamente
    if (config_.linkerPath.empty()) {
        detectVisualStudioInstallation();
    }
}

LinkerIntegration::~LinkerIntegration() = default;

bool LinkerIntegration::detectVisualStudioInstallation() {
    auto vsPath = findVisualStudioInstallation();
    if (!vsPath.empty()) {
        setupDefaultPaths(vsPath);
        return true;
    }

    // Fallback: buscar en PATH
    config_.linkerPath = "link.exe";
    return isLinkerAvailable();
}

bool LinkerIntegration::isValidVSInstallation(const std::filesystem::path& path) const {
    // Verificar archivos clave de VS
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

    return true;
}

std::filesystem::path LinkerIntegration::findVisualStudioInstallation() {
    // Buscar en ubicaciones estándar de Visual Studio
    std::vector<std::filesystem::path> searchPaths = {
        "C:/Program Files/Microsoft Visual Studio/2022",
        "C:/Program Files (x86)/Microsoft Visual Studio/2019",
        "C:/Program Files (x86)/Microsoft Visual Studio/2017"
    };

    for (const auto& basePath : searchPaths) {
        if (std::filesystem::exists(basePath)) {
            for (const auto& entry : std::filesystem::directory_iterator(basePath)) {
                if (entry.is_directory()) {
                    auto vsPath = entry.path();
                    if (isValidVSInstallation(vsPath)) {
                        return vsPath;
                    }
                }
            }
        }
    }

    return {};
}

void LinkerIntegration::setupDefaultPaths(const std::filesystem::path& vsPath) {
    // Configurar ruta del linker
    config_.linkerPath = getLinkerPathForArchitecture(config_.machine);

    // Configurar librerías por defecto
    config_.defaultLibraries = {
        "kernel32.lib",
        "user32.lib",
        "advapi32.lib",
        "msvcrt.lib",
        "vcruntime.lib",
        "ucrt.lib"
    };

    // Configurar rutas de librerías
    std::filesystem::path vcPath = vsPath / "VC/Tools/MSVC";
    if (std::filesystem::exists(vcPath)) {
        // Encontrar la versión más reciente de MSVC
        std::filesystem::path latestVersion;
        for (const auto& entry : std::filesystem::directory_iterator(vcPath)) {
            if (entry.is_directory() && entry.path().filename().string().find(".") != std::string::npos) {
                if (latestVersion.empty() || entry.path() > latestVersion) {
                    latestVersion = entry.path();
                }
            }
        }

        if (!latestVersion.empty()) {
            config_.defaultLibraryPaths.push_back((latestVersion / "lib/x64").string());
        }
    }

    // SDK de Windows
    std::vector<std::filesystem::path> sdkPaths = {
        "C:/Program Files (x86)/Windows Kits/10/Lib",
        "C:/Program Files/Windows Kits/10/Lib"
    };

    for (const auto& sdkBase : sdkPaths) {
        if (std::filesystem::exists(sdkBase)) {
            // Encontrar la versión más reciente del SDK
            std::filesystem::path latestSDK;
            for (const auto& entry : std::filesystem::directory_iterator(sdkBase)) {
                if (entry.is_directory() && entry.path().filename().string().find(".") != std::string::npos) {
                    if (latestSDK.empty() || entry.path() > latestSDK) {
                        latestSDK = entry.path();
                    }
                }
            }

            if (!latestSDK.empty()) {
                config_.defaultLibraryPaths.push_back((latestSDK / "um/x64").string());
                config_.defaultLibraryPaths.push_back((latestSDK / "ucrt/x64").string());
            }
        }
    }
}

std::filesystem::path LinkerIntegration::getLinkerPathForArchitecture(const std::string& arch) const {
    // En un sistema real, esto buscaría la ruta correcta basada en la arquitectura
    // Por simplicidad, asumimos que link.exe está en PATH
    return "link.exe";
}

LinkResult LinkerIntegration::linkExecutable(
    const std::vector<std::filesystem::path>& objectFiles,
    const std::filesystem::path& outputFile,
    const std::vector<std::string>& libraries,
    const std::vector<std::string>& libraryPaths) {

    if (!isLinkerAvailable()) {
        return LinkResult(false, -1, "Linker no disponible");
    }

    // Validar archivos objeto
    for (const auto& obj : objectFiles) {
        if (!validateObjectFile(obj)) {
            return LinkResult(false, -1, "Archivo objeto inválido: " + obj.string());
        }
    }

    // Construir línea de comandos
    auto commandLine = getLinkCommandLine(objectFiles, outputFile, libraries, libraryPaths, false);

    // Ejecutar linker
    return executeProcess(commandLine);
}

LinkResult LinkerIntegration::linkDLL(
    const std::vector<std::filesystem::path>& objectFiles,
    const std::filesystem::path& outputFile,
    const std::vector<std::string>& libraries,
    const std::vector<std::string>& libraryPaths) {

    if (!isLinkerAvailable()) {
        return LinkResult(false, -1, "Linker no disponible");
    }

    // Construir línea de comandos con flag /DLL
    auto commandLine = getLinkCommandLine(objectFiles, outputFile, libraries, libraryPaths, true);

    // Ejecutar linker
    return executeProcess(commandLine);
}

LinkResult LinkerIntegration::linkStaticLibrary(
    const std::vector<std::filesystem::path>& objectFiles,
    const std::filesystem::path& outputFile) {

    // Para librerías estáticas, usar lib.exe en lugar de link.exe
    std::string libPath = "lib.exe"; // Asumir que está en PATH

    std::stringstream cmd;
    cmd << "\"" << libPath << "\" /OUT:\"" << outputFile.string() << "\"";

    for (const auto& obj : objectFiles) {
        cmd << " \"" << obj.string() << "\"";
    }

    return executeProcess(cmd.str());
}

bool LinkerIntegration::validateObjectFile(const std::filesystem::path& objectFile) {
    if (!std::filesystem::exists(objectFile)) {
        return false;
    }

    // Verificar extensión
    std::string ext = objectFile.extension().string();
    if (ext != ".obj" && ext != ".o") {
        return false;
    }

    // Verificar que sea un archivo regular
    return std::filesystem::is_regular_file(objectFile);
}

ObjectFileInfo LinkerIntegration::getObjectFileInfo(const std::filesystem::path& objectFile) {
    ObjectFileInfo info(objectFile);

    // En un compilador real, aquí se usaría dumpbin.exe o similar
    // para extraer información de símbolos del archivo objeto

    // Placeholder: agregar algunos símbolos comunes
    info.definedSymbols = {"main", "_mainCRTStartup"};
    info.undefinedSymbols = {"printf", "malloc", "free"};

    return info;
}

LinkResult LinkerIntegration::executeLinker(const std::vector<std::string>& args) {
    std::stringstream cmd;
    cmd << "\"" << config_.linkerPath.string() << "\"";

    for (const auto& arg : args) {
        cmd << " " << arg;
    }

    return executeProcess(cmd.str());
}

std::string LinkerIntegration::getLinkCommandLine(
    const std::vector<std::filesystem::path>& objectFiles,
    const std::filesystem::path& outputFile,
    const std::vector<std::string>& libraries,
    const std::vector<std::string>& libraryPaths,
    bool isDLL) const {

    std::stringstream cmd;

    // Output file
    cmd << "/OUT:\"" << outputFile.string() << "\"";

    // Machine type
    cmd << " /MACHINE:" << config_.machine;

    // Subsystem
    cmd << " /SUBSYSTEM:" << config_.subsystem;

    // Entry point
    if (!config_.entryPoint.empty()) {
        cmd << " /ENTRY:" << config_.entryPoint;
    }

    // Debug symbols
    if (config_.debugSymbols) {
        cmd << " /DEBUG";
    }

    // Incremental linking
    if (config_.incrementalLinking) {
        cmd << " /INCREMENTAL";
    }

    // DLL flag
    if (isDLL) {
        cmd << " /DLL";
    }

    // Object files
    for (const auto& obj : objectFiles) {
        cmd << " \"" << obj.string() << "\"";
    }

    // Libraries
    auto allLibraries = libraries;
    allLibraries.insert(allLibraries.end(), config_.defaultLibraries.begin(), config_.defaultLibraries.end());

    for (const auto& lib : allLibraries) {
        cmd << " " << lib;
    }

    // Library paths
    auto allLibPaths = libraryPaths;
    allLibPaths.insert(allLibPaths.end(), config_.defaultLibraryPaths.begin(), config_.defaultLibraryPaths.end());

    for (const auto& path : allLibPaths) {
        cmd << " /LIBPATH:\"" << path << "\"";
    }

    return cmd.str();
}

std::vector<std::string> LinkerIntegration::getAvailableLibraries() const {
    std::vector<std::string> availableLibs = config_.defaultLibraries;

    // En un compilador real, aquí se escanearían los directorios de librerías
    // para encontrar todas las librerías disponibles

    return availableLibs;
}

bool LinkerIntegration::isLinkerAvailable() const {
    // Verificar si el ejecutable existe
    if (!config_.linkerPath.empty() && std::filesystem::exists(config_.linkerPath)) {
        return true;
    }

    // Verificar si está en PATH
    return system("link.exe /? >nul 2>&1") == 0;
}

std::string LinkerIntegration::getLinkerVersion() {
    if (!isLinkerAvailable()) {
        return "Linker no disponible";
    }

    // Ejecutar link.exe para obtener versión
    auto result = executeProcess("\"" + config_.linkerPath.string() + "\" /?");

    if (result.success) {
        // Extraer información de versión de la salida
        // Esto sería más sofisticado en un compilador real
        return "Microsoft Linker (versión detectada)";
    }

    return "Error al obtener versión";
}

LinkResult LinkerIntegration::executeProcess(const std::string& command) {
    LinkResult result;
    auto startTime = std::chrono::high_resolution_clock::now();

#ifdef _WIN32
    // Usar CreateProcess en Windows para mejor control
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };

    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    // Crear pipes para capturar salida
    HANDLE hStdOutRead, hStdOutWrite;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
        result.success = false;
        result.errorMessage = "Error al crear pipe";
        return result;
    }

    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdOutWrite;

    // Convertir command string a char*
    std::vector<char> cmd(command.begin(), command.end());
    cmd.push_back('\0');

    if (!CreateProcessA(NULL, cmd.data(), NULL, NULL, TRUE,
                       0, NULL, config_.workingDirectory.string().c_str(),
                       &si, &pi)) {
        result.success = false;
        result.errorMessage = "Error al ejecutar proceso: " + std::to_string(GetLastError());
        CloseHandle(hStdOutRead);
        CloseHandle(hStdOutWrite);
        return result;
    }

    // Esperar a que termine
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Obtener código de salida
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    result.exitCode = static_cast<int>(exitCode);
    result.success = (exitCode == 0);

    // Leer salida
    std::string output;
    char buffer[4096];
    DWORD bytesRead;
    while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }

    // Parsear salida
    parseLinkerOutput(output, result);

    // Limpiar handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hStdOutRead);
    CloseHandle(hStdOutWrite);

#else
    // Para otros sistemas (no implementado en este ejemplo)
    result.success = false;
    result.errorMessage = "Plataforma no soportada";
#endif

    auto endTime = std::chrono::high_resolution_clock::now();
    result.linkTime = std::chrono::duration<double>(endTime - startTime).count();

    return result;
}

void LinkerIntegration::parseLinkerOutput(const std::string& output, LinkResult& result) {
    // Parsear la salida del linker para extraer errores y warnings
    std::stringstream ss(output);
    std::string line;

    while (std::getline(ss, line)) {
        // Detectar errores
        if (line.find("error") != std::string::npos ||
            line.find("Error") != std::string::npos) {
            result.errorMessage += line + "\n";
        }
        // Detectar warnings
        else if (line.find("warning") != std::string::npos ||
                 line.find("Warning") != std::string::npos) {
            result.warnings.push_back(line);
        }
    }
}

// ============================================================================
// PEUtils - Implementación
// ============================================================================

bool PEUtils::isValidCOFFFile(const std::filesystem::path& file) {
    // Verificar que el archivo existe
    if (!std::filesystem::exists(file) || !std::filesystem::is_regular_file(file)) {
        return false;
    }

    // En un compilador real, aquí se verificaría la estructura PE/COFF
    // leyendo el header del archivo

    // Verificación simplificada por extensión
    std::string ext = file.extension().string();
    return ext == ".obj" || ext == ".lib" || ext == ".dll" || ext == ".exe";
}

std::vector<std::string> PEUtils::extractSymbols(const std::filesystem::path& objectFile) {
    // En un compilador real, aquí se usaría dumpbin.exe /symbols
    // o se parsearía directamente el archivo COFF

    std::vector<std::string> symbols;

    // Placeholder: devolver símbolos comunes
    if (objectFile.filename().string().find("main") != std::string::npos) {
        symbols = {"main", "_main", "printf", "puts"};
    }

    return symbols;
}

std::vector<std::string> PEUtils::getDependencies(const std::filesystem::path& objectFile) {
    // En un compilador real, aquí se analizarían las dependencias del objeto
    std::vector<std::string> deps;

    // Placeholder
    deps = {"kernel32.dll", "msvcrt.dll"};

    return deps;
}

std::string PEUtils::getMachineType(const std::filesystem::path& objectFile) {
    // En un compilador real, aquí se leería el machine type del header COFF
    return "AMD64"; // Placeholder para x64
}

bool PEUtils::hasDebugInfo(const std::filesystem::path& objectFile) {
    // En un compilador real, aquí se verificaría si hay información de debug
    // en las secciones del archivo PE/COFF
    return false; // Placeholder
}

} // namespace cpp20::compiler::backend
