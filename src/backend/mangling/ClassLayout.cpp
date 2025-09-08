/**
 * @file ClassLayout.cpp
 * @brief Implementación del layout de clases MSVC
 */

#include <compiler/backend/mangling/ClassLayout.h>
#include <algorithm>
#include <cassert>

namespace cpp20::compiler::backend::mangling {

// ============================================================================
// ClassLayout - Implementación
// ============================================================================

ClassLayout::ClassLayout(const std::string& className, const std::string& scope)
    : className_(className), scope_(scope), totalSize_(0), alignment_(1),
      vtableOffset_(0), layoutComputed_(false) {
}

ClassLayout::~ClassLayout() = default;

void ClassLayout::addDataMember(const MemberInfo& member) {
    assert(!layoutComputed_ && "Cannot add members after layout computation");
    dataMembers_.push_back(member);
}

void ClassLayout::addVirtualFunction(const VirtualFunctionInfo& vfunc) {
    assert(!layoutComputed_ && "Cannot add virtual functions after layout computation");
    virtualFunctions_.push_back(vfunc);
}

void ClassLayout::addInheritance(const InheritanceInfo& inheritance) {
    assert(!layoutComputed_ && "Cannot add inheritance after layout computation");
    inheritance_.push_back(inheritance);
}

void ClassLayout::computeLayout() {
    if (layoutComputed_) return;

    // 1. Calcular layout de herencia primero
    computeInheritanceLayout();

    // 2. Calcular offsets de miembros de datos
    computeDataMemberOffsets();

    // 3. Calcular layout de funciones virtuales
    computeVirtualFunctionLayout();

    // 4. Calcular tamaño total y alineación
    computeSizeAndAlignment();

    layoutComputed_ = true;
}

void ClassLayout::computeDataMemberOffsets() {
    size_t currentOffset = 0;

    // Si hay funciones virtuales, reservar espacio para vtable pointer
    if (hasVirtualFunctions()) {
        // En MSVC, el vtable pointer está al inicio de la clase
        vtableOffset_ = 0;
        currentOffset = sizeof(void*); // Tamaño de puntero
    }

    // Aplicar reglas de herencia primero
    for (const auto& inherit : inheritance_) {
        if (!inherit.isVirtual) {
            currentOffset = std::max(currentOffset, inherit.offset);
        }
    }

    // Calcular offsets para miembros de datos
    for (auto& member : dataMembers_) {
        if (member.isStatic) continue; // Miembros static no ocupan espacio en instancia

        size_t memberSize = getTypeSize(member.type);
        size_t memberAlign = getTypeAlignment(member.type);

        // Alinear el offset
        currentOffset = alignOffset(currentOffset, memberAlign);

        member.offset = currentOffset;
        currentOffset += memberSize;
    }
}

void ClassLayout::computeInheritanceLayout() {
    // MSVC layout rules para herencia:
    // 1. Base classes primero, en orden de declaración
    // 2. Miembros de datos después
    // 3. Padding para alineación

    size_t currentOffset = 0;

    for (auto& inherit : inheritance_) {
        if (inherit.isVirtual) {
            // Herencia virtual - el offset se determina en runtime
            inherit.offset = 0; // Placeholder
        } else {
            // Herencia regular - colocar al inicio
            inherit.offset = currentOffset;
            // Simplificado: asumir tamaño de base class
            currentOffset += 8; // Placeholder size
        }
    }
}

void ClassLayout::computeVirtualFunctionLayout() {
    if (!hasVirtualFunctions()) return;

    // Asignar índices de vtable a funciones virtuales
    int nextIndex = 0;

    for (auto& vfunc : virtualFunctions_) {
        vfunc.vtableIndex = nextIndex++;
    }

    // En MSVC, las funciones virtuales de bases se incluyen primero
    // Esta es una simplificación - en la práctica sería más complejo
}

void ClassLayout::computeSizeAndAlignment() {
    if (dataMembers_.empty() && inheritance_.empty()) {
        totalSize_ = hasVirtualFunctions() ? sizeof(void*) : 1; // Al menos 1 byte
        alignment_ = hasVirtualFunctions() ? alignof(void*) : 1;
        return;
    }

    size_t maxAlignment = 1;
    size_t currentSize = 0;

    // Considerar alineación de miembros de datos
    for (const auto& member : dataMembers_) {
        if (member.isStatic) continue;

        size_t memberAlign = getTypeAlignment(member.type);
        maxAlignment = std::max(maxAlignment, memberAlign);

        size_t memberEnd = member.offset + getTypeSize(member.type);
        currentSize = std::max(currentSize, memberEnd);
    }

    // Considerar alineación de bases
    for (const auto& inherit : inheritance_) {
        if (!inherit.isVirtual) {
            // Simplificado - en la práctica necesitaríamos el layout de la base
            maxAlignment = std::max(maxAlignment, alignof(void*));
        }
    }

    // Alinear el tamaño final
    totalSize_ = alignOffset(currentSize, maxAlignment);
    alignment_ = maxAlignment;
}

size_t ClassLayout::getTypeSize(const std::string& typeName) const {
    // Simplificado - solo tipos básicos comunes
    if (typeName == "int" || typeName == "unsigned int") return 4;
    if (typeName == "long" || typeName == "unsigned long") return 4;
    if (typeName == "long long" || typeName == "unsigned long long") return 8;
    if (typeName == "short" || typeName == "unsigned short") return 2;
    if (typeName == "char" || typeName == "unsigned char" || typeName == "bool") return 1;
    if (typeName == "float") return 4;
    if (typeName == "double") return 8;
    if (typeName == "long double") return 8;
    if (typeName == "void*") return sizeof(void*);

    // Para tipos complejos, asumir tamaño de puntero
    return sizeof(void*);
}

size_t ClassLayout::getTypeAlignment(const std::string& typeName) const {
    // Alineaciones típicas de MSVC
    if (typeName == "long long" || typeName == "unsigned long long") return 8;
    if (typeName == "double" || typeName == "long double") return 8;
    if (typeName == "int" || typeName == "unsigned int") return 4;
    if (typeName == "long" || typeName == "unsigned long") return 4;
    if (typeName == "short" || typeName == "unsigned short") return 2;
    if (typeName == "char" || typeName == "unsigned char" || typeName == "bool") return 1;
    if (typeName == "float") return 4;
    if (typeName == "void*") return alignof(void*);

    // Para tipos complejos, asumir alineación de puntero
    return alignof(void*);
}

size_t ClassLayout::alignOffset(size_t offset, size_t alignment) const {
    if (alignment == 0) return offset;
    return ((offset + alignment - 1) / alignment) * alignment;
}

std::string ClassLayout::generateVTableName() const {
    ClassInfo classInfo{className_, scope_, false, hasVirtualFunctions(), 0};
    return nameMangler_.mangleClass(classInfo) + "@@6B@"; // VTable suffix
}

std::string ClassLayout::generateTypeInfoName() const {
    ClassInfo classInfo{className_, scope_, false, false, 0};
    return nameMangler_.mangleClass(classInfo) + "@@8type_info@@"; // type_info suffix
}

bool ClassLayout::isMSVCCompatible() const {
    return validateMSVCRules();
}

bool ClassLayout::validateMSVCRules() const {
    // Reglas básicas de validación MSVC:

    // 1. Vtable pointer debe estar al inicio si hay funciones virtuales
    if (hasVirtualFunctions() && vtableOffset_ != 0) {
        return false;
    }

    // 2. Offsets deben estar alineados correctamente
    for (const auto& member : dataMembers_) {
        if (!member.isStatic) {
            size_t requiredAlign = getTypeAlignment(member.type);
            if ((member.offset % requiredAlign) != 0) {
                return false;
            }
        }
    }

    // 3. Tamaño total debe ser múltiplo de la alineación
    if ((totalSize_ % alignment_) != 0) {
        return false;
    }

    // 4. No debe haber overlaps entre miembros
    // Esta sería una validación más compleja en la práctica

    return true;
}

// ============================================================================
// ClassLayoutGenerator - Implementación
// ============================================================================

std::unique_ptr<ClassLayout> ClassLayoutGenerator::createSimpleClass(
    const std::string& className,
    const std::vector<MemberInfo>& members) {

    auto layout = std::make_unique<ClassLayout>(className);

    for (const auto& member : members) {
        layout->addDataMember(member);
    }

    layout->computeLayout();
    return layout;
}

std::unique_ptr<ClassLayout> ClassLayoutGenerator::createInheritedClass(
    const std::string& className,
    const std::vector<InheritanceInfo>& bases,
    const std::vector<MemberInfo>& members) {

    auto layout = std::make_unique<ClassLayout>(className);

    for (const auto& base : bases) {
        layout->addInheritance(base);
    }

    for (const auto& member : members) {
        layout->addDataMember(member);
    }

    layout->computeLayout();
    return layout;
}

std::unique_ptr<ClassLayout> ClassLayoutGenerator::createPolymorphicClass(
    const std::string& className,
    const std::vector<MemberInfo>& members,
    const std::vector<VirtualFunctionInfo>& virtualFuncs) {

    auto layout = std::make_unique<ClassLayout>(className);

    for (const auto& member : members) {
        layout->addDataMember(member);
    }

    for (const auto& vfunc : virtualFuncs) {
        layout->addVirtualFunction(vfunc);
    }

    layout->computeLayout();
    return layout;
}

bool ClassLayoutGenerator::validateLayout(const ClassLayout& layout) {
    return layout.isMSVCCompatible();
}

bool ClassLayoutGenerator::layoutsCompatible(const ClassLayout& layout1, const ClassLayout& layout2) {
    // Verificar tamaños
    if (layout1.getSize() != layout2.getSize()) {
        return false;
    }

    // Verificar alineaciones
    if (layout1.getAlignment() != layout2.getAlignment()) {
        return false;
    }

    // Verificar miembros (simplificado)
    const auto& members1 = layout1.getDataMembers();
    const auto& members2 = layout2.getDataMembers();

    if (members1.size() != members2.size()) {
        return false;
    }

    // Verificar offsets de miembros
    for (size_t i = 0; i < members1.size(); ++i) {
        if (members1[i].offset != members2[i].offset ||
            members1[i].type != members2[i].type) {
            return false;
        }
    }

    return true;
}

} // namespace cpp20::compiler::backend::mangling
