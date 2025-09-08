/**
 * @file MiniLinker.cpp
 * @brief Implementación del mini-linker PE/COFF propio
 */

#include <compiler/backend/link/MiniLinker.h>
#include <compiler/backend/coff/COFFTypes.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cstring>

namespace cpp20::compiler::backend::link {

// ============================================================================
// MiniLinker - Implementación
// ============================================================================

MiniLinker::MiniLinker()
    : entryPoint_("main"), subsystem_("CONSOLE"), machineType_("X64"),
      imageBase_(0x400000), optimize_(false), sectionAlignment_(0x1000),
      fileAlignment_(0x200), totalSymbols_(0), resolvedSymbols_(0), totalRelocations_(0) {
}

MiniLinker::~MiniLinker() = default;

bool MiniLinker::addObjectFile(const std::filesystem::path& objectFile) {
    ObjectFileInfo objInfo(objectFile);

    if (!parseObjectFile(objInfo)) {
        return false;
    }

    if (!checkObjectCompatibility(objInfo)) {
        return false;
    }

    objectFiles_.push_back(std::move(objInfo));
    return true;
}

bool MiniLinker::addLibrary(const std::filesystem::path& libraryFile) {
    return parseLibraryFile(libraryFile);
}

void MiniLinker::setEntryPoint(const std::string& entryPoint) {
    entryPoint_ = entryPoint;
}

void MiniLinker::setSubsystem(const std::string& subsystem) {
    subsystem_ = subsystem;
}

void MiniLinker::setMachineType(const std::string& machine) {
    machineType_ = machine;
}

void MiniLinker::setImageBase(uint64_t imageBase) {
    imageBase_ = imageBase;
}

void MiniLinker::setOptimize(bool optimize) {
    optimize_ = optimize;
}

LinkResult MiniLinker::link(const std::filesystem::path& outputFile) {
    LinkResult result;
    result.outputFile = outputFile;

    try {
        // Paso 1: Construir tabla de símbolos global
        buildGlobalSymbolTable();

        // Paso 2: Resolver símbolos
        if (!resolveSymbols()) {
            result.errorMessage = "Error resolviendo símbolos";
            return result;
        }

        // Paso 3: Combinar secciones
        combineSections();

        // Paso 4: Aplicar relocations
        if (!applyRelocations()) {
            result.errorMessage = "Error aplicando relocations";
            return result;
        }

        // Paso 5: Asignar direcciones virtuales
        assignVirtualAddresses();

        // Paso 6: Obtener RVA del entry point
        uint32_t entryPointRVA = getSymbolRVA(entryPoint_);
        if (entryPointRVA == 0 && !entryPoint_.empty()) {
            result.errorMessage = "Entry point '" + entryPoint_ + "' no encontrado";
            return result;
        }

        // Paso 7: Crear headers PE
        auto peHeader = createPEHeader(entryPointRVA);
        auto sectionTable = createSectionTable();
        auto importDirectory = createImportDirectory();
        auto exportDirectory = createExportDirectory();
        auto baseRelocations = createBaseRelocations();

        // Paso 8: Escribir archivo PE
        if (!writePEFile(outputFile, peHeader, sectionTable, importDirectory,
                        exportDirectory, baseRelocations)) {
            result.errorMessage = "Error escribiendo archivo PE";
            return result;
        }

        // Paso 9: Actualizar estadísticas
        updateStatistics();

        result.success = true;
        result.entryPoint = entryPointRVA;
        result.imageSize = calculateImageSize();

        // Llenar mapa de direcciones de símbolos
        for (const auto& [name, symbol] : globalSymbols_) {
            if (symbol.isDefined) {
                result.symbolAddresses[name] = symbol.value;
            }
        }

    } catch (const std::exception& e) {
        result.errorMessage = std::string("Excepción durante linking: ") + e.what();
    }

    return result;
}

std::vector<std::string> MiniLinker::getUndefinedSymbols() const {
    return SymbolResolver::findUndefinedSymbols(globalSymbols_);
}

std::unordered_map<std::string, size_t> MiniLinker::getLinkStatistics() const {
    return {
        {"total_symbols", totalSymbols_},
        {"resolved_symbols", resolvedSymbols_},
        {"undefined_symbols", totalSymbols_ - resolvedSymbols_},
        {"total_relocations", totalRelocations_},
        {"object_files", objectFiles_.size()},
        {"combined_sections", combinedSections_.size()}
    };
}

void MiniLinker::clear() {
    objectFiles_.clear();
    globalSymbols_.clear();
    combinedSections_.clear();
    imports_.clear();

    totalSymbols_ = 0;
    resolvedSymbols_ = 0;
    totalRelocations_ = 0;
}

bool MiniLinker::parseObjectFile(ObjectFileInfo& objInfo) {
    return COFFReader::readObjectFile(objInfo.path, objInfo);
}

bool MiniLinker::parseLibraryFile(const std::filesystem::path& libraryFile) {
    // En una implementación real, aquí se parsearía archivos .lib
    // Por simplicidad, asumimos que es válido
    ImportInfo import(libraryFile.filename().string());
    imports_.push_back(import);
    return true;
}

void MiniLinker::buildGlobalSymbolTable() {
    globalSymbols_.clear();

    // Añadir símbolos de runtime estándar
    addRuntimeSymbols();

    // Procesar símbolos de cada archivo objeto
    for (const auto& objFile : objectFiles_) {
        for (const auto& symbol : objFile.symbols) {
            auto it = globalSymbols_.find(symbol.name);

            if (it == globalSymbols_.end()) {
                // Nuevo símbolo
                globalSymbols_[symbol.name] = symbol;
            } else {
                // Resolver conflictos
                if (symbol.isDefined && !it->second.isDefined) {
                    // Nuevo símbolo definido, reemplaza indefinido
                    it->second = symbol;
                } else if (symbol.isDefined && it->second.isDefined) {
                    // Conflicto: dos definiciones
                    // En un linker real, esto sería un error
                    // Por simplicidad, usamos el último
                    it->second = symbol;
                }
            }
        }
    }

    totalSymbols_ = globalSymbols_.size();
}

bool MiniLinker::resolveSymbols() {
    // Resolver referencias cruzadas
    for (auto& [name, symbol] : globalSymbols_) {
        if (!symbol.isDefined && !symbol.isExternal) {
            // Buscar en otros archivos objeto
            for (const auto& objFile : objectFiles_) {
                auto it = std::find_if(objFile.symbols.begin(), objFile.symbols.end(),
                    [&name](const SymbolInfo& s) { return s.name == name && s.isDefined; });

                if (it != objFile.symbols.end()) {
                    symbol = *it;
                    symbol.isDefined = true;
                    resolvedSymbols_++;
                    break;
                }
            }
        } else if (symbol.isDefined) {
            resolvedSymbols_++;
        }
    }

    // Verificar símbolos débiles
    handleWeakSymbols();

    // Verificar conflictos
    return resolveSymbolConflicts();
}

void MiniLinker::combineSections() {
    std::unordered_map<std::string, SectionInfo> sectionMap;

    // Combinar secciones de todos los archivos objeto
    for (const auto& objFile : objectFiles_) {
        for (const auto& section : objFile.sections) {
            auto it = sectionMap.find(section.name);

            if (it == sectionMap.end()) {
                // Nueva sección
                sectionMap[section.name] = section;
            } else {
                // Combinar con sección existente
                auto& combined = it->second;

                // Concatenar datos
                combined.data.insert(combined.data.end(),
                                   section.data.begin(),
                                   section.data.end());

                // Combinar relocations (ajustar offsets)
                uint32_t offset = combined.rawSize;
                for (const auto& reloc : section.relocations) {
                    RelocationInfo adjustedReloc = reloc;
                    adjustedReloc.virtualAddress += offset;
                    combined.relocations.push_back(adjustedReloc);
                }

                // Actualizar tamaño
                combined.rawSize += section.rawSize;
                combined.virtualSize += section.virtualSize;

                // Combinar características
                combined.characteristics |= section.characteristics;
            }
        }
    }

    // Convertir mapa a vector
    combinedSections_.reserve(sectionMap.size());
    for (auto& [name, section] : sectionMap) {
        combinedSections_.push_back(std::move(section));
    }

    // Optimizar layout si está habilitado
    if (optimize_) {
        optimizeSectionLayout();
    }
}

bool MiniLinker::applyRelocations() {
    for (auto& section : combinedSections_) {
        if (!RelocationApplier::applySectionRelocations(
                section.data, section.relocations, globalSymbols_, combinedSections_)) {
            return false;
        }
        totalRelocations_ += section.relocations.size();
    }
    return true;
}

void MiniLinker::assignVirtualAddresses() {
    uint32_t currentRVA = 0x1000; // Después de headers

    for (auto& section : combinedSections_) {
        // Alinear RVA
        currentRVA = (currentRVA + sectionAlignment_ - 1) & ~(sectionAlignment_ - 1);

        section.virtualAddress = currentRVA;
        currentRVA += section.virtualSize;

        // Actualizar símbolos que apuntan a esta sección
        for (auto& [name, symbol] : globalSymbols_) {
            if (symbol.sectionIndex == (&section - &combinedSections_[0] + 1)) {
                symbol.value += section.virtualAddress;
            }
        }
    }
}

std::vector<uint8_t> MiniLinker::createPEHeader(uint32_t entryPointRVA) {
    std::vector<uint8_t> header;

    // DOS Header
    auto dosStub = PEWriter::createDOSStub();
    header.insert(header.end(), dosStub.begin(), dosStub.end());

    // PE Signature
    const char* peSig = "PE\0\0";
    header.insert(header.end(), peSig, peSig + 4);

    // COFF Header
    COFFHeader coffHeader = {};
    coffHeader.machine = (machineType_ == "X64") ? 0x8664 : 0x014C;
    coffHeader.numberOfSections = static_cast<uint16_t>(combinedSections_.size());
    coffHeader.timeDateStamp = static_cast<uint32_t>(time(nullptr));
    coffHeader.pointerToSymbolTable = 0;
    coffHeader.numberOfSymbols = 0;
    coffHeader.sizeOfOptionalHeader = 240; // PE32+
    coffHeader.characteristics = 0x010F; // Executable, 32-bit, etc.

    const uint8_t* coffBytes = reinterpret_cast<const uint8_t*>(&coffHeader);
    header.insert(header.end(), coffBytes, coffBytes + sizeof(COFFHeader));

    // Optional Header (PE32+)
    uint32_t imageSize = calculateImageSize();
    uint32_t codeSize = 0, dataSize = 0;

    // Calcular tamaños de código y datos
    for (const auto& section : combinedSections_) {
        if (section.characteristics & 0x20) { // CODE
            codeSize += section.virtualSize;
        } else if (section.characteristics & 0x40) { // DATA
            dataSize += section.virtualSize;
        }
    }

    // Optional Header fields
    uint16_t magic = 0x20B; // PE32+
    uint8_t majorLinkerVersion = 14;
    uint8_t minorLinkerVersion = 0;
    uint32_t sizeOfCode = codeSize;
    uint32_t sizeOfInitializedData = dataSize;
    uint32_t sizeOfUninitializedData = 0;
    uint32_t addressOfEntryPoint = entryPointRVA;
    uint32_t baseOfCode = 0x1000;
    uint64_t imageBase = imageBase_;
    uint32_t sectionAlignment = sectionAlignment_;
    uint32_t fileAlignment = fileAlignment_;
    uint16_t majorOSVersion = 6;
    uint16_t minorOSVersion = 0;
    uint16_t majorImageVersion = 0;
    uint16_t minorImageVersion = 0;
    uint16_t majorSubsystemVersion = 6;
    uint16_t minorSubsystemVersion = 0;
    uint32_t win32VersionValue = 0;
    uint32_t sizeOfImage = imageSize;
    uint32_t sizeOfHeaders = 0x400; // Asumiendo tamaño estándar
    uint32_t checkSum = 0; // Se calcula después
    uint16_t subsystem = (subsystem_ == "WINDOWS") ? 2 : 3; // 2=GUI, 3=Console
    uint16_t dllCharacteristics = 0x8160; // NX, ASLR, etc.
    uint64_t sizeOfStackReserve = 0x100000;
    uint64_t sizeOfStackCommit = 0x1000;
    uint64_t sizeOfHeapReserve = 0x100000;
    uint64_t sizeOfHeapCommit = 0x1000;
    uint32_t loaderFlags = 0;
    uint32_t numberOfRvaAndSizes = 16;

    // Serializar optional header
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&magic), reinterpret_cast<uint8_t*>(&magic) + 2);
    header.push_back(majorLinkerVersion);
    header.push_back(minorLinkerVersion);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&sizeOfCode), reinterpret_cast<uint8_t*>(&sizeOfCode) + 4);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&sizeOfInitializedData), reinterpret_cast<uint8_t*>(&sizeOfInitializedData) + 4);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&sizeOfUninitializedData), reinterpret_cast<uint8_t*>(&sizeOfUninitializedData) + 4);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&addressOfEntryPoint), reinterpret_cast<uint8_t*>(&addressOfEntryPoint) + 4);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&baseOfCode), reinterpret_cast<uint8_t*>(&baseOfCode) + 4);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&imageBase), reinterpret_cast<uint8_t*>(&imageBase) + 8);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&sectionAlignment), reinterpret_cast<uint8_t*>(&sectionAlignment) + 4);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&fileAlignment), reinterpret_cast<uint8_t*>(&fileAlignment) + 4);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&majorOSVersion), reinterpret_cast<uint8_t*>(&majorOSVersion) + 2);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&minorOSVersion), reinterpret_cast<uint8_t*>(&minorOSVersion) + 2);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&majorImageVersion), reinterpret_cast<uint8_t*>(&majorImageVersion) + 2);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&minorImageVersion), reinterpret_cast<uint8_t*>(&minorImageVersion) + 2);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&majorSubsystemVersion), reinterpret_cast<uint8_t*>(&majorSubsystemVersion) + 2);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&minorSubsystemVersion), reinterpret_cast<uint8_t*>(&minorSubsystemVersion) + 2);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&win32VersionValue), reinterpret_cast<uint8_t*>(&win32VersionValue) + 4);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&sizeOfImage), reinterpret_cast<uint8_t*>(&sizeOfImage) + 4);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&sizeOfHeaders), reinterpret_cast<uint8_t*>(&sizeOfHeaders) + 4);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&checkSum), reinterpret_cast<uint8_t*>(&checkSum) + 4);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&subsystem), reinterpret_cast<uint8_t*>(&subsystem) + 2);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&dllCharacteristics), reinterpret_cast<uint8_t*>(&dllCharacteristics) + 2);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&sizeOfStackReserve), reinterpret_cast<uint8_t*>(&sizeOfStackReserve) + 8);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&sizeOfStackCommit), reinterpret_cast<uint8_t*>(&sizeOfStackCommit) + 8);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&sizeOfHeapReserve), reinterpret_cast<uint8_t*>(&sizeOfHeapReserve) + 8);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&sizeOfHeapCommit), reinterpret_cast<uint8_t*>(&sizeOfHeapCommit) + 8);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&loaderFlags), reinterpret_cast<uint8_t*>(&loaderFlags) + 4);
    header.insert(header.end(), reinterpret_cast<uint8_t*>(&numberOfRvaAndSizes), reinterpret_cast<uint8_t*>(&numberOfRvaAndSizes) + 4);

    // Data directories (16 entradas, todas cero por simplicidad)
    for (int i = 0; i < 16 * 8; ++i) {
        header.push_back(0);
    }

    return header;
}

