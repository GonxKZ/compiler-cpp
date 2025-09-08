/**
 * @file MiniLinker.h
 * @brief Mini-linker PE/COFF propio para resolución de símbolos
 */

#pragma once

#include "compiler/backend/coff/COFFTypes.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <filesystem>
#include <cstdint>
#include <string>

// ========================================================================
// Definiciones directas de tipos COFF para evitar problemas de namespace
// ========================================================================

namespace cpp20::compiler::backend {

// Forward declarations and type definitions
namespace coff {

// COFF Symbol structure
struct COFFSymbol {
    std::string name;
    uint32_t value = 0;
    int16_t sectionNumber = 0;
    uint16_t type = 0;
    uint8_t storageClass = 0;
    uint8_t auxSymbols = 0;

    COFFSymbol(std::string n, uint8_t storage = 2) // IMAGE_SYM_CLASS_EXTERNAL = 2
        : name(std::move(n)), storageClass(storage) {}
};

// COFF File Header structure
struct IMAGE_FILE_HEADER {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
};

// COFF Section Header structure
struct IMAGE_SECTION_HEADER {
    char Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
};

} // namespace coff
} // namespace cpp20::compiler::backend

namespace cpp20::compiler::backend::link {

// ========================================================================
// Tipos auxiliares para el linker
// ========================================================================

/**
 * @brief Alias para el header COFF estándar
 */
using COFFHeader = cpp20::compiler::backend::coff::IMAGE_FILE_HEADER;

/**
 * @brief Alias para el header de sección COFF estándar
 */
using SectionHeader = cpp20::compiler::backend::coff::IMAGE_SECTION_HEADER;

/**
 * @brief Alias para el símbolo COFF estándar
 */
using COFFSymbol = cpp20::compiler::backend::coff::COFFSymbol;

/**
 * @brief Información de relocalización extendida
 */
struct RelocationInfo {
    uint32_t virtualAddress;
    uint32_t symbolIndex;
    uint16_t type;
    std::string symbolName;
    uint32_t addend;
    uint32_t sectionOffset;

    RelocationInfo(uint32_t vaddr = 0, uint32_t symIdx = 0, uint16_t t = 0,
                   const std::string& symName = "", uint32_t add = 0, uint32_t sectOff = 0)
        : virtualAddress(vaddr), symbolIndex(symIdx), type(t), symbolName(symName),
          addend(add), sectionOffset(sectOff) {}
};


/**
 * @brief Información de un símbolo en el proceso de linking
 */
struct SymbolInfo {
    std::string name;
    uint32_t value;
    int16_t sectionNumber;  // Cambiado a int16_t para ser consistente con COFF
    uint16_t type;
    uint8_t storageClass;
    bool isDefined;
    bool isExternal;
    bool isWeak;
    std::string moduleName;  // Archivo objeto donde se define

    SymbolInfo(const std::string& n = "", uint32_t val = 0, int16_t sect = 0)
        : name(n), value(val), sectionNumber(sect), type(0), storageClass(0),
          isDefined(false), isExternal(false), isWeak(false) {}
};

/**
 * @brief Información de una sección en el proceso de linking
 */
struct SectionInfo {
    std::string name;
    std::vector<uint8_t> data;
    uint32_t virtualAddress;
    uint32_t rawSize;
    uint32_t virtualSize;
    uint32_t characteristics;
    std::vector<RelocationInfo> relocations;
    bool isBSS;  // Sección sin inicialización

    SectionInfo(const std::string& n = "")
        : name(n), virtualAddress(0), rawSize(0), virtualSize(0),
          characteristics(0), isBSS(false) {}
};

/**
 * @brief Información de un archivo objeto
 */
struct ObjectFileInfo {
    std::filesystem::path path;
    std::vector<SectionInfo> sections;
    std::vector<SymbolInfo> symbols;
    std::string machineType;
    bool isValid;

    ObjectFileInfo(const std::filesystem::path& p)
        : path(p), isValid(false) {}
};

/**
 * @brief Información de import de biblioteca
 */
struct ImportInfo {
    std::string dllName;
    std::vector<std::string> functionNames;
    std::vector<uint16_t> hintOrdinals;

