/**
 * @file MSVCNameMangler.h
 * @brief Name mangling compatible con Microsoft Visual C++
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

namespace cpp20::compiler::backend::mangling {

/**
 * @brief Tipos de entidades que pueden ser mangled
 */
enum class EntityType {
    Function,           // Función libre
    MemberFunction,     // Función miembro
    StaticFunction,     // Función miembro static
    Constructor,        // Constructor
    Destructor,         // Destructor
    Variable,           // Variable global
    StaticMember,       // Miembro static
    VirtualFunction,    // Función virtual
    Thunk,              // Thunk para virtual inheritance
    LocalStatic,        // Variable static local
    VTable,             // Tabla de funciones virtuales
    VTT,                // Virtual table table
    TypeInfo,           // Información de tipo
    GuardVariable       // Variable de guarda para inicialización
};

/**
 * @brief Calificadores de función
 */
enum class FunctionQualifiers {
    None = 0,
    Const = 1,
    Volatile = 2,
    ConstVolatile = 3,
    Restrict = 4,
    ConstRestrict = 5,
    VolatileRestrict = 6,
    ConstVolatileRestrict = 7
};

/**
 * @brief Información de una función para mangling
 */
struct FunctionInfo {
    std::string name;                           // Nombre base
    std::string scope;                          // Ámbito (namespace/clase)
    std::vector<std::string> parameterTypes;    // Tipos de parámetros
    std::string returnType;                     // Tipo de retorno
    FunctionQualifiers qualifiers;              // Calificadores
    bool isVirtual;                             // Es virtual
    bool isStatic;                              // Es static
    bool isExternC;                             // Es extern "C"
    int templateArgs;                           // Número de argumentos template

    FunctionInfo() : qualifiers(FunctionQualifiers::None), isVirtual(false),
                     isStatic(false), isExternC(false), templateArgs(0) {}
};

/**
 * @brief Información de una variable para mangling
 */
struct VariableInfo {
    std::string name;           // Nombre base
    std::string scope;          // Ámbito
    std::string type;           // Tipo
    bool isStatic;              // Es static
    bool isExternC;             // Es extern "C"
};

/**
 * @brief Información de una clase para mangling
 */
struct ClassInfo {
    std::string name;           // Nombre de la clase
    std::string scope;          // Ámbito contenedor
    bool isStruct;              // Es struct en lugar de class
    bool hasVirtualFunctions;   // Tiene funciones virtuales
    int templateArgs;           // Número de argumentos template
};

/**
 * @brief Mangler de nombres compatible con MSVC
 *
 * Implementa el algoritmo completo de name decoration de Microsoft Visual C++.
 * Soporta funciones, variables, clases y templates.
 */
class MSVCNameMangler {
public:
    /**
     * @brief Constructor
     */
    MSVCNameMangler();

    /**
     * @brief Destructor
     */
    ~MSVCNameMangler();

    /**
     * @brief Manglea el nombre de una función
     * @param funcInfo Información de la función
     * @return Nombre mangled
     */
    std::string mangleFunction(const FunctionInfo& funcInfo);

    /**
     * @brief Manglea el nombre de una variable
     * @param varInfo Información de la variable
     * @return Nombre mangled
     */
    std::string mangleVariable(const VariableInfo& varInfo);

    /**
     * @brief Manglea el nombre de una clase
     * @param classInfo Información de la clase
     * @return Nombre mangled
     */
    std::string mangleClass(const ClassInfo& classInfo) const;

    /**
     * @brief Manglea un tipo básico
     * @param typeName Nombre del tipo
     * @return Código de tipo mangled
     */
    std::string mangleType(const std::string& typeName);

    /**
     * @brief Manglea un tipo puntero
     * @param pointeeType Tipo apuntado
     * @return Código de tipo puntero mangled
     */
    std::string manglePointerType(const std::string& pointeeType);

    /**
     * @brief Manglea un tipo referencia
     * @param refereeType Tipo referenciado
     * @return Código de tipo referencia mangled
     */
    std::string mangleReferenceType(const std::string& refereeType);

