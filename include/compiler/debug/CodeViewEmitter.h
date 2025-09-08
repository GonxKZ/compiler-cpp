/**
 * @file CodeViewEmitter.h
 * @brief Emisor de información de debug en formato CodeView para Windows
 */

#pragma once

#include <compiler/ir/IR.h>
#include <compiler/ast/ASTNode.h>
#include <compiler/types/Type.h>
#include <compiler/symbols/Symbol.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <filesystem>

namespace cpp20::compiler::debug {

/**
 * @brief Tipos de registros CodeView
 */
enum class CodeViewRecordType {
    // Símbolos
    S_COMPILE = 0x0001,        // Información de compilación
    S_REGISTER = 0x0002,       // Variable en registro
    S_CONSTANT = 0x0003,       // Constante
    S_UDT = 0x0004,           // Tipo definido por usuario
    S_SSEARCH = 0x0005,       // Búsqueda de símbolo
    S_END = 0x0006,           // Fin de bloque
    S_SKIP = 0x0007,          // Skip record
    S_CVRESERVE = 0x0008,     // Reservado
    S_OBJNAME = 0x0009,       // Nombre de objeto
    S_ENDARG = 0x000A,        // Fin de argumentos
    S_COBOLUDT = 0x000B,      // UDT COBOL
    S_MANYREG = 0x000C,       // Variable en múltiples registros
    S_RETURN = 0x000D,        // Información de retorno
    S_ENTRYTHIS = 0x000E,     // This entry point
    S_BPREL16 = 0x0100,       // BP-relative, 16-bit
    S_LDATA16 = 0x0101,       // Local data, 16-bit
    S_GDATA16 = 0x0102,       // Global data, 16-bit
    S_PUB16 = 0x0103,         // Public symbol, 16-bit
    S_LPROC16 = 0x0104,       // Local procedure, 16-bit
    S_GPROC16 = 0x0105,       // Global procedure, 16-bit
    S_THUNK16 = 0x0106,       // Thunk, 16-bit
    S_BLOCK16 = 0x0107,       // Block, 16-bit
    S_WITH16 = 0x0108,        // With, 16-bit
    S_LABEL16 = 0x0109,       // Label, 16-bit
    S_CEXMODEL16 = 0x010A,    // C++ exception model, 16-bit
    S_VFTABLE16 = 0x010B,     // Virtual function table, 16-bit
    S_REGREL16 = 0x010C,      // Register-relative, 16-bit
    S_BPREL32 = 0x0200,       // BP-relative, 32-bit
    S_LDATA32 = 0x0201,       // Local data, 32-bit
    S_GDATA32 = 0x0202,       // Global data, 32-bit
    S_PUB32 = 0x0203,         // Public symbol, 32-bit
    S_LPROC32 = 0x0204,       // Local procedure, 32-bit
    S_GPROC32 = 0x0205,       // Global procedure, 32-bit
    S_THUNK32 = 0x0206,       // Thunk, 32-bit
    S_BLOCK32 = 0x0207,       // Block, 32-bit
    S_WITH32 = 0x0208,        // With, 32-bit
    S_LABEL32 = 0x0209,       // Label, 32-bit
    S_CEXMODEL32 = 0x020A,    // C++ exception model, 32-bit
    S_VFTABLE32 = 0x020B,     // Virtual function table, 32-bit
    S_REGREL32 = 0x020C,      // Register-relative, 32-bit
    S_LTHREAD32 = 0x020D,     // TLS local data, 32-bit
    S_GTHREAD32 = 0x020E,     // TLS global data, 32-bit
    S_SLINK32 = 0x020F,       // Static link, 32-bit
    S_LPROCMIPS = 0x0300,     // Local procedure, MIPS
    S_GPROCMIPS = 0x0301,     // Global procedure, MIPS
    S_PROCREF = 0x0400,       // Procedure reference
    S_DATAREF = 0x0401,       // Data reference
    S_ALIGN = 0x0402,         // Alignment
    S_LPROCREF = 0x0403,      // Local procedure reference
    S_TI16_MAX = 0x1000,
    S_TI16_MAX = 0x1000,
    S_REGISTER_ST = 0x1001,   // Register variable, ST version
    S_CONSTANT_ST = 0x1002,   // Constant, ST version
    S_UDT_ST = 0x1003,        // UDT, ST version
    S_COBOLUDT_ST = 0x1004,   // COBOL UDT, ST version
    S_MANYREG_ST = 0x1005,    // Many register variable, ST version
    S_BPREL32_ST = 0x1006,    // BP-relative, 32-bit, ST version
    S_LDATA32_ST = 0x1007,    // Local data, 32-bit, ST version
    S_GDATA32_ST = 0x1008,    // Global data, 32-bit, ST version
    S_PUB32_ST = 0x1009,      // Public symbol, 32-bit, ST version
    S_LPROC32_ST = 0x100A,    // Local procedure, 32-bit, ST version
    S_GPROC32_ST = 0x100B,    // Global procedure, 32-bit, ST version
    S_LPROCMIPS_ST = 0x100C,  // Local procedure, MIPS, ST version
    S_GPROCMIPS_ST = 0x100D,  // Global procedure, MIPS, ST version
    S_FRAMEPROC = 0x1012,     // Frame procedure
    S_COMPILE2_ST = 0x1013,   // Compile flags, ST version
    S_MANYREG2_ST = 0x1014,   // Many register variable, ST version
    S_LPROCIA64_ST = 0x1015,  // Local procedure, IA64, ST version
    S_GPROCIA64_ST = 0x1016,  // Global procedure, IA64, ST version
    S_LOCALSLOT_ST = 0x1017,  // Local slot, ST version
    S_PARAMSLOT_ST = 0x1018,  // Parameter slot, ST version
    S_ANNOTATION = 0x1019,    // Annotation
    S_GMANPROC_ST = 0x101A,   // Global managed procedure, ST version
    S_LMANPROC_ST = 0x101B,   // Local managed procedure, ST version
    S_RESERVED1 = 0x101C,     // Reserved
    S_RESERVED2 = 0x101D,     // Reserved
    S_RESERVED3 = 0x101E,     // Reserved
    S_RESERVED4 = 0x101F,     // Reserved
    S_LMANDATA_ST = 0x1020,   // Local managed data, ST version
    S_GMANDATA_ST = 0x1021,   // Global managed data, ST version
    S_MANFRAMEREL_ST = 0x1022,// Managed frame relative, ST version
    S_MANREGISTER_ST = 0x1023, // Managed register, ST version
    S_MANSLOT_ST = 0x1024,    // Managed slot, ST version
    S_MANMANYREG_ST = 0x1025, // Managed many register, ST version
    S_MANREGREL_ST = 0x1026,  // Managed register relative, ST version
    S_MANMANYREG2_ST = 0x1027,// Managed many register 2, ST version
    S_MANTYPREF = 0x1028,     // Managed type reference
    S_UNAMESPACE_ST = 0x1029, // Using namespace, ST version
    S_ST_MAX = 0x1100,        // End of ST versions
    S_OBJNAME_ST = 0x1101,    // Object name, ST version
    S_THUNK32_ST = 0x1102,    // Thunk, 32-bit, ST version
    S_BLOCK32_ST = 0x1103,    // Block, 32-bit, ST version
    S_WITH32_ST = 0x1104,     // With, 32-bit, ST version
    S_LABEL32_ST = 0x1105,    // Label, 32-bit, ST version
    S_REGISTER = 0x1106,      // Register variable
    S_CONSTANT = 0x1107,      // Constant
    S_UDT = 0x1108,           // User defined type
    S_COBOLUDT = 0x1109,      // COBOL user defined type
    S_MANYREG = 0x110A,       // Many register variable
    S_BPREL32 = 0x110B,       // BP-relative, 32-bit
    S_LDATA32 = 0x110C,       // Local data, 32-bit
    S_GDATA32 = 0x110D,       // Global data, 32-bit
    S_PUB32 = 0x110E,         // Public symbol, 32-bit
    S_LPROC32 = 0x110F,       // Local procedure, 32-bit
    S_GPROC32 = 0x1110,       // Global procedure, 32-bit
    S_THUNK32 = 0x1111,       // Thunk, 32-bit
    S_BLOCK32 = 0x1112,       // Block, 32-bit
    S_WITH32 = 0x1113,        // With, 32-bit
    S_LABEL32 = 0x1114,       // Label, 32-bit
    S_CEXMODEL32 = 0x1115,    // C++ exception model, 32-bit
    S_VFTABLE32 = 0x1116,     // Virtual function table, 32-bit
    S_REGREL32 = 0x1117,      // Register-relative, 32-bit
    S_LTHREAD32 = 0x1118,     // TLS local data, 32-bit
    S_GTHREAD32 = 0x1119,     // TLS global data, 32-bit
    S_SLINK32 = 0x111A,       // Static link, 32-bit
    S_LPROCMIPS = 0x111B,     // Local procedure, MIPS
    S_GPROCMIPS = 0x111C,     // Global procedure, MIPS
    S_PROCREF = 0x111D,       // Procedure reference
    S_DATAREF = 0x111E,       // Data reference
    S_ALIGN = 0x111F,         // Alignment
    S_LPROCREF = 0x1120,      // Local procedure reference

