/**
 * @file MSVCNameMangler.cpp
 * @brief Implementación del name mangling MSVC
 */

#include <compiler/backend/mangling/MSVCNameMangler.h>
#include <algorithm>
#include <sstream>
#include <cassert>

namespace cpp20::compiler::backend::mangling {

// ============================================================================
// Constantes de tipos básicos MSVC
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
    if (funcInfo.isExternC) {
        return funcInfo.name; // extern "C" functions are not mangled
    }

    std::string result = "?";

    // Añadir scope si existe
    if (!funcInfo.scope.empty()) {
        result += mangleScope(funcInfo.scope) + "@";
    }

    // Nombre base
    result += mangleBaseName(funcInfo.name);

    // Información de template si aplica
    if (funcInfo.templateArgs > 0) {
        result += "@@";
        // Template parameters would be encoded here
    }

    // Parámetros
    result += mangleParameterList(funcInfo.parameterTypes);

    // Calificadores
    if (funcInfo.qualifiers != FunctionQualifiers::None) {
        result += mangleQualifiers(funcInfo.qualifiers);
    }

    // Sufijo de función
    result += generateFunctionSuffix(funcInfo);

    return result;
}

std::string MSVCNameMangler::mangleVariable(const VariableInfo& varInfo) {
    if (varInfo.isExternC) {
        return varInfo.name; // extern "C" variables are not mangled
    }

    std::string result = "?";

    // Scope
    if (!varInfo.scope.empty()) {
        result += mangleScope(varInfo.scope) + "@";
    }

    // Nombre base
    result += mangleBaseName(varInfo.name);

    // Sufijo para variables
    result += "@@3";

    // Tipo (simplificado)
    result += mangleType(varInfo.type);

    return result;
}

std::string MSVCNameMangler::mangleClass(const ClassInfo& classInfo) const {
    std::string result = "?";

    // Scope
    if (!classInfo.scope.empty()) {
        result += mangleScope(classInfo.scope) + "@";
    }

    // Nombre de clase
    result += mangleBaseName(classInfo.name);

    // Información de template
    if (classInfo.templateArgs > 0) {
        result += "@@";
    }

    // Sufijo de clase
    result += "@@";

    return result;
}

std::string MSVCNameMangler::mangleType(const std::string& typeName) {
    // Tipos básicos
    if (typeName == "void") return VOID_CODE;
    if (typeName == "bool") return BOOL_CODE;
    if (typeName == "char") return CHAR_CODE;
    if (typeName == "unsigned char") return UCHAR_CODE;
    if (typeName == "short") return SHORT_CODE;
    if (typeName == "unsigned short") return USHORT_CODE;
    if (typeName == "int") return INT_CODE;
    if (typeName == "unsigned int") return UINT_CODE;
    if (typeName == "long") return LONG_CODE;
    if (typeName == "unsigned long") return ULONG_CODE;
    if (typeName == "long long") return LONGLONG_CODE;
    if (typeName == "unsigned long long") return ULONGLONG_CODE;
    if (typeName == "float") return FLOAT_CODE;
    if (typeName == "double") return DOUBLE_CODE;
    if (typeName == "long double") return LONGDOUBLE_CODE;

    // Tipos compuestos - simplificado
    if (typeName.find("const ") == 0) {
        return "B" + mangleType(typeName.substr(6));
    }
    if (typeName.find("volatile ") == 0) {
        return "C" + mangleType(typeName.substr(9));
    }

    // Para tipos complejos, usar representación simplificada
    return "V" + mangleBaseName(typeName);
}

std::string MSVCNameMangler::manglePointerType(const std::string& pointeeType) {
    std::string mangledPointee = mangleType(pointeeType);
    return "P" + mangledPointee; // Pointer
}

std::string MSVCNameMangler::mangleReferenceType(const std::string& refereeType) {
    std::string mangledReferee = mangleType(refereeType);
    return "A" + mangledReferee; // Reference
}

std::string MSVCNameMangler::mangleArrayType(const std::string& elementType, size_t size) {
    std::string mangledElement = mangleType(elementType);
    if (size > 0) {
        return "Y" + std::to_string(size) + mangledElement;
    } else {
        return "Q" + mangledElement; // Array desconocido
    }
}

std::string MSVCNameMangler::mangleFunctionType(const std::string& returnType,
                                               const std::vector<std::string>& paramTypes) {
    std::string result = "$$A6";
    result += mangleType(returnType);
    result += mangleParameterList(paramTypes);
    return result;
}

std::string MSVCNameMangler::generateFunctionPrefix(const FunctionInfo& funcInfo) {
    std::string prefix = "?";

    if (funcInfo.isStatic) {
        prefix += "S"; // Static member function
    } else if (funcInfo.isVirtual) {
        prefix += "U"; // Virtual member function
    } else {
        prefix += "Y"; // Non-virtual member function
    }

    return prefix;
}

