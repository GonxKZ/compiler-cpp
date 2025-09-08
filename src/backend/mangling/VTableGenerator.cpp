/**
 * @file VTableGenerator.cpp
 * @brief Implementación del generador de VTables
 */

#include <compiler/backend/mangling/VTableGenerator.h>
#include <algorithm>
#include <cassert>

namespace cpp20::compiler::backend::mangling {

// ============================================================================
// VTableGenerator - Implementación
// ============================================================================

VTableGenerator::VTableGenerator() = default;
VTableGenerator::~VTableGenerator() = default;

std::vector<VTableEntry> VTableGenerator::generateVTable(const ClassLayout& layout) {
    std::vector<VTableEntry> entries;

    // 1. Generar entradas para funciones virtuales heredadas
    auto inheritedEntries = generateInheritedVirtualEntries(layout);
    entries.insert(entries.end(), inheritedEntries.begin(), inheritedEntries.end());

    // 2. Generar entradas para funciones virtuales propias
    auto ownEntries = generateOwnVirtualEntries(layout);
    entries.insert(entries.end(), ownEntries.begin(), ownEntries.end());

    // 3. Generar thunks si hay herencia múltiple
    auto thunkEntries = generateThunks(layout);
    entries.insert(entries.end(), thunkEntries.begin(), thunkEntries.end());

    // 4. Ordenar según reglas MSVC
    orderVTableEntries(entries);

    // 5. Asignar offsets finales
    uint32_t currentOffset = 0;
    for (auto& entry : entries) {
        entry.offset = currentOffset;
        currentOffset += sizeof(void*); // Cada entrada es un puntero
    }

    return entries;
}

RTTIInfo VTableGenerator::generateRTTIInfo(const ClassLayout& layout) {
    RTTIInfo rtti(layout.getDataMembers().empty() ? "class" : "struct",
                  nameMangler_.mangleClass({layout.getDataMembers().empty() ? "class" : "struct"}));

    // Añadir clases base
    for (const auto& inherit : layout.getInheritance()) {
        rtti.baseClasses.push_back(inherit.baseClass);
    }

    // Verificar si tiene destructor virtual
    for (const auto& vfunc : layout.getVirtualFunctions()) {
        if (vfunc.name.find("~") == 0) { // Destructor
            rtti.hasVirtualDestructor = true;
            break;
        }
    }

    return rtti;
}

std::vector<uint8_t> VTableGenerator::generateVTableData(const std::vector<VTableEntry>& entries) {
    std::vector<uint8_t> data;

    for (const auto& entry : entries) {
        // En un compilador real, aquí irían los punteros a las funciones
        // Para esta implementación, usamos placeholders

        // RVA de la función (4 bytes en little-endian)
        uint32_t functionRVA = 0x1000; // Placeholder
        if (entry.isPureVirtual) {
            functionRVA = 0; // Pure virtual functions point to null
        }

        const uint8_t* rvaBytes = reinterpret_cast<const uint8_t*>(&functionRVA);
        data.insert(data.end(), rvaBytes, rvaBytes + 4);
    }

    return data;
}

std::vector<uint8_t> VTableGenerator::generateRTTIData(const RTTIInfo& rttiInfo) {
    std::vector<uint8_t> data;

    // type_info structure (simplified)
    // En MSVC, type_info contiene:
    // - vtable pointer
    // - mangled name
    // - padding/alignment

    // VTable pointer (placeholder)
    uint32_t vtablePtr = 0x2000;
    const uint8_t* vtableBytes = reinterpret_cast<const uint8_t*>(&vtablePtr);
    data.insert(data.end(), vtableBytes, vtableBytes + 4);

    // Mangled name como string
    const std::string& mangledName = rttiInfo.mangledClassName;
    data.insert(data.end(), mangledName.begin(), mangledName.end());
    data.push_back(0); // Null terminator

    // Padding para alineación
    while (data.size() % 4 != 0) {
        data.push_back(0);
    }

    return data;
}

size_t VTableGenerator::calculateVTableSize(const std::vector<VTableEntry>& entries) {
    return entries.size() * sizeof(void*); // Cada entrada es un puntero
}

bool VTableGenerator::validateVTable(const std::vector<VTableEntry>& entries) {
    if (entries.empty()) {
        return true; // OK si no hay funciones virtuales
    }

    // Verificar que los offsets sean consecutivos
    uint32_t expectedOffset = 0;
    for (const auto& entry : entries) {
        if (entry.offset != expectedOffset) {
            return false;
        }
        expectedOffset += sizeof(void*);
    }

    // Verificar que no haya funciones puras virtuales al final
    // (MSVC permite funciones puras virtuales, pero tienen restricciones)

    return VTableGenerator::validateMSVCVTableRules(entries);
}

std::vector<VTableEntry> VTableGenerator::generateOwnVirtualEntries(const ClassLayout& layout) {
    std::vector<VTableEntry> entries;

    for (const auto& vfunc : layout.getVirtualFunctions()) {
        FunctionInfo funcInfo;
        funcInfo.name = vfunc.name;
        funcInfo.scope = "";
        funcInfo.parameterTypes = {};
        funcInfo.returnType = "";
        funcInfo.qualifiers = FunctionQualifiers::None;
        funcInfo.isVirtual = true;
        funcInfo.isStatic = false;
        funcInfo.isExternC = false;
        funcInfo.templateArgs = 0;
        std::string mangledName = nameMangler_.mangleFunction(funcInfo);

        entries.emplace_back(vfunc.name, mangledName, 0, vfunc.isPureVirtual, false);
    }

    return entries;
}

std::vector<VTableEntry> VTableGenerator::generateInheritedVirtualEntries(const ClassLayout& layout) {
    std::vector<VTableEntry> entries;

    // Para herencia simple, incluir funciones virtuales de la base
    // Esta es una simplificación - en la práctica sería más complejo
    for (const auto& inherit : layout.getInheritance()) {
        if (!inherit.isVirtual) {
            // Simular funciones virtuales heredadas
            // En un compilador real, esto vendría del layout de la clase base
            std::string baseFuncName = "inherited_func";
            FunctionInfo funcInfo;
            funcInfo.name = baseFuncName;
            funcInfo.scope = inherit.baseClass;
            funcInfo.parameterTypes = {};
            funcInfo.returnType = "";
            funcInfo.qualifiers = FunctionQualifiers::None;
            funcInfo.isVirtual = true;
            funcInfo.isStatic = false;
            funcInfo.isExternC = false;
            funcInfo.templateArgs = 0;
            std::string mangledName = nameMangler_.mangleFunction(funcInfo);

            entries.emplace_back(baseFuncName, mangledName, 0, false, true);
        }
    }

    return entries;
}

void VTableGenerator::orderVTableEntries(std::vector<VTableEntry>& entries) {
    // MSVC ordering rules:
    // 1. Funciones virtuales de clases base primero
    // 2. Luego funciones virtuales propias
    // 3. Thunks al final

    // Esta implementación es simplificada
    std::sort(entries.begin(), entries.end(),
              [](const VTableEntry& a, const VTableEntry& b) {
                  // Thunks van al final
                  if (a.isThunk && !b.isThunk) return false;
                  if (!a.isThunk && b.isThunk) return true;

                  // Funciones puras virtuales van al final
                  if (a.isPureVirtual && !b.isPureVirtual) return false;
                  if (!a.isPureVirtual && b.isPureVirtual) return true;

                  // Ordenar por nombre para consistencia
                  return a.functionName < b.functionName;
              });
}

std::vector<VTableEntry> VTableGenerator::generateThunks(const ClassLayout& layout) {
    std::vector<VTableEntry> thunks;

    // Generar thunks para herencia múltiple
    // Esta es una simplificación - en la práctica sería mucho más complejo
    for (const auto& inherit : layout.getInheritance()) {
        if (inherit.isVirtual) {
            // Crear thunk para ajustar 'this' pointer
            int32_t offset = calculateThunkOffset(inherit);

            std::string thunkName = generateThunkName("virtual_func", offset);
            FunctionInfo funcInfo;
            funcInfo.name = thunkName;
            funcInfo.scope = layout.getDataMembers().empty() ? "class" : "struct";
            funcInfo.parameterTypes = {};
            funcInfo.returnType = "";
            funcInfo.qualifiers = FunctionQualifiers::None;
            funcInfo.isVirtual = true;
            funcInfo.isStatic = false;
            funcInfo.isExternC = false;
            funcInfo.templateArgs = 0;
            std::string mangledName = nameMangler_.mangleFunction(funcInfo);

            thunks.emplace_back(thunkName, mangledName, 0, false, true);
        }
    }

    return thunks;
}

int32_t VTableGenerator::calculateThunkOffset(const InheritanceInfo& inheritance) const {
    // Simplificado - en la práctica dependería del layout exacto
    return static_cast<int32_t>(inheritance.offset);
}

std::string VTableGenerator::generateThunkName(const std::string& functionName,
                                              int32_t offset) const {
    return functionName + "_thunk_" + std::to_string(offset);
}

bool VTableGenerator::validateMSVCVTableRules(const std::vector<VTableEntry>& entries) {
    // Reglas de validación MSVC para vtables:

    // 1. Primera entrada nunca debe ser null (excepto para clases abstractas puras)
    if (!entries.empty() && entries[0].mangledName.empty() && !entries[0].isPureVirtual) {
        return false;
    }

    // 2. Funciones puras virtuales pueden tener RVA null, pero deben estar al final
    bool foundNonPureAfterPure = false;
    for (const auto& entry : entries) {
        if (foundNonPureAfterPure && entry.isPureVirtual) {
            return false; // Pure virtual después de non-pure
        }
        if (!entry.isPureVirtual) {
            foundNonPureAfterPure = true;
        }
    }

    // 3. Offsets deben ser consecutivos
    for (size_t i = 0; i < entries.size(); ++i) {
        uint32_t expectedOffset = static_cast<uint32_t>(i * sizeof(void*));
        if (entries[i].offset != expectedOffset) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// VTableUtils - Implementación
// ============================================================================

const VTableEntry* VTableUtils::findFunctionEntry(
    const std::vector<VTableEntry>& entries,
    const std::string& functionName) {

    for (const auto& entry : entries) {
        if (entry.functionName == functionName) {
            return &entry;
        }
    }
    return nullptr;
}

int VTableUtils::getFunctionIndex(
    const std::vector<VTableEntry>& entries,
    const std::string& functionName) {

    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].functionName == functionName) {
            return static_cast<int>(i);
        }
    }
    return -1; // Not found
}