std::vector<uint8_t> MiniLinker::createSectionTable() {
    std::vector<uint8_t> sectionTable;

    for (const auto& section : combinedSections_) {
        SectionHeader header = {};

        // Copiar nombre (máximo 8 caracteres)
        std::string name = section.name.substr(0, 8);
        std::memcpy(header.name, name.c_str(), name.size());

        header.virtualSize = section.virtualSize;
        header.virtualAddress = section.virtualAddress;
        header.sizeOfRawData = section.rawSize;
        header.pointerToRawData = 0; // Se calcula después
        header.pointerToRelocations = 0;
        header.pointerToLinenumbers = 0;
        header.numberOfRelocations = 0;
        header.numberOfLinenumbers = 0;
        header.characteristics = section.characteristics;

        const uint8_t* headerBytes = reinterpret_cast<const uint8_t*>(&header);
        sectionTable.insert(sectionTable.end(), headerBytes, headerBytes + sizeof(SectionHeader));
    }

    return sectionTable;
}

std::vector<uint8_t> MiniLinker::createImportDirectory() {
    // Simplificado: no implementamos import directory completo
    // En un linker real, esto sería mucho más complejo
    return std::vector<uint8_t>(20, 0); // Estructura mínima
}

std::vector<uint8_t> MiniLinker::createExportDirectory() {
    // Simplificado: no exportamos nada
    return std::vector<uint8_t>(40, 0); // Estructura mínima
}