    // Tipos
    LF_MODIFIER = 0x1001,     // Modifier
    LF_POINTER = 0x1002,      // Pointer
    LF_ARRAY = 0x1003,        // Array
    LF_CLASS = 0x1004,        // Class
    LF_STRUCTURE = 0x1005,    // Structure
    LF_UNION = 0x1006,        // Union
    LF_ENUM = 0x1007,         // Enumeration
    LF_PROCEDURE = 0x1008,    // Procedure
    LF_MFUNCTION = 0x1009,    // Member function
    LF_VTSHAPE = 0x000A,      // Virtual table shape
    LF_COBOL0 = 0x100B,       // COBOL0
    LF_COBOL1 = 0x100C,       // COBOL1
    LF_BARRAY = 0x100D,       // Basic array
    LF_LABEL = 0x100E,        // Label
    LF_NULL = 0x100F,         // NULL
    LF_NOTTRAN = 0x1010,      // Not translated
    LF_DIMARRAY = 0x1011,     // Dimensioned array
    LF_VFTPATH = 0x1012,      // Virtual function table path
    LF_PRECOMP = 0x1013,      // Precompiled types
    LF_ENDPRECOMP = 0x1014,   // End precompiled types
    LF_OEM = 0x1015,          // OEM type
    LF_TYPESERVER = 0x1016,   // Type server
};

/**
 * @brief Información de línea de código fuente
 */
struct SourceLineInfo {
    std::string fileName;
    uint32_t lineNumber;
    uint32_t address;
    uint16_t columnStart;
    uint16_t columnEnd;