    ImportInfo(const std::string& dll = "")
        : dllName(dll) {}
};

/**
 * @brief Resultado del proceso de linking
 */
struct LinkResult {
    bool success;
    std::string errorMessage;
    std::filesystem::path outputFile;
    std::vector<std::string> warnings;
    std::unordered_map<std::string, uint32_t> symbolAddresses;
    size_t imageSize;
    uint32_t entryPoint;

    LinkResult(bool s = false, const std::string& msg = "")
        : success(s), errorMessage(msg), imageSize(0), entryPoint(0) {}
};

/**
 * @brief Mini-linker PE/COFF
 */
class MiniLinker {
public:
    /**
     * @brief Constructor
     */
    MiniLinker();

    /**
     * @brief Destructor
     */
    ~MiniLinker();

    /**
     * @brief Añade un archivo objeto al proceso de linking
     */
    bool addObjectFile(const std::filesystem::path& objectFile);

    /**
     * @brief Añade una biblioteca para importar
     */
    bool addLibrary(const std::filesystem::path& libraryFile);

    /**
     * @brief Establece el punto de entrada
     */
    void setEntryPoint(const std::string& entryPoint);

    /**
     * @brief Establece el tipo de subsistema
     */
    void setSubsystem(const std::string& subsystem);

    /**
     * @brief Establece la arquitectura de destino
     */
    void setMachineType(const std::string& machine);

    /**
     * @brief Establece el directorio base para el image
     */
    void setImageBase(uint64_t imageBase);

    /**
     * @brief Habilita/deshabilita optimizaciones de linking
     */
    void setOptimize(bool optimize);

    /**
     * @brief Realiza el proceso completo de linking
     */
    LinkResult link(const std::filesystem::path& outputFile);

    /**
     * @brief Obtiene información de símbolos sin resolver
     */
    std::vector<std::string> getUndefinedSymbols() const;

    /**
     * @brief Obtiene estadísticas del proceso de linking
     */
    std::unordered_map<std::string, size_t> getLinkStatistics() const;

    /**
     * @brief Limpia el estado del linker
     */
    void clear();

private:
    // Archivos objeto
    std::vector<ObjectFileInfo> objectFiles_;

    // Símbolos globales
    std::unordered_map<std::string, SymbolInfo> globalSymbols_;

    // Secciones combinadas
    std::vector<SectionInfo> combinedSections_;

    // Imports
    std::vector<ImportInfo> imports_;

    // Configuración
    std::string entryPoint_;
    std::string subsystem_;
    std::string machineType_;
    uint64_t imageBase_;
    bool optimize_;
    uint32_t sectionAlignment_;
    uint32_t fileAlignment_;

    // Estadísticas
    size_t totalSymbols_;
    size_t resolvedSymbols_;
    size_t totalRelocations_;

    /**
     * @brief Parsea un archivo objeto COFF
     */
    bool parseObjectFile(ObjectFileInfo& objInfo);

    /**
     * @brief Parsea un archivo de biblioteca
     */
    bool parseLibraryFile(const std::filesystem::path& libraryFile);

    /**
     * @brief Construye la tabla de símbolos global
     */
    void buildGlobalSymbolTable();

    /**
     * @brief Resuelve referencias de símbolos
     */
    bool resolveSymbols();

    /**
     * @brief Combina secciones de todos los archivos objeto
     */
    void combineSections();

    /**
     * @brief Aplica relocations
     */
    bool applyRelocations();

    /**
     * @brief Asigna direcciones virtuales
     */
    void assignVirtualAddresses();

    /**
     * @brief Crea el header PE
     */
    std::vector<uint8_t> createPEHeader(uint32_t entryPointRVA);

    /**
     * @brief Crea la tabla de secciones
     */
    std::vector<uint8_t> createSectionTable();

    /**
     * @brief Crea el directorio de imports
     */
    std::vector<uint8_t> createImportDirectory();

    /**
     * @brief Crea el directorio de exports (si es DLL)
     */
    std::vector<uint8_t> createExportDirectory();

    /**
     * @brief Crea la tabla de relocations base
     */
    std::vector<uint8_t> createBaseRelocations();

    /**
     * @brief Escribe el archivo PE final
     */
    bool writePEFile(const std::filesystem::path& outputFile,
                    const std::vector<uint8_t>& peHeader,
                    const std::vector<uint8_t>& sectionTable,
                    const std::vector<uint8_t>& importDirectory,
                    const std::vector<uint8_t>& exportDirectory,
                    const std::vector<uint8_t>& baseRelocations);