std::vector<uint8_t> MiniLinker::createBaseRelocations() {
    // Simplificado: sin relocations base
    return std::vector<uint8_t>();
}

bool MiniLinker::writePEFile(const std::filesystem::path& outputFile,
                            const std::vector<uint8_t>& peHeader,
                            const std::vector<uint8_t>& sectionTable,
                            const std::vector<uint8_t>& importDirectory,
                            const std::vector<uint8_t>& exportDirectory,
                            const std::vector<uint8_t>& baseRelocations) {

    std::vector<std::vector<uint8_t>> sectionData;
    for (const auto& section : combinedSections_) {
        sectionData.push_back(section.data);
    }

    return PEWriter::writePEFile(outputFile,
                                PEWriter::createDOSStub(),
                                peHeader,
                                sectionTable,
                                sectionData,
                                importDirectory,
                                exportDirectory,
                                baseRelocations);
}

bool MiniLinker::checkObjectCompatibility(const ObjectFileInfo& obj) const {
    // Verificar arquitectura
    if (obj.machineType != machineType_) {
        return false;
    }

    // Verificar que el archivo sea válido
    return obj.isValid;
}

bool MiniLinker::resolveSymbolConflicts() {
    auto conflicts = SymbolResolver::findSymbolConflicts(globalSymbols_);

    if (!conflicts.empty()) {
        // En un linker real, esto sería un error
        // Por simplicidad, reportamos pero continuamos
        for (const auto& conflict : conflicts) {
            std::cerr << "Warning: Symbol conflict for '" << conflict << "'" << std::endl;
        }
    }

    return true;
}