    SourceLineInfo(const std::string& file = "", uint32_t line = 0, uint32_t addr = 0,
                 uint16_t colStart = 0, uint16_t colEnd = 0)
        : fileName(file), lineNumber(line), address(addr),
          columnStart(colStart), columnEnd(colEnd) {}
};

/**
 * @brief Información de símbolo para debug
 */
struct DebugSymbol {
    CodeViewRecordType type;
    std::string name;
    uint32_t address;
    uint32_t size;
    std::string typeName;
    std::vector<uint8_t> data;

    DebugSymbol(CodeViewRecordType t, const std::string& n = "",
               uint32_t addr = 0, uint32_t sz = 0)
        : type(t), name(n), address(addr), size(sz) {}
};

/**
 * @brief Información de tipo para debug
 */
struct DebugType {
    CodeViewRecordType type;
    uint32_t typeIndex;
    std::vector<uint8_t> data;

    DebugType(CodeViewRecordType t, uint32_t idx = 0)
        : type(t), typeIndex(idx) {}
};

/**
 * @brief Archivo objeto con información de debug
 */
struct DebugObjectFile {
    std::filesystem::path path;
    std::vector<DebugSymbol> symbols;
    std::vector<DebugType> types;
    std::vector<SourceLineInfo> lineInfo;
    std::unordered_map<std::string, uint32_t> fileNameToIndex;