    /**
     * @brief Manglea un tipo array
     * @param elementType Tipo de elementos
     * @param size Tamaño del array (0 = desconocido)
     * @return Código de tipo array mangled
     */
    std::string mangleArrayType(const std::string& elementType, size_t size = 0);

    /**
     * @brief Manglea un tipo función
     * @param returnType Tipo de retorno
     * @param paramTypes Tipos de parámetros
     * @return Código de tipo función mangled
     */
    std::string mangleFunctionType(const std::string& returnType,
                                   const std::vector<std::string>& paramTypes);

    /**
     * @brief Genera nombre mangled para vtable
     * @param className Nombre de la clase
     * @param scope Ámbito de la clase
     * @return Nombre mangled de la vtable
     */
    std::string generateVTableName(const std::string& className,
                                   const std::string& scope = "") const;

    /**
     * @brief Genera nombre mangled para type_info
     * @param className Nombre de la clase
     * @param scope Ámbito de la clase
     * @return Nombre mangled del type_info
     */
    std::string generateTypeInfoName(const std::string& className,
                                     const std::string& scope = "") const;

    /**
     * @brief Manglea un nombre base (método público)
     * @param name Nombre a manglear
     * @return Nombre mangled
     */
    std::string mangleName(const std::string& name) const;

private:
    /**
     * @brief Códigos base para tipos básicos MSVC
     */
    static const std::string VOID_CODE;
    static const std::string BOOL_CODE;
    static const std::string CHAR_CODE;
    static const std::string UCHAR_CODE;
    static const std::string SHORT_CODE;
    static const std::string USHORT_CODE;
    static const std::string INT_CODE;
    static const std::string UINT_CODE;
    static const std::string LONG_CODE;
    static const std::string ULONG_CODE;
    static const std::string LONGLONG_CODE;
    static const std::string ULONGLONG_CODE;
    static const std::string FLOAT_CODE;
    static const std::string DOUBLE_CODE;
    static const std::string LONGDOUBLE_CODE;

    /**
     * @brief Genera prefijo de función
     */
    std::string generateFunctionPrefix(const FunctionInfo& funcInfo);

    /**
     * @brief Genera sufijo de función
     */
    std::string generateFunctionSuffix(const FunctionInfo& funcInfo);

    /**
     * @brief Manglea nombre base
     */
    std::string mangleBaseName(const std::string& name) const;

    /**
     * @brief Manglea scope/namespace
     */
    std::string mangleScope(const std::string& scope) const;

    /**
     * @brief Manglea calificadores de función
     */
    std::string mangleQualifiers(FunctionQualifiers qualifiers);

    /**
     * @brief Manglea lista de parámetros
     */
    std::string mangleParameterList(const std::vector<std::string>& paramTypes);

    /**
     * @brief Codifica longitud para MSVC
     */
    std::string encodeLength(size_t length) const;

    /**
     * @brief Verifica si un caracter es válido en nombres mangled
     */
    bool isValidMangledChar(char c) const;

    /**
     * @brief Escapa caracteres especiales
     */
    std::string escapeSpecialChars(const std::string& str) const;
};

/**
 * @brief Utilidades para trabajar con nombres mangled
 */
class MangledNameUtils {
public:
    /**
     * @brief Desmanglea un nombre (simplificado)
     * @param mangled Nombre mangled
     * @return Nombre desmangled aproximado
     */
    static std::string demangle(const std::string& mangled);

    /**
     * @brief Verifica si un nombre está mangled
     */
    static bool isMangled(const std::string& name);

    /**
     * @brief Extrae el nombre base de un nombre mangled
     */
    static std::string extractBaseName(const std::string& mangled);

    /**
     * @brief Compara dos nombres mangled para ver si representan la misma entidad
     */
    static bool namesEqual(const std::string& name1, const std::string& name2);
};

} // namespace cpp20::compiler::backend::mangling
