/**
 * @file VTableGenerator.h
 * @brief Generador de tablas de funciones virtuales (vtables)
 */

#pragma once

#include <compiler/backend/mangling/ClassLayout.h>
#include <vector>
#include <string>
#include <memory>

namespace cpp20::compiler::backend::mangling {

/**
 * @brief Entrada en la vtable
 */
struct VTableEntry {
    std::string functionName;       // Nombre de la función
    std::string mangledName;        // Nombre mangled
    uint32_t offset;                // Offset en la vtable
    bool isPureVirtual;             // Es pura virtual
    bool isThunk;                   // Es thunk (para herencia múltiple)

    VTableEntry(const std::string& name, const std::string& mangled,
                uint32_t off, bool pure = false, bool thunk = false)
        : functionName(name), mangledName(mangled), offset(off),
          isPureVirtual(pure), isThunk(thunk) {}
};

/**
 * @brief Información completa de RTTI
 */
struct RTTIInfo {
    std::string className;          // Nombre de la clase
    std::string mangledClassName;   // Nombre mangled
    std::vector<std::string> baseClasses; // Clases base
    bool hasVirtualDestructor;      // Tiene destructor virtual
    uint32_t typeInfoOffset;        // Offset del type_info

    RTTIInfo(const std::string& name, const std::string& mangled)
        : className(name), mangledClassName(mangled),
          hasVirtualDestructor(false), typeInfoOffset(0) {}
};

/**
 * @brief Generador de VTables compatible con MSVC
 */
class VTableGenerator {
public:
    /**
     * @brief Constructor
     */
    VTableGenerator();

    /**
     * @brief Destructor
     */
    ~VTableGenerator();

    /**
     * @brief Genera vtable para una clase
     * @param layout Layout de la clase
     * @return Vector de entradas de vtable
     */
    std::vector<VTableEntry> generateVTable(const ClassLayout& layout);

    /**
     * @brief Genera información RTTI
     * @param layout Layout de la clase
     * @return Información RTTI
     */
    RTTIInfo generateRTTIInfo(const ClassLayout& layout);

    /**
     * @brief Genera datos binarios de la vtable
     * @param entries Entradas de la vtable
     * @return Datos binarios
     */
    std::vector<uint8_t> generateVTableData(const std::vector<VTableEntry>& entries);

    /**
     * @brief Genera datos binarios de RTTI
     * @param rttiInfo Información RTTI
     * @return Datos binarios
     */
    std::vector<uint8_t> generateRTTIData(const RTTIInfo& rttiInfo);

    /**
     * @brief Calcula el tamaño de la vtable
     */
    static size_t calculateVTableSize(const std::vector<VTableEntry>& entries);

    /**
     * @brief Verifica que una vtable sea compatible con MSVC
     */
    static bool validateVTable(const std::vector<VTableEntry>& entries);

private:
    MSVCNameMangler nameMangler_;   // Para generar nombres mangled

    /**
     * @brief Genera entradas para funciones virtuales propias
     */
    std::vector<VTableEntry> generateOwnVirtualEntries(const ClassLayout& layout);

    /**
     * @brief Genera entradas para funciones virtuales heredadas
     */
    std::vector<VTableEntry> generateInheritedVirtualEntries(const ClassLayout& layout);

    /**
     * @brief Ordena entradas de vtable según reglas MSVC
     */
    void orderVTableEntries(std::vector<VTableEntry>& entries);

    /**
     * @brief Genera thunks para herencia múltiple
     */
    std::vector<VTableEntry> generateThunks(const ClassLayout& layout);

    /**
     * @brief Calcula offset de ajuste para thunk
     */
    int32_t calculateThunkOffset(const InheritanceInfo& inheritance) const;

    /**
     * @brief Genera nombre mangled para thunk
     */
    std::string generateThunkName(const std::string& functionName,
                                 int32_t offset) const;

    /**
     * @brief Verifica reglas de layout de vtable de MSVC
     */
    static bool validateMSVCVTableRules(const std::vector<VTableEntry>& entries);
};

/**
 * @brief Utilidades para trabajar con vtables
 */
class VTableUtils {
public:
    /**
     * @brief Encuentra entrada de función en vtable
     */
    static const VTableEntry* findFunctionEntry(
        const std::vector<VTableEntry>& entries,
        const std::string& functionName);

    /**
     * @brief Calcula índice de función en vtable
     */
    static int getFunctionIndex(
        const std::vector<VTableEntry>& entries,
        const std::string& functionName);

    /**
     * @brief Verifica que dos vtables sean compatibles
     */
    static bool vtablesCompatible(
        const std::vector<VTableEntry>& vtable1,
        const std::vector<VTableEntry>& vtable2);

    /**
     * @brief Genera código de comparación de type_info
     */
    static std::string generateTypeComparisonCode(
        const RTTIInfo& rtti1,
        const RTTIInfo& rtti2);

    /**
     * @brief Verifica herencia dinámica
     */
    static bool isDerivedFrom(
        const RTTIInfo& derived,
        const RTTIInfo& base);
};

} // namespace cpp20::compiler::backend::mangling
