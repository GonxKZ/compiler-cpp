/**
 * @file CodeViewEmitter.cpp
 * @brief Implementación del emisor de información de debug CodeView
 */

#include <compiler/debug/CodeViewEmitter.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>

namespace cpp20::compiler::debug {

// ============================================================================
// SourceLineInfo - Implementación
// ============================================================================

// Ya definido en header

// ============================================================================
// DebugSymbol - Implementación
// ============================================================================

// Ya definido en header

// ============================================================================
// CodeViewEmitter - Implementación
// ============================================================================

CodeViewEmitter::CodeViewEmitter()
    : targetArch_("x64"), nextTypeIndex_(0x1000), nextFileIndex_(1) {
}

CodeViewEmitter::~CodeViewEmitter() = default;

void CodeViewEmitter::addSourceFile(const std::filesystem::path& sourceFile,
                                  const std::vector<SourceLineInfo>& lineInfo) {
    // Añadir índice de archivo
    if (fileNameMap_.find(sourceFile.string()) == fileNameMap_.end()) {
        fileNameMap_[sourceFile.string()] = nextFileIndex_++;
    }

    // Añadir información de línea
    sourceLines_.insert(sourceLines_.end(), lineInfo.begin(), lineInfo.end());
}

void CodeViewEmitter::addDebugSymbol(const DebugSymbol& symbol) {
    debugSymbols_.push_back(symbol);
}

void CodeViewEmitter::addDebugType(const DebugType& type) {
    debugTypes_.push_back(type);
}

std::vector<uint8_t> CodeViewEmitter::generateDebugSSymbols() {
    std::vector<uint8_t> debugS;

    // Header CodeView
    const char* cvHeader = "Microsoft C/C++ MSF 7.00\r\n\x1A\x44\x53";
    debugS.insert(debugS.end(), cvHeader, cvHeader + 31);

    uint32_t version = 4; // CV 7.00
    const uint8_t* verBytes = reinterpret_cast<const uint8_t*>(&version);
    debugS.insert(debugS.end(), verBytes, verBytes + 4);

    // Generar símbolos
    for (const auto& symbol : debugSymbols_) {
        auto serialized = serializeSymbol(symbol);
        debugS.insert(debugS.end(), serialized.begin(), serialized.end());
    }

    return debugS;
}

std::vector<uint8_t> CodeViewEmitter::generateDebugTTypes() {
    std::vector<uint8_t> debugT;

    // Header de tipos CodeView
    uint32_t signature = 0x00000004; // CV 7.00
    const uint8_t* sigBytes = reinterpret_cast<const uint8_t*>(&signature);
    debugT.insert(debugT.end(), sigBytes, sigBytes + 4);

    // Generar tipos
    for (const auto& type : debugTypes_) {
        auto serialized = serializeType(type);
        debugT.insert(debugT.end(), serialized.begin(), serialized.end());
    }

    return debugT;
}

std::vector<uint8_t> CodeViewEmitter::generateDebugFFiles() {
    std::vector<uint8_t> debugF;

    // Header de archivos
    uint32_t signature = 0x00000001;
    const uint8_t* sigBytes = reinterpret_cast<const uint8_t*>(&signature);
    debugF.insert(debugF.end(), sigBytes, sigBytes + 4);

    // Lista de archivos
    for (const auto& [fileName, index] : fileNameMap_) {
        // Longitud del nombre
        uint32_t nameLen = static_cast<uint32_t>(fileName.size());
        const uint8_t* lenBytes = reinterpret_cast<const uint8_t*>(&nameLen);
        debugF.insert(debugF.end(), lenBytes, lenBytes + 4);

        // Nombre del archivo
        debugF.insert(debugF.end(), fileName.begin(), fileName.end());
        debugF.push_back(0); // Null terminator
    }

    return debugF;
}

std::vector<uint8_t> CodeViewEmitter::generateLineNumbers() {
    std::vector<uint8_t> lineNumbers;

    // Header de números de línea
    uint32_t signature = 0x00000002;
    const uint8_t* sigBytes = reinterpret_cast<const uint8_t*>(&signature);
    lineNumbers.insert(lineNumbers.end(), sigBytes, sigBytes + 4);

    // Generar información de línea comprimida
    auto compressed = compressLineInfo(sourceLines_);
    lineNumbers.insert(lineNumbers.end(), compressed.begin(), compressed.end());

    return lineNumbers;
}

void CodeViewEmitter::setTargetArchitecture(const std::string& arch) {
    targetArch_ = arch;
}

void CodeViewEmitter::clear() {
    debugSymbols_.clear();
    debugTypes_.clear();
    sourceLines_.clear();
    fileNameMap_.clear();
    nextTypeIndex_ = 0x1000;
    nextFileIndex_ = 1;
}

std::unordered_map<std::string, size_t> CodeViewEmitter::getDebugStatistics() const {
    return {
        {"debug_symbols", debugSymbols_.size()},
        {"debug_types", debugTypes_.size()},
        {"source_lines", sourceLines_.size()},
        {"source_files", fileNameMap_.size()}
    };
}

std::vector<uint8_t> CodeViewEmitter::createSubsectionHeader(uint32_t type, uint32_t size) {
    std::vector<uint8_t> header;

    // Tipo de subsección
    const uint8_t* typeBytes = reinterpret_cast<const uint8_t*>(&type);
    header.insert(header.end(), typeBytes, typeBytes + 4);

    // Tamaño
    const uint8_t* sizeBytes = reinterpret_cast<const uint8_t*>(&size);
    header.insert(header.end(), sizeBytes, sizeBytes + 4);

    return header;
}

std::vector<uint8_t> CodeViewEmitter::serializeSymbol(const DebugSymbol& symbol) {
    std::vector<uint8_t> data;

    // Tipo de registro
    uint16_t recordType = static_cast<uint16_t>(symbol.type);
    const uint8_t* typeBytes = reinterpret_cast<const uint8_t*>(&recordType);
    data.insert(data.end(), typeBytes, typeBytes + 2);

    // Longitud (se calcula después)
    uint16_t length = 0;
    const uint8_t* lenBytes = reinterpret_cast<const uint8_t*>(&length);
    data.insert(data.end(), lenBytes, lenBytes + 2);

    // Datos específicos del símbolo
    switch (symbol.type) {
        case CodeViewRecordType::S_GPROC32: {
            // Global procedure, 32-bit
            uint32_t pParent = 0;
            uint32_t pEnd = 0;
            uint32_t pNext = 0;
            uint32_t procLen = symbol.size;
            uint32_t debugStart = symbol.address;
            uint32_t debugEnd = symbol.address + symbol.size;
            uint32_t typeIndex = 0; // TODO: calcular índice de tipo
            uint8_t flags = 0;

            const uint8_t* parentBytes = reinterpret_cast<const uint8_t*>(&pParent);
            data.insert(data.end(), parentBytes, parentBytes + 4);

            const uint8_t* endBytes = reinterpret_cast<const uint8_t*>(&pEnd);
            data.insert(data.end(), endBytes, endBytes + 4);

            const uint8_t* nextBytes = reinterpret_cast<const uint8_t*>(&pNext);
            data.insert(data.end(), nextBytes, nextBytes + 4);

            const uint8_t* lenBytes = reinterpret_cast<const uint8_t*>(&procLen);
            data.insert(data.end(), lenBytes, lenBytes + 4);

            const uint8_t* startBytes = reinterpret_cast<const uint8_t*>(&debugStart);
            data.insert(data.end(), startBytes, startBytes + 4);

            const uint8_t* endAddrBytes = reinterpret_cast<const uint8_t*>(&debugEnd);
            data.insert(data.end(), endAddrBytes, endAddrBytes + 4);

            const uint8_t* typeBytes = reinterpret_cast<const uint8_t*>(&typeIndex);
            data.insert(data.end(), typeBytes, typeBytes + 4);

            data.push_back(flags);

            // Nombre de la función
            data.insert(data.end(), symbol.name.begin(), symbol.name.end());
            data.push_back(0); // Null terminator

            break;
        }

        case CodeViewRecordType::S_GDATA32: {
            // Global data, 32-bit
            uint32_t typeIndex = 0; // TODO: calcular índice de tipo

            const uint8_t* typeBytes = reinterpret_cast<const uint8_t*>(&typeIndex);
            data.insert(data.end(), typeBytes, typeBytes + 4);

            const uint8_t* addrBytes = reinterpret_cast<const uint8_t*>(&symbol.address);
            data.insert(data.end(), addrBytes, addrBytes + 4);

            // Nombre de la variable
            data.insert(data.end(), symbol.name.begin(), symbol.name.end());
            data.push_back(0); // Null terminator

            break;
        }

        default:
            // Símbolo genérico
            data.insert(data.end(), symbol.data.begin(), symbol.data.end());
            break;
    }

    // Actualizar longitud
    length = static_cast<uint16_t>(data.size() - 4); // -4 para type y length
    std::memcpy(&data[2], &length, 2);

    return data;
}

std::vector<uint8_t> CodeViewEmitter::serializeType(const DebugType& type) {
    std::vector<uint8_t> data;

    // Tipo de registro
    uint16_t recordType = static_cast<uint16_t>(type.type);
    const uint8_t* typeBytes = reinterpret_cast<const uint8_t*>(&recordType);
    data.insert(data.end(), typeBytes, typeBytes + 2);

    // Longitud
    uint16_t length = static_cast<uint16_t>(type.data.size());
    const uint8_t* lenBytes = reinterpret_cast<const uint8_t*>(&length);
    data.insert(data.end(), lenBytes, lenBytes + 2);

    // Datos del tipo
    data.insert(data.end(), type.data.begin(), type.data.end());

    return data;
}

std::vector<uint8_t> CodeViewEmitter::serializeLineInfo(const SourceLineInfo& lineInfo) {
    std::vector<uint8_t> data;

    // Índice de archivo
    uint32_t fileIndex = getFileIndex(lineInfo.fileName);
    const uint8_t* fileBytes = reinterpret_cast<const uint8_t*>(&fileIndex);
    data.insert(data.end(), fileBytes, fileBytes + 4);

    // Número de línea
    const uint8_t* lineBytes = reinterpret_cast<const uint8_t*>(&lineInfo.lineNumber);
    data.insert(data.end(), lineBytes, lineBytes + 4);

    // Dirección
    const uint8_t* addrBytes = reinterpret_cast<const uint8_t*>(&lineInfo.address);
    data.insert(data.end(), addrBytes, addrBytes + 4);

    return data;
}

uint32_t CodeViewEmitter::getFileIndex(const std::string& fileName) {
    auto it = fileNameMap_.find(fileName);
    return (it != fileNameMap_.end()) ? it->second : 0;
}

uint32_t CodeViewEmitter::calculateChecksum(const std::vector<uint8_t>& data) {
    uint32_t checksum = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        checksum = (checksum << 1) | (checksum >> 31);
        checksum ^= data[i];
    }
    return checksum;
}