    /**
     * @brief Verifica compatibilidad de archivos objeto
     */
    bool checkObjectCompatibility(const ObjectFileInfo& obj) const;

    /**
     * @brief Resuelve conflictos de símbolos
     */
    bool resolveSymbolConflicts();

    /**
     * @brief Optimiza el layout de secciones
     */
    void optimizeSectionLayout();

    /**
     * @brief Calcula el tamaño del image
     */
    uint32_t calculateImageSize() const;

    /**
     * @brief Calcula checksum del PE
     */
    uint32_t calculatePEChecksum(const std::vector<uint8_t>& data) const;

    /**
     * @brief Obtiene RVA de un símbolo
     */
    uint32_t getSymbolRVA(const std::string& symbolName) const;

    /**
     * @brief Verifica si un símbolo es definido externamente
     */
    bool isExternallyDefined(const std::string& symbolName) const;

    /**
     * @brief Añade símbolos de runtime estándar
     */
    void addRuntimeSymbols();

    /**
     * @brief Maneja símbolos débiles
     */
    void handleWeakSymbols();

    /**
     * @brief Actualiza estadísticas
     */
    void updateStatistics();
};

/**
 * @brief Lector de archivos objeto COFF
 */
class COFFReader {
public:
    /**
     * @brief Lee un archivo objeto COFF
     */
    static bool readObjectFile(const std::filesystem::path& filePath,
                              ObjectFileInfo& objInfo);

    /**
     * @brief Valida formato COFF
     */
    static bool validateCOFFFormat(const std::filesystem::path& filePath);

    /**
     * @brief Extrae símbolos de un archivo COFF
     */
    static std::vector<SymbolInfo> extractSymbols(const std::vector<uint8_t>& data,
                                                 const COFFHeader& header);

    /**
     * @brief Extrae secciones de un archivo COFF
     */
    static std::vector<SectionInfo> extractSections(const std::vector<uint8_t>& data,
                                                   const COFFHeader& header);

    /**
     * @brief Extrae relocations de una sección
     */
    static std::vector<RelocationInfo> extractRelocations(const std::vector<uint8_t>& data,
                                                         const SectionHeader& section,
                                                         uint32_t symbolCount);

private:
    /**
     * @brief Lee el header COFF
     */
    static bool readCOFFHeader(std::ifstream& file, COFFHeader& header);

    /**
     * @brief Lee headers de sección
     */
    static bool readSectionHeaders(std::ifstream& file, const COFFHeader& coffHeader,
                                  std::vector<SectionHeader>& sectionHeaders);

    /**
     * @brief Lee tabla de símbolos
     */
    static bool readSymbolTable(std::ifstream& file, const COFFHeader& coffHeader,
                               std::vector<COFFSymbol>& symbols,
                               std::vector<std::string>& stringTable);

    /**
     * @brief Lee datos de sección
     */
    static bool readSectionData(std::ifstream& file, const SectionHeader& section,
                               std::vector<uint8_t>& data);

    /**
     * @brief Lee relocations de sección
     */
    static bool readSectionRelocations(std::ifstream& file, const SectionHeader& section,
                                      std::vector<RelocationInfo>& relocations);
};

/**
 * @brief Escritor de archivos PE
 */
class PEWriter {
public:
    /**
     * @brief Escribe un archivo PE completo
     */
    static bool writePEFile(const std::filesystem::path& outputFile,
                           const std::vector<uint8_t>& dosHeader,
                           const std::vector<uint8_t>& peHeader,
                           const std::vector<uint8_t>& sectionTable,
                           const std::vector<std::vector<uint8_t>>& sectionData,
                           const std::vector<uint8_t>& importDirectory,
                           const std::vector<uint8_t>& exportDirectory,
                           const std::vector<uint8_t>& baseRelocations);

    /**
     * @brief Crea header DOS stub
     */
    static std::vector<uint8_t> createDOSStub();

    /**
     * @brief Actualiza checksum en el header PE
     */
    static void updatePEChecksum(std::vector<uint8_t>& peHeader, uint32_t checksum);

