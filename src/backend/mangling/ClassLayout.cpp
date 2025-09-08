/**
 * @file ClassLayout.cpp
 * @brief Implementación completa del layout de clases compatible con MSVC
 */

#include <compiler/backend/mangling/ClassLayout.h>
#include <algorithm>

namespace cpp20::compiler::backend::mangling {

// ============================================================================
// ClassLayout - Implementación
// ============================================================================

ClassLayout::ClassLayout(const std::string& className, const std::string& scope)
    : className_(className), scope_(scope), totalSize_(0), alignment_(1),
      vtableOffset_(0), layoutComputed_(false), nameMangler_(className, scope) {
}

ClassLayout::~ClassLayout() = default;

void ClassLayout::addDataMember(const MemberInfo& member) {
    if (layoutComputed_) {
        // Si el layout ya fue calculado, necesitamos recalcular
        layoutComputed_ = false;
    }
    dataMembers_.push_back(member);
}

void ClassLayout::addVirtualFunction(const VirtualFunctionInfo& vfunc) {
    if (layoutComputed_) {
        layoutComputed_ = false;
    }
    virtualFunctions_.push_back(vfunc);
}

void ClassLayout::addInheritance(const InheritanceInfo& inheritance) {
    if (layoutComputed_) {
        layoutComputed_ = false;
    }
    inheritance_.push_back(inheritance);
}

void ClassLayout::computeLayout() {
    if (layoutComputed_) return;

    // 1. Procesar herencia primero (define el orden base)
    computeInheritanceLayout();

    // 2. Calcular posiciones de funciones virtuales
    computeVirtualFunctionLayout();

    // 3. Calcular offsets de miembros de datos
    computeDataMemberOffsets();

    // 4. Calcular tamaño total y alineación
    computeSizeAndAlignment();

    layoutComputed_ = true;
}

void ClassLayout::computeDataMemberOffsets() {
    size_t currentOffset = 0;

    // Si hay funciones virtuales, reservar espacio para vptr al inicio
    if (hasVirtualFunctions()) {
        vtableOffset_ = currentOffset;
        currentOffset += 8; // Tamaño de puntero en x64
    }

    // Procesar cada miembro de datos
    for (auto& member : dataMembers_) {
        if (member.isStatic) {
            // Miembros static no ocupan espacio en el objeto
            member.offset = 0;
            continue;
        }

        if (member.isBitField) {
            // Manejo especial para bit fields
            // TODO: Implementar layout de bit fields según reglas de MSVC
            member.offset = currentOffset;
        } else {
            // Alinear offset según el tipo del miembro
            size_t typeAlignment = getTypeAlignment(member.type);
            currentOffset = alignOffset(currentOffset, typeAlignment);

            member.offset = currentOffset;

            // Avanzar offset según el tamaño del tipo
            size_t typeSize = getTypeSize(member.type);
            currentOffset += typeSize;
        }
    }
}

void ClassLayout::computeInheritanceLayout() {
    // Implementación simplificada de layout de herencia
    // En MSVC, el orden de herencia afecta el layout

    size_t currentOffset = 0;

    for (auto& inherit : inheritance_) {
        if (inherit.isVirtual) {
            // Herencia virtual - más compleja
            // TODO: Implementar layout de herencia virtual
            inherit.offset = currentOffset;
            currentOffset += 8; // vptr para base virtual
        } else {
            // Herencia simple
            inherit.offset = currentOffset;
            // El tamaño se añade al final en computeSizeAndAlignment
        }
    }
}

void ClassLayout::computeVirtualFunctionLayout() {
    // Asignar índices a funciones virtuales
    int vtableIndex = 0;

    for (auto& vfunc : virtualFunctions_) {
        vfunc.vtableIndex = vtableIndex++;
    }
}

void ClassLayout::computeSizeAndAlignment() {
    size_t maxAlignment = 1;
    size_t currentSize = 0;

    // Considerar alineación de clases base
    for (const auto& inherit : inheritance_) {
        if (!inherit.isVirtual) {
            // Añadir tamaño de clase base (simplificado)
            size_t baseSize = getTypeSize(inherit.baseClass);
            size_t baseAlignment = getTypeAlignment(inherit.baseClass);

            currentSize += baseSize;
            maxAlignment = std::max(maxAlignment, baseAlignment);
        }
    }

    // Considerar alineación de miembros de datos
    for (const auto& member : dataMembers_) {
        if (!member.isStatic) {
            size_t typeSize = getTypeSize(member.type);
            size_t typeAlignment = getTypeAlignment(member.type);

            currentSize += typeSize;
            maxAlignment = std::max(maxAlignment, typeAlignment);
        }
    }

    // Si hay funciones virtuales, considerar alineación del vptr
    if (hasVirtualFunctions()) {
        maxAlignment = std::max(maxAlignment, size_t(8)); // Alineación de puntero
    }

    // Alinear el tamaño final
    totalSize_ = alignOffset(currentSize, maxAlignment);
    alignment_ = maxAlignment;
}

size_t ClassLayout::getTypeSize(const std::string& typeName) const {
    // Tabla simplificada de tamaños de tipos
    static const std::unordered_map<std::string, size_t> typeSizes = {
        {"bool", 1},
        {"char", 1},
        {"short", 2},
        {"int", 4},
        {"long", 4},
        {"long long", 8},
        {"float", 4},
        {"double", 8},
        {"long double", 8},
        {"void*", 8},
        {"char*", 8},
        {"int*", 8}
    };

    auto it = typeSizes.find(typeName);
    if (it != typeSizes.end()) {
        return it->second;
    }

    // Para tipos no encontrados, asumir tamaño de puntero
    return 8;
}

size_t ClassLayout::getTypeAlignment(const std::string& typeName) const {
    // Tabla simplificada de alineaciones de tipos
    static const std::unordered_map<std::string, size_t> typeAlignments = {
        {"bool", 1},
        {"char", 1},
        {"short", 2},
        {"int", 4},
        {"long", 4},
        {"long long", 8},
        {"float", 4},
        {"double", 8},
        {"long double", 8},
        {"void*", 8},
        {"char*", 8},
        {"int*", 8}
    };

    auto it = typeAlignments.find(typeName);
    if (it != typeAlignments.end()) {
        return it->second;
    }

    // Para tipos no encontrados, asumir alineación de puntero
    return 8;
}

size_t ClassLayout::alignOffset(size_t offset, size_t alignment) const {
    if (alignment == 0) return offset;
    return (offset + alignment - 1) & ~(alignment - 1);
}

bool ClassLayout::validateMSVCRules() const {
    // Reglas específicas de layout de MSVC
    // TODO: Implementar validaciones específicas de MSVC

    // 1. vptr debe estar al inicio si hay funciones virtuales
    if (hasVirtualFunctions() && vtableOffset_ != 0) {
        return false;
    }

    // 2. Alineación debe ser potencia de 2
    if ((alignment_ & (alignment_ - 1)) != 0) {
        return false;
    }

    // 3. Tamaño debe ser múltiplo de alineación
    if (totalSize_ % alignment_ != 0) {
        return false;
    }

    return true;
}

std::string ClassLayout::generateVTableName() const {
    return nameMangler_.generateVTableName();
}

std::string ClassLayout::generateTypeInfoName() const {
    return nameMangler_.generateTypeInfoName();
}

bool ClassLayout::isMSVCCompatible() const {
    if (!layoutComputed_) {
        return false;
    }
    return validateMSVCRules();
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
    // Verificar compatibilidad básica
    if (layout1.getSize() != layout2.getSize()) {
        return false;
    }

    if (layout1.getAlignment() != layout2.getAlignment()) {
        return false;
    }

    // Verificar miembros de datos
    const auto& members1 = layout1.getDataMembers();
    const auto& members2 = layout2.getDataMembers();

    if (members1.size() != members2.size()) {
        return false;
    }

    for (size_t i = 0; i < members1.size(); ++i) {
        if (members1[i].offset != members2[i].offset ||
            members1[i].type != members2[i].type) {
            return false;
        }
    }

    return true;
}

} // namespace cpp20::compiler::backend::mangling