std::vector<uint8_t> CodeViewEmitter::compressLineInfo(const std::vector<SourceLineInfo>& lines) {
    std::vector<uint8_t> compressed;

    if (lines.empty()) return compressed;

    // Implementación simplificada de compresión
    // En un compilador real, se usaría un algoritmo más sofisticado

    for (const auto& line : lines) {
        auto serialized = serializeLineInfo(line);
        compressed.insert(compressed.end(), serialized.begin(), serialized.end());
    }

    return compressed;
}

// ============================================================================
// PDBGenerator - Implementación
// ============================================================================

PDBGenerator::PDBGenerator() : timestamp_(0) {
}

PDBGenerator::~PDBGenerator() = default;

void PDBGenerator::addDebugInfo(const DebugObjectFile& debugInfo) {
    debugObjects_.push_back(debugInfo);
}

bool PDBGenerator::generatePDB(const std::filesystem::path& pdbPath) {
    // En una implementación real, aquí se crearía un archivo PDB completo
    // con múltiples streams según el formato MSF (Multi-Stream File)

    // Por simplicidad, creamos un archivo básico
    std::ofstream file(pdbPath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    try {
        // Header PDB básico
        const char* pdbHeader = "Microsoft C/C++ PDB 7.00\r\n\x1A\x4A\x53";
        file.write(pdbHeader, 28);

        // Timestamp
        file.write(reinterpret_cast<const char*>(&timestamp_), sizeof(timestamp_));

        // Tamaño de página
        uint32_t pageSize = 0x1000;
        file.write(reinterpret_cast<const char*>(&pageSize), sizeof(pageSize));

        // Número de páginas de directorio
        uint32_t numDirPages = 1;
        file.write(reinterpret_cast<const char*>(&numDirPages), sizeof(numDirPages));

        // Placeholder para datos adicionales
        uint32_t zero = 0;
        for (int i = 0; i < 10; ++i) {
            file.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
        }

        file.close();
        return true;

    } catch (const std::exception&) {
        return false;
    }
}

void PDBGenerator::setModuleName(const std::string& name) {
    moduleName_ = name;
}

void PDBGenerator::setTimestamp(uint32_t timestamp) {
    timestamp_ = timestamp;
}

std::vector<uint8_t> PDBGenerator::createModuleInfoStream() {
    std::vector<uint8_t> stream;

    // En una implementación real, aquí se crearían los streams del PDB:
    // - Stream 0: PDB info
    // - Stream 1: TPI (Type Program Interface)
    // - Stream 2: DBI (Debug Info)
    // - Stream 3: IPI (Item Program Interface)
    // - Etc.

    return stream;
}

std::vector<uint8_t> PDBGenerator::createGlobalSymbolStream() {
    std::vector<uint8_t> stream;

    // Stream de símbolos globales
    for (const auto& obj : debugObjects_) {
        for (const auto& symbol : obj.symbols) {
            // Serializar símbolo para PDB
            // Implementación simplificada
        }
    }

    return stream;
}

std::vector<uint8_t> PDBGenerator::createTypeStream() {
    std::vector<uint8_t> stream;

    // Stream de tipos
    for (const auto& obj : debugObjects_) {
        for (const auto& type : obj.types) {
            // Serializar tipo para PDB
            // Implementación simplificada
        }
    }

    return stream;
}

std::vector<uint8_t> PDBGenerator::createLineInfoStream() {
    std::vector<uint8_t> stream;

    // Stream de información de línea
    for (const auto& obj : debugObjects_) {
        for (const auto& line : obj.lineInfo) {
            // Serializar información de línea
            // Implementación simplificada
        }
    }

    return stream;
}

std::string PDBGenerator::calculateContentHash(const std::vector<uint8_t>& data) {
    // Implementación simplificada de hash
    std::hash<std::string> hasher;
    std::string dataStr(data.begin(), data.end());
    return std::to_string(hasher(dataStr));
}

// ============================================================================
// CodeViewUtils - Implementación
// ============================================================================

CodeViewRecordType CodeViewUtils::convertToCodeViewType(const Type& type) {
    // Conversión simplificada de tipos C++ a tipos CodeView
    switch (type.getKind()) {
        case TypeKind::Void:
            return CodeViewRecordType::LF_NULL;
        case TypeKind::Bool:
        case TypeKind::Char:
        case TypeKind::Int:
        case TypeKind::Float:
            return CodeViewRecordType::LF_MODIFIER;
        case TypeKind::Pointer:
            return CodeViewRecordType::LF_POINTER;
        case TypeKind::Array:
            return CodeViewRecordType::LF_ARRAY;
        case TypeKind::Class:
            return CodeViewRecordType::LF_CLASS;
        case TypeKind::Struct:
            return CodeViewRecordType::LF_STRUCTURE;
        case TypeKind::Enum:
            return CodeViewRecordType::LF_ENUM;
        case TypeKind::Function:
            return CodeViewRecordType::LF_PROCEDURE;
        default:
            return CodeViewRecordType::LF_NULL;
    }
}

CodeViewRecordType CodeViewUtils::convertToCodeViewSymbol(const Symbol& symbol) {
    // Conversión simplificada de símbolos
    if (symbol.isFunction()) {
        return CodeViewRecordType::S_GPROC32;
    } else if (symbol.isVariable()) {
        return CodeViewRecordType::S_GDATA32;
    } else {
        return CodeViewRecordType::S_CONSTANT;
    }
}

std::vector<SourceLineInfo> CodeViewUtils::generateLineInfoForFunction(
    const std::string& functionName,
    const std::vector<uint32_t>& addresses,
    const std::vector<uint32_t>& lineNumbers) {

    std::vector<SourceLineInfo> lineInfo;

    size_t count = std::min(addresses.size(), lineNumbers.size());
    for (size_t i = 0; i < count; ++i) {
        lineInfo.emplace_back("", lineNumbers[i], addresses[i]);
    }

    return lineInfo;
}

bool CodeViewUtils::isCompatibleVersion(uint16_t version) {
    // Versiones CodeView compatibles
    return version >= 4 && version <= 8; // CV 7.00 a 8.00
}

size_t CodeViewUtils::getRecordSize(CodeViewRecordType type) {
    // Tamaños aproximados de registros CodeView
    switch (type) {
        case CodeViewRecordType::S_GPROC32:
            return 44; // Tamaño típico
        case CodeViewRecordType::S_GDATA32:
            return 20;
        case CodeViewRecordType::S_CONSTANT:
            return 16;
        default:
            return 8; // Tamaño mínimo
    }
}

bool CodeViewUtils::validateRecord(const std::vector<uint8_t>& record) {
    if (record.size() < 4) return false;

    // Verificar que la longitud sea consistente
    uint16_t length = *reinterpret_cast<const uint16_t*>(&record[2]);
    return record.size() >= static_cast<size_t>(length) + 4;
}

std::string CodeViewUtils::extractRecordName(const std::vector<uint8_t>& record) {
    if (record.size() < 4) return "";

    uint16_t length = *reinterpret_cast<const uint16_t*>(&record[2]);
    if (record.size() < static_cast<size_t>(length) + 4) return "";

    // Buscar null terminator
    for (size_t i = 4; i < record.size(); ++i) {
        if (record[i] == 0) {
            return std::string(record.begin() + 4, record.begin() + i);
        }
    }

    return "";
}

std::string CodeViewUtils::formatRecord(const std::vector<uint8_t>& record) {
    if (record.size() < 4) return "Invalid record";

    uint16_t type = *reinterpret_cast<const uint16_t*>(&record[0]);
    uint16_t length = *reinterpret_cast<const uint16_t*>(&record[2]);

    std::stringstream ss;
    ss << "Type: 0x" << std::hex << type
       << ", Length: " << std::dec << length
       << ", Name: " << extractRecordName(record);

    return ss.str();
}

// ============================================================================
// DebugIntegration - Implementación
// ============================================================================

DebugIntegration::DebugIntegration()
    : debugEnabled_(false), debugLevel_(0) {
}

DebugIntegration::~DebugIntegration() = default;

void DebugIntegration::enableDebugGeneration(bool enable) {
    debugEnabled_ = enable;
}

void DebugIntegration::setDebugLevel(int level) {
    debugLevel_ = level;
}

void DebugIntegration::addDebugInfoFromAST(const ast::ASTNode* node,
                                         const std::filesystem::path& sourceFile) {
    if (!debugEnabled_) return;

    // Recorrer el AST y extraer información de debug
    // Implementación simplificada
    processDeclarationForDebug(node, sourceFile);
}

void DebugIntegration::addDebugInfoFromIR(const ir::IRFunction* function) {
    if (!debugEnabled_ || !function) return;

    // Extraer información de debug de la IR
    // Implementación simplificada

    DebugSymbol funcSymbol(CodeViewRecordType::S_GPROC32, function->name);
    funcSymbol.address = 0; // TODO: calcular dirección real
    funcSymbol.size = 0; // TODO: calcular tamaño real

    codeViewEmitter_.addDebugSymbol(funcSymbol);
}

std::vector<uint8_t> DebugIntegration::generateCompleteDebugInfo() {
    if (!debugEnabled_) return {};

    // Generar información de debug completa
    auto debugS = codeViewEmitter_.generateDebugSSymbols();
    auto debugT = codeViewEmitter_.generateDebugTTypes();
    auto debugF = codeViewEmitter_.generateDebugFFiles();

    // Combinar todas las secciones
    std::vector<uint8_t> completeDebug;
    completeDebug.insert(completeDebug.end(), debugS.begin(), debugS.end());
    completeDebug.insert(completeDebug.end(), debugT.begin(), debugT.end());
    completeDebug.insert(completeDebug.end(), debugF.begin(), debugF.end());

    return completeDebug;
}

void DebugIntegration::processDeclarationForDebug(const ast::ASTNode* decl,
                                                const std::filesystem::path& sourceFile) {
    if (!decl) return;

    // Procesar diferentes tipos de declaraciones
    switch (decl->getType()) {
        case ast::ASTNodeType::FunctionDeclaration:
            processFunctionForDebug(decl, sourceFile);
            break;
        case ast::ASTNodeType::VariableDeclaration:
            processVariableForDebug(decl, sourceFile);
            break;
        default:
            // Otros tipos de declaraciones
            break;
    }
}

void DebugIntegration::processFunctionForDebug(const ast::ASTNode* func,
                                             const std::filesystem::path& sourceFile) {
    // Extraer información de función para debug
    std::string funcName = "unknown_function"; // TODO: extraer nombre real

    DebugSymbol funcSymbol(CodeViewRecordType::S_GPROC32, funcName);
    funcSymbol.address = calculateDebugRVA(funcName);
    funcSymbol.size = 0; // TODO: calcular tamaño

    codeViewEmitter_.addDebugSymbol(funcSymbol);

    // Añadir información de línea
    auto lineInfo = extractSourceLocation(func, sourceFile);
    std::vector<SourceLineInfo> lines = {lineInfo};
    codeViewEmitter_.addSourceFile(sourceFile, lines);
}

void DebugIntegration::processVariableForDebug(const ast::ASTNode* var,
                                             const std::filesystem::path& sourceFile) {
    // Extraer información de variable para debug
    std::string varName = "unknown_variable"; // TODO: extraer nombre real

    DebugSymbol varSymbol(CodeViewRecordType::S_GDATA32, varName);
    varSymbol.address = calculateDebugRVA(varName);
    varSymbol.size = 4; // TODO: calcular tamaño real

    codeViewEmitter_.addDebugSymbol(varSymbol);
}

void DebugIntegration::processTypeForDebug(const Type& type) {
    // Procesar información de tipo para debug
    DebugType debugType(CodeViewUtils::convertToCodeViewType(type));
    debugType.typeIndex = 0; // TODO: asignar índice único

    codeViewEmitter_.addDebugType(debugType);
}

SourceLineInfo DebugIntegration::extractSourceLocation(const ast::ASTNode* node,
                                                    const std::filesystem::path& sourceFile) {
    // Extraer ubicación fuente del AST
    // Implementación simplificada
    return SourceLineInfo(sourceFile.string(), 1, 0);
}

uint32_t DebugIntegration::calculateDebugRVA(const std::string& symbolName) {
    // Calcular RVA para debug
    // En un compilador real, esto consultaría la tabla de símbolos
    return 0x1000; // Placeholder
}

} // namespace cpp20::compiler::debug
