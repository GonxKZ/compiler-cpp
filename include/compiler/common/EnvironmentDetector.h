/**
 * @file EnvironmentDetector.h
 * @brief Detección reproducible del entorno SDK/CRT de Windows
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <memory>
#include <optional>

namespace cpp20::compiler {

/**
 * @brief Información de versión del SDK
 */
struct SDKVersion {
    int major;
    int minor;
    int build;
    std::string fullVersion;
    std::filesystem::path installPath;

    SDKVersion(int maj = 0, int min = 0, int bld = 0, const std::string& ver = "",
              const std::filesystem::path& path = "")
        : major(maj), minor(min), build(bld), fullVersion(ver), installPath(path) {}

    bool operator<(const SDKVersion& other) const {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return build < other.build;
    }

    bool operator==(const SDKVersion& other) const {
        return major == other.major && minor == other.minor && build == other.build;
    }

    std::string toString() const {
        return fullVersion.empty() ? std::to_string(major) + "." + std::to_string(minor) :
                                   fullVersion;
    }
};

/**
 * @brief Información de versión de MSVC
 */
struct MSVCVersion {
    int major;
    int minor;
    int build;
    int revision;
    std::string fullVersion;
    std::filesystem::path installPath;
    std::string toolchainVersion;

    MSVCVersion(int maj = 0, int min = 0, int bld = 0, int rev = 0,
               const std::string& ver = "", const std::filesystem::path& path = "",
               const std::string& toolchain = "")
        : major(maj), minor(min), build(bld), revision(rev), fullVersion(ver),
          installPath(path), toolchainVersion(toolchain) {}

    bool operator<(const MSVCVersion& other) const {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        if (build != other.build) return build < other.build;
        return revision < other.revision;
    }

    std::string toString() const {
        return fullVersion.empty() ?
            std::to_string(major) + "." + std::to_string(minor) + "." +
            std::to_string(build) + "." + std::to_string(revision) : fullVersion;
    }
};

/**
 * @brief Información del entorno de compilación detectado
 */
struct DetectedEnvironment {
    MSVCVersion msvcVersion;
    SDKVersion windowsSDK;
    std::filesystem::path msvcInstallPath;
    std::filesystem::path sdkInstallPath;
    std::vector<std::filesystem::path> includePaths;
    std::vector<std::filesystem::path> libraryPaths;
    std::vector<std::string> preprocessorDefinitions;
    std::string targetArchitecture;
    bool isValid;

    DetectedEnvironment()
        : targetArchitecture("x64"), isValid(false) {}
};

/**
 * @brief Detector de entorno Windows SDK/CRT
 */
class EnvironmentDetector {
public:
    /**
     * @brief Constructor
     */
    EnvironmentDetector();

    /**
     * @brief Destructor
     */
    ~EnvironmentDetector();

    /**
     * @brief Detecta automáticamente el entorno de compilación
     */
    DetectedEnvironment detectEnvironment(const std::string& targetArch = "x64");

    /**
     * @brief Busca instalación de Visual Studio
     */
    std::optional<MSVCVersion> findMSVCInstallation(const std::string& targetArch = "x64");

    /**
     * @brief Busca instalación de Windows SDK
     */
    std::optional<SDKVersion> findWindowsSDK();

    /**
     * @brief Obtiene rutas de include estándar
     */
    std::vector<std::filesystem::path> getStandardIncludePaths(
        const MSVCVersion& msvc,
        const SDKVersion& sdk,
        const std::string& targetArch = "x64");

    /**
     * @brief Obtiene rutas de librerías estándar
     */
    std::vector<std::filesystem::path> getStandardLibraryPaths(
        const MSVCVersion& msvc,
        const SDKVersion& sdk,
        const std::string& targetArch = "x64");

    /**
     * @brief Obtiene definiciones de preprocesador estándar
     */
    std::vector<std::string> getStandardPreprocessorDefinitions(
        const MSVCVersion& msvc,
        const SDKVersion& sdk);

    /**
     * @brief Valida que el entorno detectado sea funcional
     */
    bool validateEnvironment(const DetectedEnvironment& env);

    /**
     * @brief Genera configuración de compilador para el entorno
     */
    std::string generateCompilerConfig(const DetectedEnvironment& env);

    /**
     * @brief Busca archivos en rutas del entorno
     */
    std::optional<std::filesystem::path> findFileInEnvironment(
        const std::string& filename,
        const DetectedEnvironment& env,
        const std::vector<std::string>& searchExtensions = {".h", ".hpp"});

    /**
     * @brief Obtiene versión de compilador de línea de comandos
     */
    std::string getCompilerVersionString(const std::filesystem::path& compilerPath);

    /**
     * @brief Lista todas las versiones disponibles de MSVC
     */
    std::vector<MSVCVersion> listAvailableMSVCVersions();

    /**
     * @brief Lista todas las versiones disponibles de Windows SDK
     */
    std::vector<SDKVersion> listAvailableSDKVersions();

    /**
     * @brief Establece arquitectura de destino preferida
     */
    void setPreferredArchitecture(const std::string& arch) { preferredArch_ = arch; }

    /**
     * @brief Obtiene arquitectura de destino preferida
     */
    std::string getPreferredArchitecture() const { return preferredArch_; }

private:
    std::string preferredArch_;