bool VTableUtils::vtablesCompatible(
    const std::vector<VTableEntry>& vtable1,
    const std::vector<VTableEntry>& vtable2) {

    if (vtable1.size() != vtable2.size()) {
        return false;
    }

    for (size_t i = 0; i < vtable1.size(); ++i) {
        const auto& entry1 = vtable1[i];
        const auto& entry2 = vtable2[i];

        // Comparar nombres de función y offsets
        if (entry1.functionName != entry2.functionName ||
            entry1.offset != entry2.offset ||
            entry1.isPureVirtual != entry2.isPureVirtual) {
            return false;
        }
    }

    return true;
}

std::string VTableUtils::generateTypeComparisonCode(
    const RTTIInfo& rtti1,
    const RTTIInfo& rtti2) {

    // Generar código C++ para comparar type_info
    std::string code = "bool compare_types(const std::type_info& t1, const std::type_info& t2) {\n";
    code += "    return t1 == t2;\n";
    code += "}\n";

    // También generar comparación de nombres mangled
    code += "\n// Comparación de nombres mangled\n";
    code += "bool compare_mangled_names(const std::string& name1, const std::string& name2) {\n";
    code += "    return name1 == name2;\n";
    code += "}\n";

    return code;
}

bool VTableUtils::isDerivedFrom(
    const RTTIInfo& derived,
    const RTTIInfo& base) {

    // Verificar si 'derived' hereda de 'base'
    for (const auto& baseClass : derived.baseClasses) {
        if (baseClass == base.className) {
            return true;
        }
    }

    return false;
}

} // namespace cpp20::compiler::backend::mangling
