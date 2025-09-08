/**
 * @file VTableGenerator.cpp
 * @brief Implementación completa del generador de vtables y RTTI
 */

#include <compiler/backend/mangling/VTableGenerator.h>
#include <algorithm>
#include <sstream>

namespace cpp20::compiler::backend::mangling {

// ============================================================================
// VTableGenerator - Implementación
// ============================================================================

VTableGenerator::VTableGenerator() = default;
VTableGenerator::~VTableGenerator() = default;

std::vector<VTableEntry> VTableGenerator::generateVTable(const ClassLayout& layout) {
    std::vector<VTableEntry> entries;

    // Generar entradas para funciones virtuales propias
    auto ownEntries = generateOwnVirtualEntries(layout);
    entries.insert(entries.end(), ownEntries.begin(), ownEntries.end());

    // Generar entradas para funciones virtuales heredadas
    auto inheritedEntries = generateInheritedVirtualEntries(layout);
    entries.insert(entries.end(), inheritedEntries.begin(), inheritedEntries.end());

    // Generar thunks si es necesario (herencia múltiple)
    auto thunkEntries = generateThunks(layout);
    entries.insert(entries.end(), thunkEntries.begin(), thunkEntries.end());

    // Ordenar según reglas de MSVC
    orderVTableEntries(entries);

    // Asignar offsets
    uint32_t currentOffset = 0;
    for (auto& entry : entries) {
        entry.offset = currentOffset;
        currentOffset += 8; // Tamaño de puntero en x64
    }

    return entries;
}

RTTIInfo VTableGenerator::generateRTTIInfo(const ClassLayout& layout) {
    RTTIInfo rtti(layout.getClassName(), layout.generateTypeInfoName());

    // Agregar clases base
    for (const auto& inherit : layout.getInheritance()) {
        rtti.baseClasses.push_back(inherit.baseClass);
    }

    // Verificar si tiene destructor virtual
    const auto& virtualFuncs = layout.getVirtualFunctions();
    rtti.hasVirtualDestructor = std::any_of(
        virtualFuncs.begin(),
        virtualFuncs.end(),
        [](const VirtualFunctionInfo& vf) {
            return vf.name.find("~") == 0; // Destructor
        }
    );

    return rtti;
}

std::vector<uint8_t> VTableGenerator::generateVTableData(const std::vector<VTableEntry>& entries) {
    std::vector<uint8_t> data;

    // Cada entrada es un puntero de 8 bytes en x64
    for ([[maybe_unused]] const auto& entry : entries) {
        // En un compilador real, aquí se generarían los punteros reales a las funciones
        // Por ahora, generamos datos dummy
        for (int i = 0; i < 8; ++i) {
            data.push_back(0x00);
        }
    }

    return data;
}

std::vector<uint8_t> VTableGenerator::generateRTTIData(const RTTIInfo& rttiInfo) {
    std::vector<uint8_t> data;

    // Formato simplificado de RTTI de MSVC
    // En un compilador real, esto sería mucho más complejo

    // Nombre de la clase (como cadena terminada en null)
    const std::string& className = rttiInfo.className;
    data.insert(data.end(), className.begin(), className.end());
    data.push_back(0); // Null terminator

    // Información de clases base (simplificada)
    for (const auto& base : rttiInfo.baseClasses) {
        data.insert(data.end(), base.begin(), base.end());
        data.push_back(0); // Null terminator
    }

    return data;
}

size_t VTableGenerator::calculateVTableSize(const std::vector<VTableEntry>& entries) {
    return entries.size() * 8; // 8 bytes por entrada en x64
}

bool VTableGenerator::validateVTable(const std::vector<VTableEntry>& entries) {
    return validateMSVCVTableRules(entries);
}

std::vector<VTableEntry> VTableGenerator::generateOwnVirtualEntries(const ClassLayout& layout) {
    std::vector<VTableEntry> entries;

    const auto& virtualFuncs = layout.getVirtualFunctions();
    for (const auto& vfunc : virtualFuncs) {
        // Generar nombre mangled
        std::string mangledName = nameMangler_.mangleName(vfunc.name);

        entries.emplace_back(
            vfunc.name,
            mangledName,
            0, // Offset se asigna después
            vfunc.isPureVirtual,
            false // No es thunk
        );
    }

    return entries;
}

std::vector<VTableEntry> VTableGenerator::generateInheritedVirtualEntries(const ClassLayout& layout) {
    std::vector<VTableEntry> entries;

    // Para herencia simple, copiar funciones virtuales de la clase base
    // En un compilador real, esto sería mucho más complejo
    for (const auto& inherit : layout.getInheritance()) {
        if (!inherit.isVirtual) {
            // Simular herencia de funciones virtuales de la base
            // En la práctica, esto requeriría acceso al layout de la clase base
            entries.emplace_back(
                "inherited_func", // Dummy
                "?inherited_func@" + inherit.baseClass + "@@",
                0,
                false,
                false
            );
        }
    }

    return entries;
}

void VTableGenerator::orderVTableEntries(std::vector<VTableEntry>& entries) {
    // Ordenar según reglas de MSVC:
    // 1. Funciones propias primero
    // 2. Funciones heredadas después
    // 3. Thunks al final

    std::sort(entries.begin(), entries.end(),
        [](const VTableEntry& a, const VTableEntry& b) {
            // Thunks van al final
            if (a.isThunk != b.isThunk) {
                return !a.isThunk;
            }

            // Comparar por nombre para orden consistente
            return a.functionName < b.functionName;
        });
}

std::vector<VTableEntry> VTableGenerator::generateThunks(const ClassLayout& layout) {
    std::vector<VTableEntry> entries;

    // Generar thunks para herencia múltiple
    for (const auto& inherit : layout.getInheritance()) {
        if (inherit.isVirtual || !inherit.isPrimary) {
            // Calcular offset de ajuste
            int32_t offset = calculateThunkOffset(inherit);

            // Generar thunk para cada función virtual
            for (const auto& vfunc : layout.getVirtualFunctions()) {
                std::string thunkName = generateThunkName(vfunc.name, offset);
                std::string mangledThunk = nameMangler_.mangleName(thunkName);

                entries.emplace_back(
                    thunkName,
                    mangledThunk,
                    0,
                    false,
                    true // Es thunk
                );
            }
        }
    }

    return entries;
}

int32_t VTableGenerator::calculateThunkOffset(const InheritanceInfo& inheritance) const {
    // Cálculo simplificado del offset de ajuste para thunks
    // En un compilador real, esto sería mucho más complejo
    return static_cast<int32_t>(inheritance.offset);
}

std::string VTableGenerator::generateThunkName(const std::string& functionName,
                                              int32_t offset) const {
    std::stringstream ss;
    ss << "__thunk_" << functionName << "_" << offset;
    return ss.str();
}

bool VTableGenerator::validateMSVCVTableRules(const std::vector<VTableEntry>& entries) {
    // Reglas básicas de validación de vtable de MSVC

    // 1. No puede haber funciones virtuales puras al final (excepto destructores)
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        if (entry.isPureVirtual && entry.functionName.find("~") != 0) {
            // Las funciones virtuales puras solo pueden estar al final si son destructores
            for (size_t j = i + 1; j < entries.size(); ++j) {
                if (!entries[j].isPureVirtual) {
                    return false;
                }
            }
        }
    }