void MiniLinker::optimizeSectionLayout() {
    // Ordenar secciones por características para mejor locality
    std::sort(combinedSections_.begin(), combinedSections_.end(),
              [](const SectionInfo& a, const SectionInfo& b) {
                  // Código primero, luego datos inicializados, luego datos no inicializados
                  auto getPriority = [](const SectionInfo& s) -> int {
                      if (s.characteristics & 0x20) return 0; // CODE
                      if (s.characteristics & 0x40) return 1; // INITIALIZED_DATA
                      if (s.characteristics & 0x80) return 2; // UNINITIALIZED_DATA
                      return 3; // Otros
                  };

                  return getPriority(a) < getPriority(b);
              });
}

uint32_t MiniLinker::calculateImageSize() const {
    if (combinedSections_.empty()) {
        return sectionAlignment_;
    }

    const auto& lastSection = combinedSections_.back();
    uint32_t lastEnd = lastSection.virtualAddress + lastSection.virtualSize;
    return (lastEnd + sectionAlignment_ - 1) & ~(sectionAlignment_ - 1);
}

uint32_t MiniLinker::getSymbolRVA(const std::string& symbolName) const {
    auto it = globalSymbols_.find(symbolName);
    if (it != globalSymbols_.end() && it->second.isDefined) {
        return it->second.value;
    }
    return 0;
}

