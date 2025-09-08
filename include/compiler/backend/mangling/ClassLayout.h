/**
 * @file ClassLayout.h
 * @brief Layout de clases compatible con MSVC
 */

#pragma once

#include <compiler/backend/mangling/MSVCNameMangler.h>
#include <vector>
#include <string>
#include <memory>

namespace cpp20::compiler::backend::mangling {

/**
 * @brief Información de un miembro de clase
 */
struct MemberInfo {
    std::string name;           // Nombre del miembro
    std::string type;           // Tipo del miembro
    size_t offset;              // Offset desde el inicio de la clase
    bool isVirtual;             // Es función virtual
    bool isStatic;              // Es miembro static
    bool isBitField;            // Es bit field
    int bitOffset;              // Offset de bit (para bit fields)
    int bitWidth;               // Ancho de bit (para bit fields)

    MemberInfo(const std::string& n, const std::string& t, size_t off = 0)
        : name(n), type(t), offset(off), isVirtual(false), isStatic(false),
          isBitField(false), bitOffset(0), bitWidth(0) {}
};

/**
 * @brief Información de función virtual
 */
struct VirtualFunctionInfo {
    std::string name;           // Nombre de la función
    std::string signature;      // Firma completa
    int vtableIndex;            // Índice en vtable
    bool isPureVirtual;         // Es pura virtual
    bool isOverride;            // Es override

    VirtualFunctionInfo(const std::string& n, const std::string& sig, int idx)
        : name(n), signature(sig), vtableIndex(idx), isPureVirtual(false), isOverride(false) {}
};

/**
 * @brief Información de herencia
 */
struct InheritanceInfo {
    std::string baseClass;      // Nombre de la clase base
    size_t offset;              // Offset de la subobjeto base
    bool isVirtual;             // Es herencia virtual
    bool isPrimary;             // Es base primaria

    InheritanceInfo(const std::string& base, size_t off, bool virt = false, bool primary = true)
        : baseClass(base), offset(off), isVirtual(virt), isPrimary(primary) {}
};

/**
 * @brief Layout completo de una clase compatible con MSVC
 */
class ClassLayout {
public:
    /**
     * @brief Constructor
     */
    ClassLayout(const std::string& className, const std::string& scope = "");

    /**
     * @brief Destructor
     */
    ~ClassLayout();

    /**
     * @brief Añade un miembro de datos
     */
    void addDataMember(const MemberInfo& member);

    /**
     * @brief Añade una función virtual
     */
    void addVirtualFunction(const VirtualFunctionInfo& vfunc);

    /**
     * @brief Añade información de herencia
     */
    void addInheritance(const InheritanceInfo& inheritance);

    /**
     * @brief Calcula el layout completo
     */
    void computeLayout();

    /**
     * @brief Obtiene el tamaño total de la clase
     */
    size_t getSize() const { return totalSize_; }

    /**
     * @brief Obtiene la alineación de la clase
     */
    size_t getAlignment() const { return alignment_; }

    /**
     * @brief Obtiene el offset del puntero vtable
     */
    size_t getVTableOffset() const { return vtableOffset_; }

    /**
     * @brief Verifica si la clase tiene funciones virtuales
     */
    bool hasVirtualFunctions() const { return !virtualFunctions_.empty(); }

    /**
     * @brief Obtiene lista de miembros de datos
     */
    const std::vector<MemberInfo>& getDataMembers() const { return dataMembers_; }

    /**
     * @brief Obtiene lista de funciones virtuales
     */
    const std::vector<VirtualFunctionInfo>& getVirtualFunctions() const { return virtualFunctions_; }

    /**
     * @brief Obtiene lista de herencias
     */
    const std::vector<InheritanceInfo>& getInheritance() const { return inheritance_; }

    /**
     * @brief Genera el nombre mangled de la vtable
     */
    std::string generateVTableName() const;

    /**
     * @brief Genera el nombre mangled del type_info
     */
    std::string generateTypeInfoName() const;

    /**
     * @brief Obtiene el nombre de la clase
     */
    std::string getClassName() const { return className_; }

    /**
     * @brief Verifica compatibilidad con MSVC
     */
    bool isMSVCCompatible() const;

private:
    std::string className_;                 // Nombre de la clase
    std::string scope_;                     // Ámbito
    std::vector<MemberInfo> dataMembers_;   // Miembros de datos
    std::vector<VirtualFunctionInfo> virtualFunctions_; // Funciones virtuales
    std::vector<InheritanceInfo> inheritance_; // Información de herencia

    size_t totalSize_;                      // Tamaño total calculado
    size_t alignment_;                      // Alineación calculada
    size_t vtableOffset_;                   // Offset del puntero vtable
    bool layoutComputed_;                   // Si el layout ya fue calculado

    MSVCNameMangler nameMangler_;           // Mangler para nombres

    /**
     * @brief Calcula offsets para miembros de datos
     */
    void computeDataMemberOffsets();

    /**
     * @brief Calcula layout para herencia
     */
    void computeInheritanceLayout();

    /**
     * @brief Calcula posiciones de funciones virtuales
     */
    void computeVirtualFunctionLayout();

    /**
     * @brief Calcula el tamaño total y alineación
     */
    void computeSizeAndAlignment();

    /**
     * @brief Obtiene el tamaño de un tipo
     */
    size_t getTypeSize(const std::string& typeName) const;

    /**
     * @brief Obtiene la alineación de un tipo
     */
    size_t getTypeAlignment(const std::string& typeName) const;

    /**
     * @brief Alinea un offset al siguiente límite
     */
    size_t alignOffset(size_t offset, size_t alignment) const;

    /**
     * @brief Verifica reglas de layout de MSVC
     */
    bool validateMSVCRules() const;
};

/**
 * @brief Generador de layouts de clase
 */
class ClassLayoutGenerator {
public:
    /**
     * @brief Crea layout para una clase simple
     */
    static std::unique_ptr<ClassLayout> createSimpleClass(
        const std::string& className,
        const std::vector<MemberInfo>& members);

    /**
     * @brief Crea layout para clase con herencia
     */
    static std::unique_ptr<ClassLayout> createInheritedClass(
        const std::string& className,
        const std::vector<InheritanceInfo>& bases,
        const std::vector<MemberInfo>& members);

    /**
     * @brief Crea layout para clase con funciones virtuales
     */
    static std::unique_ptr<ClassLayout> createPolymorphicClass(
        const std::string& className,
        const std::vector<MemberInfo>& members,
        const std::vector<VirtualFunctionInfo>& virtualFuncs);

    /**
     * @brief Valida que un layout sea compatible con MSVC
     */
    static bool validateLayout(const ClassLayout& layout);

    /**
     * @brief Compara dos layouts para compatibilidad
     */
    static bool layoutsCompatible(const ClassLayout& layout1, const ClassLayout& layout2);
};

} // namespace cpp20::compiler::backend::mangling
