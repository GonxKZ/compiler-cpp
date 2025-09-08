/**
 * @file LinkerIntegration.h
 * @brief Integración con Microsoft Linker (link.exe)
 */

#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include <unordered_map>

namespace cpp20::compiler::backend {

/**
 * @brief Configuración del linker
 */
struct LinkerConfig {
    std::filesystem::path linkerPath;          // Ruta a link.exe
    std::filesystem::path workingDirectory;    // Directorio de trabajo
    std::vector<std::string> defaultLibraries; // Librerías por defecto
    std::vector<std::string> defaultLibraryPaths; // Rutas de librerías por defecto
    std::string subsystem;                     // Subsistema (CONSOLE, WINDOWS)
    std::string machine;                       // Arquitectura (X64, X86)
    bool debugSymbols = false;                 // Incluir símbolos de debug
    bool incrementalLinking = false;           // Linking incremental
    std::string entryPoint;                    // Punto de entrada personalizado

    LinkerConfig() : subsystem("CONSOLE"), machine("X64") {}
};

/**
 * @brief Resultado de una operación de linking
 */
struct LinkResult {
    bool success = false;
    int exitCode = 0;
    std::string errorMessage;
    std::filesystem::path outputFile;
    std::vector<std::string> warnings;
    double linkTime = 0.0;

    LinkResult() = default;
    LinkResult(bool s, int code, const std::string& msg = "")
        : success(s), exitCode(code), errorMessage(msg) {}
};

/**
 * @brief Información de un archivo objeto
 */
struct ObjectFileInfo {
    std::filesystem::path path;
    std::vector<std::string> definedSymbols;
    std::vector<std::string> undefinedSymbols;
    std::vector<std::string> dependencies;

    ObjectFileInfo(const std::filesystem::path& p) : path(p) {}
};

/**
 * @brief Integración con Microsoft Linker
 */
class LinkerIntegration {
public:
    /**
     * @brief Constructor
     */
    LinkerIntegration(const LinkerConfig& config = LinkerConfig());

    /**
     * @brief Destructor
     */
    ~LinkerIntegration();

    /**
     * @brief Configura el linker
     */
    void setConfig(const LinkerConfig& config) { config_ = config; }

    /**
     * @brief Obtiene la configuración actual
     */
    const LinkerConfig& getConfig() const { return config_; }

    /**
     * @brief Detecta automáticamente la instalación de Visual Studio
     */
    bool detectVisualStudioInstallation();

    /**
     * @brief Genera ejecutable a partir de archivos objeto
     */
    LinkResult linkExecutable(
        const std::vector<std::filesystem::path>& objectFiles,
        const std::filesystem::path& outputFile,
        const std::vector<std::string>& libraries = {},
        const std::vector<std::string>& libraryPaths = {});

    /**
     * @brief Genera DLL a partir de archivos objeto
     */
    LinkResult linkDLL(
        const std::vector<std::filesystem::path>& objectFiles,
        const std::filesystem::path& outputFile,
        const std::vector<std::string>& libraries = {},
        const std::vector<std::string>& libraryPaths = {});

    /**
     * @brief Genera biblioteca estática (.lib)
     */
    LinkResult linkStaticLibrary(
        const std::vector<std::filesystem::path>& objectFiles,
        const std::filesystem::path& outputFile);

    /**
     * @brief Verifica si un archivo objeto es válido
     */
    bool validateObjectFile(const std::filesystem::path& objectFile);

    /**
     * @brief Obtiene información de dependencias de un objeto
     */
    ObjectFileInfo getObjectFileInfo(const std::filesystem::path& objectFile);

    /**
     * @brief Ejecuta link.exe con argumentos personalizados
     */
    LinkResult executeLinker(const std::vector<std::string>& args);

    /**
     * @brief Obtiene la línea de comandos completa para link.exe
     */
    std::string getLinkCommandLine(
        const std::vector<std::filesystem::path>& objectFiles,
        const std::filesystem::path& outputFile,
        const std::vector<std::string>& libraries,
        const std::vector<std::string>& libraryPaths,
        bool isDLL = false) const;

    /**
     * @brief Lista las librerías estándar disponibles
     */
    std::vector<std::string> getAvailableLibraries() const;

    /**
     * @brief Verifica si el linker está disponible
     */
    bool isLinkerAvailable() const;

    /**
     * @brief Obtiene la versión del linker
     */
    std::string getLinkerVersion();

private:
    LinkerConfig config_;

    /**
     * @brief Construye argumentos por defecto para el linker
     */
    std::vector<std::string> buildDefaultArguments(
        const std::filesystem::path& outputFile,
        bool isDLL = false) const;

    /**
     * @brief Añade archivos objeto a los argumentos
     */
    void addObjectFiles(
        std::vector<std::string>& args,
        const std::vector<std::filesystem::path>& objectFiles) const;

    /**
     * @brief Añade librerías a los argumentos
     */
    void addLibraries(
        std::vector<std::string>& args,
        const std::vector<std::string>& libraries) const;

    /**
     * @brief Añade rutas de librerías a los argumentos
     */
    void addLibraryPaths(
        std::vector<std::string>& args,
        const std::vector<std::string>& libraryPaths) const;

    /**
     * @brief Ejecuta un proceso y captura la salida
     */
    LinkResult executeProcess(const std::string& command);

    /**
     * @brief Parsea la salida del linker para extraer errores y warnings
     */
    void parseLinkerOutput(const std::string& output, LinkResult& result);

    /**
     * @brief Encuentra la instalación de Visual Studio automáticamente
     */
    std::filesystem::path findVisualStudioInstallation();

    /**
     * @brief Configura rutas por defecto basadas en la instalación encontrada
     */
    void setupDefaultPaths(const std::filesystem::path& vsPath);

    /**
     * @brief Verifica si un directorio contiene una instalación válida de VS
     */
    bool isValidVSInstallation(const std::filesystem::path& path) const;

    /**
     * @brief Obtiene la ruta del linker para una arquitectura específica
     */
    std::filesystem::path getLinkerPathForArchitecture(const std::string& arch) const;
};

/**
 * @brief Utilidades para trabajar con archivos PE/COFF
 */
class PEUtils {
public:
    /**
     * @brief Verifica si un archivo es un objeto COFF válido
     */
    static bool isValidCOFFFile(const std::filesystem::path& file);

    /**
     * @brief Extrae símbolos de un archivo objeto
     */
    static std::vector<std::string> extractSymbols(const std::filesystem::path& objectFile);

    /**
     * @brief Verifica dependencias de un objeto
     */
    static std::vector<std::string> getDependencies(const std::filesystem::path& objectFile);

    /**
     * @brief Obtiene el machine type de un objeto COFF
     */
    static std::string getMachineType(const std::filesystem::path& objectFile);

    /**
     * @brief Verifica si un objeto tiene información de debug
     */
    static bool hasDebugInfo(const std::filesystem::path& objectFile);
};

} // namespace cpp20::compiler::backend