bool MiniLinker::isExternallyDefined(const std::string& symbolName) const {
    // Verificar si el símbolo está definido en una biblioteca externa
    for (const auto& import : imports_) {
        for (const auto& func : import.functionNames) {
            if (func == symbolName) {
                return true;
            }
        }
    }
    return false;
}

void MiniLinker::addRuntimeSymbols() {
    // Añadir símbolos de runtime estándar
    const std::vector<std::string> runtimeSymbols = {
        "_mainCRTStartup", "main", "_start", "__libc_start_main",
        "printf", "puts", "malloc", "free", "memcpy", "memset"
    };

    for (const auto& symbol : runtimeSymbols) {
        SymbolInfo info(symbol, 0, 0);
        info.isDefined = false; // Se resolverán externamente
        info.isExternal = true;
        globalSymbols_[symbol] = info;
    }
}

void MiniLinker::handleWeakSymbols() {
    SymbolResolver::resolveWeakSymbols(globalSymbols_);
}

void MiniLinker::updateStatistics() {
    resolvedSymbols_ = 0;
    for (const auto& [name, symbol] : globalSymbols_) {
        if (symbol.isDefined) {
            resolvedSymbols_++;
        }
    }
}

// ============================================================================
// COFFReader - Implementación
// ============================================================================

