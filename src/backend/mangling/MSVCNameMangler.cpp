/**
 * @file MSVCNameMangler.cpp
 * @brief Implementación completa del name mangling compatible con MSVC
 */

#include <compiler/backend/mangling/MSVCNameMangler.h>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <iomanip>

namespace cpp20::compiler::backend::mangling {

// ============================================================================
// CONSTANTES ESTÁTICAS
// ============================================================================

const std::string MSVCNameMangler::VOID_CODE = "X";
const std::string MSVCNameMangler::BOOL_CODE = "_N";
const std::string MSVCNameMangler::CHAR_CODE = "D";
const std::string MSVCNameMangler::UCHAR_CODE = "E";
const std::string MSVCNameMangler::SHORT_CODE = "F";
const std::string MSVCNameMangler::USHORT_CODE = "G";
const std::string MSVCNameMangler::INT_CODE = "H";
const std::string MSVCNameMangler::UINT_CODE = "I";
const std::string MSVCNameMangler::LONG_CODE = "J";
const std::string MSVCNameMangler::ULONG_CODE = "K";
const std::string MSVCNameMangler::LONGLONG_CODE = "_J";
const std::string MSVCNameMangler::ULONGLONG_CODE = "_K";
const std::string MSVCNameMangler::FLOAT_CODE = "M";
const std::string MSVCNameMangler::DOUBLE_CODE = "N";
const std::string MSVCNameMangler::LONGDOUBLE_CODE = "O";

// ============================================================================
// MSVCNameMangler - Implementación
// ============================================================================

MSVCNameMangler::MSVCNameMangler() = default;
MSVCNameMangler::~MSVCNameMangler() = default;

std::string MSVCNameMangler::mangleFunction(const FunctionInfo& funcInfo) {
    std::stringstream result;

    // Prefijo para funciones
    result << "?";

    // Nombre base
    result << mangleBaseName(funcInfo.name);

    // Scope/namespace si existe
    if (!funcInfo.scope.empty()) {
        result << "@" << mangleScope(funcInfo.scope);
    }

    // Sufijo con información de tipos
    result << generateFunctionSuffix(funcInfo);

    return result.str();
}

std::string MSVCNameMangler::mangleVariable(const VariableInfo& varInfo) {
    std::stringstream result;

    // Prefijo para variables
    result << "?";

    // Nombre base
    result << mangleBaseName(varInfo.name);

    // Scope si existe
    if (!varInfo.scope.empty()) {
        result << "@" << mangleScope(varInfo.scope);
    }

    // Tipo
    result << "@@" << mangleType(varInfo.type);

    return result.str();
}

std::string MSVCNameMangler::mangleClass(const ClassInfo& classInfo) const {
    std::stringstream result;

    // Prefijo para clases
    result << "?";

    // Nombre base
    result << mangleBaseName(classInfo.name);

    // Scope si existe
    if (!classInfo.scope.empty()) {
        result << "@" << mangleScope(classInfo.scope);
    }

    // Sufijo
    result << "@@";

    return result.str();
}

std::string MSVCNameMangler::mangleType(const std::string& typeName) {
    // Mapa de tipos básicos
    static const std::unordered_map<std::string, std::string> basicTypes = {
        {"void", VOID_CODE},
        {"bool", BOOL_CODE},
        {"char", CHAR_CODE},
        {"unsigned char", UCHAR_CODE},
        {"short", SHORT_CODE},
        {"unsigned short", USHORT_CODE},
        {"int", INT_CODE},
        {"unsigned int", UINT_CODE},
        {"long", LONG_CODE},
        {"unsigned long", ULONG_CODE},
        {"long long", LONGLONG_CODE},
        {"unsigned long long", ULONGLONG_CODE},
        {"float", FLOAT_CODE},
        {"double", DOUBLE_CODE},
        {"long double", LONGDOUBLE_CODE}
    };

    auto it = basicTypes.find(typeName);
    if (it != basicTypes.end()) {
        return it->second;
    }

    // Tipos compuestos - simplificados para esta implementación
    if (typeName.find("*") != std::string::npos) {
        return manglePointerType(typeName.substr(0, typeName.find("*")));
    }

    if (typeName.find("&") != std::string::npos) {
        return mangleReferenceType(typeName.substr(0, typeName.find("&")));
    }

    // Para tipos no reconocidos, usar código genérico
    return "V"; // Tipo desconocido
}

std::string MSVCNameMangler::manglePointerType(const std::string& pointeeType) {
    std::stringstream result;

    // Código para puntero
    result << "P";

    // Tipo apuntado
    result << mangleType(pointeeType);

    return result.str();
}

std::string MSVCNameMangler::mangleReferenceType(const std::string& refereeType) {
    std::stringstream result;

    // Código para referencia (l-value reference)
    result << "A";

    // Tipo referenciado
    result << mangleType(refereeType);

    return result.str();
}

std::string MSVCNameMangler::mangleArrayType(const std::string& elementType, size_t size) {
    std::stringstream result;

    // Código para array
    result << "Y";

    // Tamaño del array
    if (size > 0) {
        result << encodeLength(size);
    }

    // Tipo de elementos
    result << mangleType(elementType);

    return result.str();
}

std::string MSVCNameMangler::mangleFunctionType(const std::string& returnType,
                                               const std::vector<std::string>& paramTypes) {
    std::stringstream result;

    // Código para tipo función
    result << "$$A6";

    // Tipo de retorno
    result << mangleType(returnType);

    // Parámetros
    result << mangleParameterList(paramTypes);

    return result.str();
}

std::string MSVCNameMangler::generateFunctionPrefix(const FunctionInfo& funcInfo) {
    std::stringstream result;

    // Prefijo específico según tipo de función
    if (funcInfo.isVirtual) {
        result << "?";
    } else if (funcInfo.isStatic) {
        result << "?";
    } else {
        result << "?";
    }

    return result.str();
}

std::string MSVCNameMangler::generateFunctionSuffix(const FunctionInfo& funcInfo) {
    std::stringstream result;

    // Separador
    result << "@@";

    // Calificadores
    if (funcInfo.qualifiers != FunctionQualifiers::None) {
        result << mangleQualifiers(funcInfo.qualifiers);
    }

    // Tipo de retorno
    result << mangleType(funcInfo.returnType);

    // Lista de parámetros
    result << mangleParameterList(funcInfo.parameterTypes);

    // Información adicional para funciones virtuales
    if (funcInfo.isVirtual) {
        result << "Z"; // Indicador de función virtual
    }

    return result.str();
}

std::string MSVCNameMangler::mangleBaseName(const std::string& name) const {
    std::stringstream result;

    // Codificar longitud del nombre
    result << encodeLength(name.length());

    // Escapar caracteres especiales
    result << escapeSpecialChars(name);

    return result.str();
}

std::string MSVCNameMangler::mangleScope(const std::string& scope) const {
    std::stringstream result;

    // Dividir scope por ::
    size_t pos = 0;
    size_t found;
    std::string remaining = scope;

    while ((found = remaining.find("::", pos)) != std::string::npos) {
        std::string part = remaining.substr(pos, found - pos);
        result << mangleBaseName(part) << "@";
        pos = found + 2;
    }

    // Última parte
    if (pos < remaining.length()) {
        result << mangleBaseName(remaining.substr(pos));
    }

    return result.str();
}

std::string MSVCNameMangler::mangleQualifiers(FunctionQualifiers qualifiers) {
    switch (qualifiers) {
        case FunctionQualifiers::None:
            return "";
        case FunctionQualifiers::Const:
            return "B";
        case FunctionQualifiers::Volatile:
            return "C";
        case FunctionQualifiers::ConstVolatile:
            return "D";
        case FunctionQualifiers::Restrict:
            return "I";
        case FunctionQualifiers::ConstRestrict:
            return "J";
        case FunctionQualifiers::VolatileRestrict:
            return "K";
        case FunctionQualifiers::ConstVolatileRestrict:
            return "L";
        default:
            return "";
    }
}

std::string MSVCNameMangler::mangleParameterList(const std::vector<std::string>& paramTypes) {
    std::stringstream result;

    if (paramTypes.empty()) {
        result << VOID_CODE; // Sin parámetros
    } else {
        for (const auto& paramType : paramTypes) {
            result << mangleType(paramType);
        }
        result << "@"; // Terminador
    }

    return result.str();
}

std::string MSVCNameMangler::encodeLength(size_t length) const {
    if (length < 10) {
        return std::to_string(length);
    } else {
        // Para longitudes mayores, usar codificación especial
        std::stringstream result;
        result << "@" << length << "@";
        return result.str();
    }
}

bool MSVCNameMangler::isValidMangledChar(char c) const {
    return (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '_' || c == '@' || c == '$';
}

std::string MSVCNameMangler::escapeSpecialChars(const std::string& str) const {
    std::stringstream result;

    for (char c : str) {
        if (isValidMangledChar(c)) {
            result << c;
        } else {
            // Escapar caracteres especiales
            result << "?" << std::hex << std::setw(2) << std::setfill('0')
                   << static_cast<int>(static_cast<unsigned char>(c));
        }
    }

    return result.str();
}

// ============================================================================
// MangledNameUtils - Implementación
// ============================================================================

std::string MangledNameUtils::demangle(const std::string& mangled) {
    // Implementación simplificada de demangling
    // En un compilador real, esto sería mucho más complejo

    if (mangled.empty() || mangled[0] != '?') {
        return mangled; // No está mangled
    }

    std::stringstream result;
    size_t pos = 1; // Saltar el '?'

    // Extraer nombre base
    if (pos < mangled.length() && isdigit(mangled[pos])) {
        int length = mangled[pos] - '0';
        pos++;
        if (pos + length <= mangled.length()) {
            result << mangled.substr(pos, length);
            pos += length;
        }
    }

    // Buscar scope
    if (pos < mangled.length() && mangled[pos] == '@') {
        pos++; // Saltar '@'
        result << "::";

        // Extraer componentes del scope
        while (pos < mangled.length() && mangled[pos] != '@') {
            if (isdigit(mangled[pos])) {
                int length = mangled[pos] - '0';
                pos++;
                if (pos + length <= mangled.length()) {
                    result << mangled.substr(pos, length);
                    pos += length;
                }
            }

            if (pos < mangled.length() && mangled[pos] == '@') {
                result << "::";
                pos++;
            }
        }
    }

    return result.str();
}

bool MangledNameUtils::isMangled(const std::string& name) {
    return !name.empty() && name[0] == '?' && name.length() > 1;
}

std::string MangledNameUtils::extractBaseName(const std::string& mangled) {
    if (!isMangled(mangled)) {
        return mangled;
    }

    // Buscar el primer '@' después del nombre base
    size_t start = 1;
    if (start < mangled.length() && isdigit(mangled[start])) {
        int length = mangled[start] - '0';
        start++;
        if (start + length <= mangled.length()) {
            return mangled.substr(start, length);
        }
    }

    return mangled;
}

bool MangledNameUtils::namesEqual(const std::string& name1, const std::string& name2) {
    // Comparación simplificada - en un compilador real esto sería más complejo
    return name1 == name2;
}

// ============================================================================
// Métodos adicionales para vtable y type_info
// ============================================================================

std::string MSVCNameMangler::generateVTableName(const std::string& className,
                                               const std::string& scope) const {
    // Formato MSVC para vtable: ??_7ClassName@@6B@
    // ??_7 indica vtable, @@6B@ es el sufijo estándar
    std::string mangledScope = mangleScope(scope);
    std::string mangledName = mangleBaseName(className);

    return "??_7" + mangledName + mangledScope + "@@6B@";
}

std::string MSVCNameMangler::generateTypeInfoName(const std::string& className,
                                                 const std::string& scope) const {
    // Formato MSVC para type_info: ??_R0?AVClassName@@@8
    // ??_R0 indica type_info, ?AV indica class virtual
    std::string mangledScope = mangleScope(scope);
    std::string mangledName = mangleBaseName(className);

    return "??_R0?AV" + mangledName + mangledScope + "@@@8";
}

std::string MSVCNameMangler::mangleName(const std::string& name) const {
    return mangleBaseName(name);
}

} // namespace cpp20::compiler::backend::mangling