std::string MSVCNameMangler::generateFunctionSuffix(const FunctionInfo& funcInfo) {
    std::string suffix = "Z"; // End of function

    if (funcInfo.isVirtual) {
        suffix = "@Z"; // Virtual function end
    }

    return suffix;
}

std::string MSVCNameMangler::mangleBaseName(const std::string& name) const {
    if (name.empty()) return "";

    // Longitud del nombre
    std::string lengthCode = encodeLength(name.length());
    std::string escapedName = escapeSpecialChars(name);

    return lengthCode + escapedName;
}

std::string MSVCNameMangler::mangleScope(const std::string& scope) const {
    if (scope.empty()) return "";

    // Parse namespace/class hierarchy
    std::vector<std::string> parts;
    std::stringstream ss(scope);
    std::string part;

    while (std::getline(ss, part, ':')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }

    std::string result;
    for (const auto& p : parts) {
        result += mangleBaseName(p) + "@";
    }

    return result;
}

std::string MSVCNameMangler::mangleQualifiers(FunctionQualifiers qualifiers) {
    switch (qualifiers) {
        case FunctionQualifiers::Const: return "B";
        case FunctionQualifiers::Volatile: return "C";
        case FunctionQualifiers::ConstVolatile: return "D";
        case FunctionQualifiers::Restrict: return "E";
        case FunctionQualifiers::ConstRestrict: return "F";
        case FunctionQualifiers::VolatileRestrict: return "G";
        case FunctionQualifiers::ConstVolatileRestrict: return "H";
        default: return "";
    }
}

std::string MSVCNameMangler::mangleParameterList(const std::vector<std::string>& paramTypes) {
    if (paramTypes.empty()) {
        return "X"; // void parameter list
    }

    std::string result;
    for (const auto& paramType : paramTypes) {
        result += mangleType(paramType);
    }

    result += "@"; // End of parameter list
    return result;
}

std::string MSVCNameMangler::encodeLength(size_t length) const {
    if (length < 10) {
        return std::to_string(length);
    } else {
        // Para longitudes mayores, MSVC usa un esquema especial
        return "@" + std::to_string(length) + "@";
    }
}

bool MSVCNameMangler::isValidMangledChar(char c) const {
    return (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '_' || c == '@' || c == '$';
}

std::string MSVCNameMangler::escapeSpecialChars(const std::string& str) const {
    std::string result;
    for (char c : str) {
        if (isValidMangledChar(c)) {
            result += c;
        } else {
            // Escape caracteres especiales (simplificado)
            result += "@" + std::to_string(static_cast<int>(c)) + "@";
        }
    }
    return result;
}

// ============================================================================
// MangledNameUtils - Implementación
// ============================================================================

std::string MangledNameUtils::demangle(const std::string& mangled) {
    if (!isMangled(mangled)) {
        return mangled; // Not mangled, return as-is
    }

    // Simplified demangling - in a real implementation this would be much more complex
    std::string result;

    // Remove leading '?'
    if (mangled[0] == '?') {
        result = mangled.substr(1);
    }

    // Find function name
    size_t atPos = result.find('@');
    if (atPos != std::string::npos) {
        // Extract base name (simplified)
        std::string baseName = result.substr(0, atPos);
        if (!baseName.empty() && std::isdigit(baseName[0])) {
            // Remove length prefix
            size_t length = std::stoi(std::string(1, baseName[0]));
            if (baseName.length() > 1) {
                baseName = baseName.substr(1, length);
            }
        }
        result = baseName;
    }

    return result;
}

bool MangledNameUtils::isMangled(const std::string& name) {
    return !name.empty() && name[0] == '?' && name.length() > 1;
}

std::string MangledNameUtils::extractBaseName(const std::string& mangled) {
    if (!isMangled(mangled)) {
        return mangled;
    }

    // Simplified extraction
    std::string result = demangle(mangled);
    return result;
}

bool MangledNameUtils::namesEqual(const std::string& name1, const std::string& name2) {
    // Compare either both mangled or both demangled
    if (isMangled(name1) && isMangled(name2)) {
        return name1 == name2;
    } else if (!isMangled(name1) && !isMangled(name2)) {
        return name1 == name2;
    } else {
        // Mixed - demangle one and compare
        std::string demangled1 = isMangled(name1) ? demangle(name1) : name1;
        std::string demangled2 = isMangled(name2) ? demangle(name2) : name2;
        return demangled1 == demangled2;
    }
}

} // namespace cpp20::compiler::backend::mangling