    /**
     * @brief Alinea datos a boundary específico
     */
    static std::vector<uint8_t> alignData(const std::vector<uint8_t>& data,
                                         size_t alignment,
                                         uint8_t fillByte = 0);

    /**
     * @brief Calcula tamaño total del archivo
     */
    static uint32_t calculateFileSize(const std::vector<uint8_t>& dosHeader,
                                     const std::vector<uint8_t>& peHeader,
                                     const std::vector<uint8_t>& sectionTable,
                                     const std::vector<std::vector<uint8_t>>& sectionData,
                                     size_t optionalDataSize);

private:
    /**
     * @brief Escribe datos a archivo con alineación
     */
    static bool writeAlignedData(std::ofstream& file,
                                const std::vector<uint8_t>& data,
                                size_t alignment,
                                uint8_t fillByte = 0);
};

/**
 * @brief Resolvedor de símbolos
 */
class SymbolResolver {
public:
    /**
     * @brief Resuelve un símbolo en la tabla global
     */
    static bool resolveSymbol(const std::string& symbolName,
                             const std::unordered_map<std::string, SymbolInfo>& globalSymbols,
                             uint32_t& resolvedAddress);

    /**
     * @brief Encuentra símbolos sin resolver
     */
    static std::vector<std::string> findUndefinedSymbols(
        const std::unordered_map<std::string, SymbolInfo>& globalSymbols);

    /**
     * @brief Verifica conflictos de símbolos
     */
    static std::vector<std::string> findSymbolConflicts(
        const std::unordered_map<std::string, SymbolInfo>& globalSymbols);

    /**
     * @brief Resuelve símbolos débiles
     */
    static void resolveWeakSymbols(
        std::unordered_map<std::string, SymbolInfo>& globalSymbols);

    /**
     * @brief Obtiene símbolos exportados
     */
    static std::vector<std::string> getExportedSymbols(
        const std::unordered_map<std::string, SymbolInfo>& globalSymbols);

    /**
     * @brief Verifica si un símbolo es válido para linking
     */
    static bool isSymbolValidForLinking(const SymbolInfo& symbol);

private:
    /**
     * @brief Lista de símbolos de runtime estándar
     */
    static const std::unordered_set<std::string>& getRuntimeSymbols();
};

/**
 * @brief Aplicador de relocations
 */
class RelocationApplier {
public:
    /**
     * @brief Aplica una relocation a los datos de sección
     */
    static bool applyRelocation(std::vector<uint8_t>& sectionData,
                               const RelocationInfo& relocation,
                               uint32_t symbolAddress,
                               uint32_t sectionRVA);

    /**
     * @brief Aplica todas las relocations de una sección
     */
    static bool applySectionRelocations(std::vector<uint8_t>& sectionData,
                                       const std::vector<RelocationInfo>& relocations,
                                       const std::unordered_map<std::string, SymbolInfo>& globalSymbols,
                                       const std::vector<SectionInfo>& sections);

    /**
     * @brief Verifica que una relocation sea válida
     */
    static bool validateRelocation(const RelocationInfo& relocation,
                                  const SymbolInfo& symbol);

    /**
     * @brief Convierte tipo de relocation COFF a RVA
     */
    static uint32_t convertRelocationToRVA(uint16_t relocationType,
                                          uint32_t symbolAddress,
                                          uint32_t sectionRVA,
                                          uint32_t offset);

    /**
     * @brief Calcula el valor final de una relocation
     */
    static uint32_t calculateRelocationValue(uint16_t type,
                                            uint32_t symbolValue,
                                            uint32_t sectionRVA,
                                            uint32_t offset);

private:
    /**
     * @brief Aplica relocation IMAGE_REL_AMD64_ADDR32
     */
    static bool applyAddr32Relocation(std::vector<uint8_t>& data,
                                     size_t offset,
                                     uint32_t value);

    /**
     * @brief Aplica relocation IMAGE_REL_AMD64_ADDR64
     */
    static bool applyAddr64Relocation(std::vector<uint8_t>& data,
                                     size_t offset,
                                     uint64_t value);

    /**
     * @brief Aplica relocation IMAGE_REL_AMD64_REL32
     */
    static bool applyRel32Relocation(std::vector<uint8_t>& data,
                                    size_t offset,
                                    uint32_t value,
                                    uint32_t currentRVA);
};

} // namespace cpp20::compiler::backend::link