    DebugObjectFile(const std::filesystem::path& p) : path(p) {}
};

/**
 * @brief Emisor de información de debug CodeView
 */
class CodeViewEmitter {
public:
    /**
     * @brief Constructor
     */
    CodeViewEmitter();

    /**
     * @brief Destructor
     */
    ~CodeViewEmitter();

    /**
     * @brief Añade información de debug de un archivo fuente
     */
    void addSourceFile(const std::filesystem::path& sourceFile,
                      const std::vector<SourceLineInfo>& lineInfo);

    /**
     * @brief Añade símbolo de debug
     */
    void addDebugSymbol(const DebugSymbol& symbol);

    /**
     * @brief Añade tipo de debug
     */
    void addDebugType(const DebugType& type);

    /**
     * @brief Genera sección .debug$S (símbolos)
     */
    std::vector<uint8_t> generateDebugSSymbols();

    /**
     * @brief Genera sección .debug$T (tipos)
     */
    std::vector<uint8_t> generateDebugTTypes();

    /**
     * @brief Genera sección .debug$F (nombres de archivos)
     */
    std::vector<uint8_t> generateDebugFFiles();

    /**
     * @brief Genera información de línea completa
     */
    std::vector<uint8_t> generateLineNumbers();

    /**
     * @brief Establece arquitectura de destino
     */
    void setTargetArchitecture(const std::string& arch);

    /**
     * @brief Obtiene arquitectura de destino
     */
    std::string getTargetArchitecture() const { return targetArch_; }

    /**
     * @brief Limpia toda la información de debug
     */
    void clear();

    /**
     * @brief Obtiene estadísticas de debug
     */
    std::unordered_map<std::string, size_t> getDebugStatistics() const;

private:
    std::string targetArch_;
    std::vector<DebugSymbol> debugSymbols_;
    std::vector<DebugType> debugTypes_;
    std::vector<SourceLineInfo> sourceLines_;
    std::unordered_map<std::string, uint32_t> fileNameMap_;
    uint32_t nextTypeIndex_;
    uint32_t nextFileIndex_;

    /**
     * @brief Crea header de subsección CodeView
     */
    std::vector<uint8_t> createSubsectionHeader(uint32_t type, uint32_t size);

    /**
     * @brief Serializa un símbolo CodeView
     */
    std::vector<uint8_t> serializeSymbol(const DebugSymbol& symbol);

    /**
     * @brief Serializa un tipo CodeView
     */
    std::vector<uint8_t> serializeType(const DebugType& type);

    /**
     * @brief Serializa información de línea
     */
    std::vector<uint8_t> serializeLineInfo(const SourceLineInfo& lineInfo);

    /**
     * @brief Obtiene índice de archivo
     */
    uint32_t getFileIndex(const std::string& fileName);

    /**
     * @brief Calcula checksum de datos
     */
    uint32_t calculateChecksum(const std::vector<uint8_t>& data);

    /**
     * @brief Comprime información de línea
     */
    std::vector<uint8_t> compressLineInfo(const std::vector<SourceLineInfo>& lines);
};

/**
 * @brief Generador de PDB (Program Database)
 */
class PDBGenerator {
public:
    /**
     * @brief Constructor
     */
    PDBGenerator();

    /**
     * @brief Destructor
     */
    ~PDBGenerator();

    /**
     * @brief Añade información de debug
     */
    void addDebugInfo(const DebugObjectFile& debugInfo);

    /**
     * @brief Genera archivo PDB
     */
    bool generatePDB(const std::filesystem::path& pdbPath);

    /**
     * @brief Establece nombre del módulo
     */
    void setModuleName(const std::string& name);

    /**
     * @brief Establece timestamp
     */
    void setTimestamp(uint32_t timestamp);

private:
    std::string moduleName_;
    uint32_t timestamp_;
    std::vector<DebugObjectFile> debugObjects_;

    /**
     * @brief Crea stream de información del módulo
     */
    std::vector<uint8_t> createModuleInfoStream();