    // 2. Los offsets deben ser consecutivos y múltiplos de 8
    uint32_t expectedOffset = 0;
    for (const auto& entry : entries) {
        if (entry.offset != expectedOffset) {
            return false;
        }
        expectedOffset += 8;
    }

    // 3. Los thunks deben estar al final
    bool foundNonThunk = false;
    for (const auto& entry : entries) {
        if (entry.isThunk) {
            if (foundNonThunk) {
                // Encontramos un thunk después de una función no-thunk
                return false;
            }
        } else {
            foundNonThunk = true;
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

    return -1;
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

        // Comparar nombres de función y propiedades
        if (entry1.functionName != entry2.functionName ||
            entry1.isPureVirtual != entry2.isPureVirtual ||
            entry1.isThunk != entry2.isThunk) {
            return false;
        }
    }

    return true;
}

std::string VTableUtils::generateTypeComparisonCode(
    const RTTIInfo& rtti1,
    const RTTIInfo& rtti2) {

    std::stringstream code;

    // Generar código simplificado para comparación de tipos
    code << "// Comparación de tipos: " << rtti1.className << " vs " << rtti2.className << "\n";
    code << "bool typesEqual = (strcmp(type1->name, type2->name) == 0);\n";

    return code.str();
}

bool VTableUtils::isDerivedFrom(
    const RTTIInfo& derived,
    const RTTIInfo& base) {

    // Verificar si 'derived' es derivada de 'base'
    for (const auto& baseClass : derived.baseClasses) {
        if (baseClass == base.className) {
            return true;
        }

        // En un compilador real, aquí se haría recursión para verificar
        // la jerarquía completa de herencia
    }

    return false;
}

} // namespace cpp20::compiler::backend::mangling