bool COFFReader::readObjectFile(const std::filesystem::path& filePath,
                               ObjectFileInfo& objInfo) {

    if (!validateCOFFFormat(filePath)) {
        return false;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    try {
        COFFHeader coffHeader;
        if (!readCOFFHeader(file, coffHeader)) {
            return false;
        }

        // Leer section headers
        std::vector<SectionHeader> sectionHeaders;
        if (!readSectionHeaders(file, coffHeader, sectionHeaders)) {
            return false;
        }

        // Leer symbol table
        std::vector<COFFSymbol> symbols;
        std::vector<std::string> stringTable;
        if (!readSymbolTable(file, coffHeader, symbols, stringTable)) {
            return false;
        }

        // Extraer secciones
        objInfo.sections = extractSections(std::vector<uint8_t>(), coffHeader);

        // Extraer símbolos
        objInfo.symbols = extractSymbols(std::vector<uint8_t>(), coffHeader);

        objInfo.machineType = (coffHeader.machine == 0x8664) ? "X64" : "X86";
        objInfo.isValid = true;

    } catch (const std::exception&) {
        return false;
    }

    return true;
}

bool COFFReader::validateCOFFFormat(const std::filesystem::path& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Leer signature (4 bytes)
    char signature[4];
    file.read(signature, 4);

    // Verificar signature COFF
    return std::memcmp(signature, "\x00\x00\xff\xff", 4) == 0 ||
           std::memcmp(signature, "PE\x00\x00", 4) == 0;
}

std::vector<SymbolInfo> COFFReader::extractSymbols(const std::vector<uint8_t>& data,
                                                  const COFFHeader& header) {
    std::vector<SymbolInfo> symbols;
    // Implementación simplificada - en un linker real esto sería más complejo
    return symbols;
}

std::vector<SectionInfo> COFFReader::extractSections(const std::vector<uint8_t>& data,
                                                    const COFFHeader& header) {
    std::vector<SectionInfo> sections;
    // Implementación simplificada
    return sections;
}

std::vector<RelocationInfo> COFFReader::extractRelocations(const std::vector<uint8_t>& data,
                                                          const SectionHeader& section,
                                                          uint32_t symbolCount) {
    std::vector<RelocationInfo> relocations;
    // Implementación simplificada
    return relocations;
}

bool COFFReader::readCOFFHeader(std::ifstream& file, COFFHeader& header) {
    file.read(reinterpret_cast<char*>(&header), sizeof(COFFHeader));
    return file.good();
}

bool COFFReader::readSectionHeaders(std::ifstream& file, const COFFHeader& coffHeader,
                                   std::vector<SectionHeader>& sectionHeaders) {
    sectionHeaders.resize(coffHeader.numberOfSections);
    for (auto& section : sectionHeaders) {
        file.read(reinterpret_cast<char*>(&section), sizeof(SectionHeader));
        if (!file.good()) return false;
    }
    return true;
}

bool COFFReader::readSymbolTable(std::ifstream& file, const COFFHeader& coffHeader,
                                std::vector<COFFSymbol>& symbols,
                                std::vector<std::string>& stringTable) {
    if (coffHeader.pointerToSymbolTable == 0) return true;

    // Implementación simplificada
    return true;
}

bool COFFReader::readSectionData(std::ifstream& file, const SectionHeader& section,
                                std::vector<uint8_t>& data) {
    if (section.sizeOfRawData == 0) return true;

    file.seekg(section.pointerToRawData);
    data.resize(section.sizeOfRawData);
    file.read(reinterpret_cast<char*>(data.data()), section.sizeOfRawData);

    return file.good();
}

bool COFFReader::readSectionRelocations(std::ifstream& file, const SectionHeader& section,
                                       std::vector<RelocationInfo>& relocations) {
    if (section.numberOfRelocations == 0) return true;

    file.seekg(section.pointerToRelocations);
    relocations.resize(section.numberOfRelocations);

    for (auto& reloc : relocations) {
        file.read(reinterpret_cast<char*>(&reloc), sizeof(RelocationInfo));
        if (!file.good()) return false;
    }

    return true;
}

// ============================================================================
// PEWriter - Implementación
// ============================================================================

bool PEWriter::writePEFile(const std::filesystem::path& outputFile,
                          const std::vector<uint8_t>& dosHeader,
                          const std::vector<uint8_t>& peHeader,
                          const std::vector<uint8_t>& sectionTable,
                          const std::vector<std::vector<uint8_t>>& sectionData,
                          const std::vector<uint8_t>& importDirectory,
                          const std::vector<uint8_t>& exportDirectory,
                          const std::vector<uint8_t>& baseRelocations) {

    std::ofstream file(outputFile, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    try {
        // Escribir DOS header
        file.write(reinterpret_cast<const char*>(dosHeader.data()), dosHeader.size());

        // Escribir PE header
        file.write(reinterpret_cast<const char*>(peHeader.data()), peHeader.size());

        // Escribir section table
        file.write(reinterpret_cast<const char*>(sectionTable.data()), sectionTable.size());

        // Escribir datos de secciones
        for (const auto& section : sectionData) {
            file.write(reinterpret_cast<const char*>(section.data()), section.size());
        }

        // Escribir directorios opcionales
        if (!importDirectory.empty()) {
            file.write(reinterpret_cast<const char*>(importDirectory.data()), importDirectory.size());
        }

        if (!exportDirectory.empty()) {
            file.write(reinterpret_cast<const char*>(exportDirectory.data()), exportDirectory.size());
        }

        if (!baseRelocations.empty()) {
            file.write(reinterpret_cast<const char*>(baseRelocations.data()), baseRelocations.size());
        }

    } catch (const std::exception&) {
        return false;
    }

    return true;
}

std::vector<uint8_t> PEWriter::createDOSStub() {
    // DOS stub mínimo
    const char* dosStub = "\x4D\x5A\x50\x00\x02\x00\x00\x00\x04\x00\x0F\x00\xFF\xFF\x00\x00"
                          "\xB8\x00\x00\x00\x00\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x00"
                          "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                          "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00\x00";

    return std::vector<uint8_t>(dosStub, dosStub + 64);
}

void PEWriter::updatePEChecksum(std::vector<uint8_t>& peHeader, uint32_t checksum) {
    if (peHeader.size() >= 0x58 + 4) {
        *reinterpret_cast<uint32_t*>(&peHeader[0x58]) = checksum;
    }
}

std::vector<uint8_t> PEWriter::alignData(const std::vector<uint8_t>& data,
                                       size_t alignment,
                                       uint8_t fillByte) {
    size_t alignedSize = (data.size() + alignment - 1) & ~(alignment - 1);
    std::vector<uint8_t> aligned = data;
    aligned.resize(alignedSize, fillByte);
    return aligned;
}

uint32_t PEWriter::calculateFileSize(const std::vector<uint8_t>& dosHeader,
                                   const std::vector<uint8_t>& peHeader,
                                   const std::vector<uint8_t>& sectionTable,
                                   const std::vector<std::vector<uint8_t>>& sectionData,
                                   size_t optionalDataSize) {

    uint32_t size = dosHeader.size() + peHeader.size() + sectionTable.size();

    for (const auto& section : sectionData) {
        size += section.size();
    }

    size += optionalDataSize;

    return size;
}

// ============================================================================
// SymbolResolver - Implementación
// ============================================================================

bool SymbolResolver::resolveSymbol(const std::string& symbolName,
                                  const std::unordered_map<std::string, SymbolInfo>& globalSymbols,
                                  uint32_t& resolvedAddress) {

    auto it = globalSymbols.find(symbolName);
    if (it != globalSymbols.end() && it->second.isDefined) {
        resolvedAddress = it->second.value;
        return true;
    }

    return false;
}

std::vector<std::string> SymbolResolver::findUndefinedSymbols(
    const std::unordered_map<std::string, SymbolInfo>& globalSymbols) {

    std::vector<std::string> undefined;

    for (const auto& [name, symbol] : globalSymbols) {
        if (!symbol.isDefined && !symbol.isExternal) {
            undefined.push_back(name);
        }
    }

    return undefined;
}

std::vector<std::string> SymbolResolver::findSymbolConflicts(
    const std::unordered_map<std::string, SymbolInfo>& globalSymbols) {

    std::vector<std::string> conflicts;
    std::unordered_map<std::string, int> definitionCount;

    for (const auto& [name, symbol] : globalSymbols) {
        if (symbol.isDefined) {
            definitionCount[name]++;
        }
    }

    for (const auto& [name, count] : definitionCount) {
        if (count > 1) {
            conflicts.push_back(name);
        }
    }

    return conflicts;
}

void SymbolResolver::resolveWeakSymbols(
    std::unordered_map<std::string, SymbolInfo>& globalSymbols) {

    for (auto& [name, symbol] : globalSymbols) {
        if (symbol.isWeak && !symbol.isDefined) {
            // Resolver símbolo débil con valor por defecto
            symbol.value = 0;
            symbol.isDefined = true;
        }
    }
}

std::vector<std::string> SymbolResolver::getExportedSymbols(
    const std::unordered_map<std::string, SymbolInfo>& globalSymbols) {

    std::vector<std::string> exported;

    for (const auto& [name, symbol] : globalSymbols) {
        if (symbol.isDefined && symbol.storageClass == 2) { // IMAGE_SYM_CLASS_EXTERNAL
            exported.push_back(name);
        }
    }

    return exported;
}

bool SymbolResolver::isSymbolValidForLinking(const SymbolInfo& symbol) {
    // Verificar que el símbolo tenga un nombre válido
    if (symbol.name.empty()) {
        return false;
    }

    // Verificar que no sea un símbolo especial
    if (symbol.name[0] == '.' || symbol.name[0] == '$') {
        return false;
    }

    return true;
}

const std::unordered_set<std::string>& SymbolResolver::getRuntimeSymbols() {
    static const std::unordered_set<std::string> runtimeSymbols = {
        "_mainCRTStartup", "main", "_start", "__libc_start_main",
        "printf", "puts", "malloc", "free", "memcpy", "memset",
        "strlen", "strcmp", "strcpy", "exit", "_exit"
    };

    return runtimeSymbols;
}

// ============================================================================
// RelocationApplier - Implementación
// ============================================================================

bool RelocationApplier::applyRelocation(std::vector<uint8_t>& sectionData,
                                       const RelocationInfo& relocation,
                                       uint32_t symbolAddress,
                                       uint32_t sectionRVA) {

    uint32_t value = calculateRelocationValue(relocation.type,
                                             symbolAddress,
                                             sectionRVA,
                                             relocation.virtualAddress);

    switch (relocation.type) {
        case 0x0001: // IMAGE_REL_AMD64_ADDR64
            return applyAddr64Relocation(sectionData, relocation.virtualAddress, value);

        case 0x0002: // IMAGE_REL_AMD64_ADDR32
            return applyAddr32Relocation(sectionData, relocation.virtualAddress, value);

        case 0x0004: // IMAGE_REL_AMD64_REL32
            return applyRel32Relocation(sectionData, relocation.virtualAddress, value, sectionRVA);

        default:
            return false; // Tipo de relocation no soportado
    }
}

bool RelocationApplier::applySectionRelocations(std::vector<uint8_t>& sectionData,
                                               const std::vector<RelocationInfo>& relocations,
                                               const std::unordered_map<std::string, SymbolInfo>& globalSymbols,
                                               const std::vector<SectionInfo>& sections) {

    for (const auto& reloc : relocations) {
        // Encontrar el símbolo referenciado
        auto symbolIt = globalSymbols.find(std::to_string(reloc.symbolTableIndex));
        if (symbolIt == globalSymbols.end()) {
            return false;
        }

        const auto& symbol = symbolIt->second;
        if (!symbol.isDefined) {
            continue; // Símbolo no definido, se resolverá después
        }

        uint32_t symbolAddress = symbol.value;
        uint32_t sectionRVA = sections[symbol.sectionIndex - 1].virtualAddress;

        if (!applyRelocation(sectionData, reloc, symbolAddress, sectionRVA)) {
            return false;
        }
    }

    return true;
}

bool RelocationApplier::validateRelocation(const RelocationInfo& relocation,
                                          const SymbolInfo& symbol) {
    // Verificar que el símbolo esté definido
    if (!symbol.isDefined) {
        return false;
    }

    // Verificar que el offset esté dentro de la sección
    if (relocation.virtualAddress >= 0x10000000) { // Límite arbitrario
        return false;
    }

    return true;
}

uint32_t RelocationApplier::convertRelocationToRVA(uint16_t relocationType,
                                                  uint32_t symbolAddress,
                                                  uint32_t sectionRVA,
                                                  uint32_t offset) {
    switch (relocationType) {
        case 0x0001: // IMAGE_REL_AMD64_ADDR64
            return symbolAddress;

        case 0x0002: // IMAGE_REL_AMD64_ADDR32
            return symbolAddress;

        case 0x0004: // IMAGE_REL_AMD64_REL32
            return symbolAddress - (sectionRVA + offset + 4);

        default:
            return symbolAddress;
    }
}

uint32_t RelocationApplier::calculateRelocationValue(uint16_t type,
                                                    uint32_t symbolValue,
                                                    uint32_t sectionRVA,
                                                    uint32_t offset) {
    return convertRelocationToRVA(type, symbolValue, sectionRVA, offset);
}

bool RelocationApplier::applyAddr32Relocation(std::vector<uint8_t>& data,
                                             size_t offset,
                                             uint32_t value) {
    if (offset + 4 > data.size()) {
        return false;
    }

    *reinterpret_cast<uint32_t*>(&data[offset]) = value;
    return true;
}

bool RelocationApplier::applyAddr64Relocation(std::vector<uint8_t>& data,
                                             size_t offset,
                                             uint64_t value) {
    if (offset + 8 > data.size()) {
        return false;
    }

    *reinterpret_cast<uint64_t*>(&data[offset]) = value;
    return true;
}

bool RelocationApplier::applyRel32Relocation(std::vector<uint8_t>& data,
                                            size_t offset,
                                            uint32_t value,
                                            uint32_t currentRVA) {
    if (offset + 4 > data.size()) {
        return false;
    }

    *reinterpret_cast<uint32_t*>(&data[offset]) = value;
    return true;
}

} // namespace cpp20::compiler::backend::link