    /**
     * @brief Crea stream de símbolos globales
     */
    std::vector<uint8_t> createGlobalSymbolStream();

    /**
     * @brief Crea stream de tipos
     */
    std::vector<uint8_t> createTypeStream();

    /**
     * @brief Crea stream de información de línea
     */
    std::vector<uint8_t> createLineInfoStream();

    /**
     * @brief Calcula hash de contenido
     */
    std::string calculateContentHash(const std::vector<uint8_t>& data);
};

/**
 * @brief Utilidades para trabajar con CodeView
 */
class CodeViewUtils {
public:
    /**
     * @brief Convierte tipo C++ a tipo CodeView
     */
    static CodeViewRecordType convertToCodeViewType(const Type& type);

    /**
     * @brief Convierte símbolo a registro CodeView
     */
    static CodeViewRecordType convertToCodeViewSymbol(const Symbol& symbol);

    /**
     * @brief Genera información de línea para una función
     */
    static std::vector<SourceLineInfo> generateLineInfoForFunction(
        const std::string& functionName,
        const std::vector<uint32_t>& addresses,
        const std::vector<uint32_t>& lineNumbers);

    /**
     * @brief Verifica compatibilidad de versión CodeView
     */
    static bool isCompatibleVersion(uint16_t version);

    /**
     * @brief Obtiene tamaño de registro CodeView
     */
    static size_t getRecordSize(CodeViewRecordType type);

    /**
     * @brief Valida registro CodeView
     */
    static bool validateRecord(const std::vector<uint8_t>& record);

    /**
     * @brief Extrae nombre de registro CodeView
     */
    static std::string extractRecordName(const std::vector<uint8_t>& record);

    /**
     * @brief Formatea registro CodeView como string
     */
    static std::string formatRecord(const std::vector<uint8_t>& record);
};

/**
 * @brief Integrador de debug en el proceso de compilación
 */
class DebugIntegration {
public:
    /**
     * @brief Constructor
     */
    DebugIntegration();

    /**
     * @brief Destructor
     */
    ~DebugIntegration();

    /**
     * @brief Habilita generación de debug
     */
    void enableDebugGeneration(bool enable);

    /**
     * @brief Establece nivel de debug
     */
    void setDebugLevel(int level);

    /**
     * @brief Añade información de debug desde AST
     */
    void addDebugInfoFromAST(const ast::ASTNode* node,
                           const std::filesystem::path& sourceFile);

    /**
     * @brief Añade información de debug desde IR
     */
    void addDebugInfoFromIR(const ir::IRFunction* function);

    /**
     * @brief Genera información de debug completa
     */
    std::vector<uint8_t> generateCompleteDebugInfo();

    /**
     * @brief Obtiene emisor CodeView
     */
    CodeViewEmitter& getCodeViewEmitter() { return codeViewEmitter_; }

    /**
     * @brief Obtiene generador PDB
     */
    PDBGenerator& getPDBGenerator() { return pdbGenerator_; }

private:
    bool debugEnabled_;
    int debugLevel_;
    CodeViewEmitter codeViewEmitter_;
    PDBGenerator pdbGenerator_;

    /**
     * @brief Procesa declaración para debug
     */
    void processDeclarationForDebug(const ast::ASTNode* decl,
                                  const std::filesystem::path& sourceFile);

    /**
     * @brief Procesa función para debug
     */
    void processFunctionForDebug(const ast::ASTNode* func,
                               const std::filesystem::path& sourceFile);

    /**
     * @brief Procesa variable para debug
     */
    void processVariableForDebug(const ast::ASTNode* var,
                               const std::filesystem::path& sourceFile);

    /**
     * @brief Procesa tipo para debug
     */
    void processTypeForDebug(const Type& type);

    /**
     * @brief Extrae ubicación fuente
     */
    SourceLineInfo extractSourceLocation(const ast::ASTNode* node,
                                      const std::filesystem::path& sourceFile);

    /**
     * @brief Calcula dirección RVA para debug
     */
    uint32_t calculateDebugRVA(const std::string& symbolName);
};

} // namespace cpp20::compiler::debug