    /**
     * @brief Escanea registro de Windows para encontrar VS
     */
    std::vector<MSVCVersion> scanRegistryForMSVC();

    /**
     * @brief Escanea directorios estándar para encontrar VS
     */
    std::vector<MSVCVersion> scanDirectoriesForMSVC();

    /**
     * @brief Escanea directorios estándar para encontrar Windows SDK
     */
    std::vector<SDKVersion> scanDirectoriesForSDK();

    /**
     * @brief Extrae versión de MSVC de la ruta de instalación
     */
    MSVCVersion extractMSVCVersionFromPath(const std::filesystem::path& path);

    /**
     * @brief Extrae versión de SDK de la ruta de instalación
     */
    SDKVersion extractSDKVersionFromPath(const std::filesystem::path& path);

    /**
     * @brief Valida instalación de MSVC
     */
    bool validateMSVCInstallation(const std::filesystem::path& path,
                                const std::string& targetArch);

    /**
     * @brief Valida instalación de Windows SDK
     */
    bool validateSDKInstallation(const std::filesystem::path& path);

    /**
     * @brief Lee archivo de configuración de VS
     */
    std::unordered_map<std::string, std::string> readVSConfigFile(
        const std::filesystem::path& configFile);

    /**
     * @brief Obtiene variables de entorno de VS
     */
    std::unordered_map<std::string, std::string> getVSEnvironmentVariables(
        const std::filesystem::path& vsPath,
        const std::string& targetArch);

    /**
     * @brief Normaliza ruta de Windows
     */
    std::filesystem::path normalizeWindowsPath(const std::filesystem::path& path);

    /**
     * @brief Verifica si una ruta existe y es accesible
     */
    bool isPathAccessible(const std::filesystem::path& path);

    /**
     * @brief Obtiene arquitectura canónica
     */
    std::string getCanonicalArchitecture(const std::string& arch);

    /**
     * @brief Determina la mejor versión de MSVC disponible
     */
    MSVCVersion selectBestMSVCVersion(const std::vector<MSVCVersion>& versions);

    /**
     * @brief Determina la mejor versión de SDK disponible
     */
    SDKVersion selectBestSDKVersion(const std::vector<SDKVersion>& versions);

    /**
     * @brief Verifica compatibilidad entre versiones de MSVC y SDK
     */
    bool areVersionsCompatible(const MSVCVersion& msvc, const SDKVersion& sdk);
};

/**
 * @brief Utilidades para trabajar con el entorno de compilación
 */
class EnvironmentUtils {
public:
    /**
     * @brief Ejecuta comando y captura salida
     */
    static std::string executeCommand(const std::string& command);

    /**
     * @brief Verifica si un ejecutable existe en PATH
     */
    static bool isExecutableInPath(const std::string& executable);

    /**
     * @brief Obtiene variable de entorno
     */
    static std::optional<std::string> getEnvironmentVariable(const std::string& name);

    /**
     * @brief Establece variable de entorno
     */
    static bool setEnvironmentVariable(const std::string& name, const std::string& value);

    /**
     * @brief Expande variables de entorno en una cadena
     */
    static std::string expandEnvironmentVariables(const std::string& input);

    /**
     * @brief Obtiene directorio del ejecutable actual
     */
    static std::filesystem::path getExecutableDirectory();

    /**
     * @brief Obtiene directorio temporal del sistema
     */
    static std::filesystem::path getTempDirectory();

    /**
     * @brief Genera nombre de archivo temporal único
     */
    static std::filesystem::path generateTempFileName(const std::string& prefix = "",
                                                     const std::string& extension = "");

    /**
     * @brief Verifica permisos de escritura en un directorio
     */
    static bool hasWritePermission(const std::filesystem::path& directory);

    /**
     * @brief Obtiene información del sistema operativo
     */
    static std::string getOSVersion();

    /**
     * @brief Verifica si estamos ejecutando en un entorno CI/CD
     */
    static bool isCIEnvironment();
};

/**
 * @brief Gestor de configuración de compilador
 */
class CompilerConfigManager {
public:
    /**
     * @brief Constructor
     */
    CompilerConfigManager();

    /**
     * @brief Carga configuración desde archivo
     */
    bool loadConfig(const std::filesystem::path& configFile);

    /**
     * @brief Guarda configuración a archivo
     */
    bool saveConfig(const std::filesystem::path& configFile,
                   const DetectedEnvironment& env);

    /**
     * @brief Genera configuración por defecto
     */
    DetectedEnvironment createDefaultConfig();

    /**
     * @brief Valida configuración existente
     */
    bool validateConfig(const DetectedEnvironment& env);

    /**
     * @brief Actualiza configuración con nueva detección
     */
    void updateConfig(DetectedEnvironment& existing, const DetectedEnvironment& detected);

private:
    std::filesystem::path configFile_;

    /**
     * @brief Parsea archivo de configuración JSON
     */
    DetectedEnvironment parseConfigFile(const std::filesystem::path& file);

    /**
     * @brief Serializa configuración a JSON
     */
    std::string serializeConfigToJSON(const DetectedEnvironment& env);
};

} // namespace cpp20::compiler
