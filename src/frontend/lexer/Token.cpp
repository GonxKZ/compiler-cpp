/**
 * @file Token.cpp
 * @brief Implementación de la clase Token
 */

#include <compiler/frontend/lexer/Token.h>
#include <unordered_map>

namespace cpp20::compiler::frontend::lexer {

// ============================================================================
// Token - Implementación
// ============================================================================

Token::Token(TokenType type, const std::string& lexeme,
             const diagnostics::SourceLocation& location,
             const std::string& value)
    : type_(type), lexeme_(lexeme), location_(location), value_(value) {
}

Token::~Token() = default;

bool Token::isKeyword() const {
    switch (type_) {
        // Tipos fundamentales
        case TokenType::VOID: case TokenType::INT: case TokenType::CHAR:
        case TokenType::SHORT: case TokenType::LONG: case TokenType::FLOAT:
        case TokenType::DOUBLE: case TokenType::BOOL:
        // Calificadores
        case TokenType::CONST: case TokenType::VOLATILE: case TokenType::CONSTEVAL:
        case TokenType::CONSTEXPR: case TokenType::CONSTINIT: case TokenType::MUTABLE:
        // Especificadores de almacenamiento
        case TokenType::STATIC: case TokenType::EXTERN: case TokenType::INLINE:
        case TokenType::THREAD_LOCAL:
        // Control de acceso
        case TokenType::PUBLIC: case TokenType::PRIVATE: case TokenType::PROTECTED:
        // Estructuras de control
        case TokenType::IF: case TokenType::ELSE: case TokenType::WHILE:
        case TokenType::FOR: case TokenType::DO: case TokenType::SWITCH:
        case TokenType::CASE: case TokenType::DEFAULT: case TokenType::BREAK:
        case TokenType::CONTINUE: case TokenType::RETURN: case TokenType::GOTO:
        // Estructuras de datos
        case TokenType::STRUCT: case TokenType::CLASS: case TokenType::UNION:
        case TokenType::ENUM:
        // Funciones y métodos
        case TokenType::VIRTUAL: case TokenType::OVERRIDE: case TokenType::FINAL:
        case TokenType::NOEXCEPT:
        // Excepciones
        case TokenType::TRY: case TokenType::CATCH: case TokenType::THROW:
        // Templates y conceptos
        case TokenType::TEMPLATE: case TokenType::TYPENAME: case TokenType::CONCEPT:
        case TokenType::REQUIRES:
        // Espacios de nombres
        case TokenType::NAMESPACE: case TokenType::USING:
        // Operadores y conversión
        case TokenType::OPERATOR: case TokenType::EXPLICIT:
        // Misceláneo
        case TokenType::SIZEOF: case TokenType::ALIGNOF: case TokenType::ALIGNAS:
        case TokenType::TYPEID: case TokenType::DECLTYPE: case TokenType::AUTO:
        // C++20 específico
        case TokenType::CO_AWAIT: case TokenType::CO_RETURN: case TokenType::CO_YIELD:
        case TokenType::MODULE: case TokenType::IMPORT: case TokenType::EXPORT:
        // Literales booleanos
        case TokenType::TRUE_LITERAL: case TokenType::FALSE_LITERAL:
        case TokenType::NULLPTR_LITERAL:
            return true;
        default:
            return false;
    }
}

bool Token::isOperator() const {
    switch (type_) {
        // Operadores aritméticos
        case TokenType::PLUS: case TokenType::MINUS: case TokenType::STAR:
        case TokenType::SLASH: case TokenType::PERCENT: case TokenType::INCREMENT:
        case TokenType::DECREMENT:
        // Operadores de comparación
        case TokenType::EQUAL: case TokenType::NOT_EQUAL: case TokenType::LESS:
        case TokenType::GREATER: case TokenType::LESS_EQUAL: case TokenType::GREATER_EQUAL:
        case TokenType::SPACESHIP:
        // Operadores lógicos
        case TokenType::LOGICAL_AND: case TokenType::LOGICAL_OR: case TokenType::LOGICAL_NOT:
        // Operadores bit a bit
        case TokenType::BIT_AND: case TokenType::BIT_OR: case TokenType::BIT_XOR:
        case TokenType::BIT_NOT: case TokenType::LEFT_SHIFT: case TokenType::RIGHT_SHIFT:
        // Operadores de asignación
        case TokenType::ASSIGN: case TokenType::PLUS_ASSIGN: case TokenType::MINUS_ASSIGN:
        case TokenType::MUL_ASSIGN: case TokenType::DIV_ASSIGN: case TokenType::MOD_ASSIGN:
        case TokenType::AND_ASSIGN: case TokenType::OR_ASSIGN: case TokenType::XOR_ASSIGN:
        case TokenType::LEFT_SHIFT_ASSIGN: case TokenType::RIGHT_SHIFT_ASSIGN:
        // Operadores misceláneos
        case TokenType::ARROW: case TokenType::ARROW_STAR: case TokenType::DOT:
        case TokenType::DOT_STAR:
            return true;
        default:
            return false;
    }
}

bool Token::isLiteral() const {
    switch (type_) {
        case TokenType::INTEGER_LITERAL: case TokenType::FLOAT_LITERAL:
        case TokenType::CHAR_LITERAL: case TokenType::STRING_LITERAL:
        case TokenType::TRUE_LITERAL: case TokenType::FALSE_LITERAL:
        case TokenType::NULLPTR_LITERAL:
        case TokenType::USER_DEFINED_INTEGER_LITERAL: case TokenType::USER_DEFINED_FLOAT_LITERAL:
        case TokenType::USER_DEFINED_CHAR_LITERAL: case TokenType::USER_DEFINED_STRING_LITERAL:
            return true;
        default:
            return false;
    }
}

bool Token::isPunctuation() const {
    switch (type_) {
        case TokenType::LEFT_PAREN: case TokenType::RIGHT_PAREN:
        case TokenType::LEFT_BRACKET: case TokenType::RIGHT_BRACKET:
        case TokenType::LEFT_BRACE: case TokenType::RIGHT_BRACE:
        case TokenType::SEMICOLON: case TokenType::COMMA: case TokenType::COLON:
        case TokenType::QUESTION: case TokenType::ELLIPSIS: case TokenType::HASH:
        case TokenType::HASH_HASH: case TokenType::SCOPE_RESOLUTION:
            return true;
        default:
            return false;
    }
}

std::string Token::toString() const {
    std::string result = TokenUtils::tokenTypeToString(type_);
    if (!lexeme_.empty()) {
        result += " '" + lexeme_ + "'";
    }
    if (!value_.empty() && value_ != lexeme_) {
        result += " (value: " + value_ + ")";
    }
    result += " at " + location_.toString();
    return result;
}

// ============================================================================
// TokenUtils - Implementación
// ============================================================================

std::string TokenUtils::tokenTypeToString(TokenType type) {
    static const std::unordered_map<TokenType, std::string> tokenNames = {
        // Tokens especiales
        {TokenType::END_OF_FILE, "END_OF_FILE"},
        {TokenType::INVALID, "INVALID"},

        // Literales
        {TokenType::INTEGER_LITERAL, "INTEGER_LITERAL"},
        {TokenType::FLOAT_LITERAL, "FLOAT_LITERAL"},
        {TokenType::CHAR_LITERAL, "CHAR_LITERAL"},
        {TokenType::STRING_LITERAL, "STRING_LITERAL"},
        {TokenType::TRUE_LITERAL, "TRUE_LITERAL"},
        {TokenType::FALSE_LITERAL, "FALSE_LITERAL"},
        {TokenType::NULLPTR_LITERAL, "NULLPTR_LITERAL"},

        // Identificadores y palabras clave
        {TokenType::IDENTIFIER, "IDENTIFIER"},

        // Tipos fundamentales
        {TokenType::VOID, "VOID"}, {TokenType::INT, "INT"}, {TokenType::CHAR, "CHAR"},
        {TokenType::SHORT, "SHORT"}, {TokenType::LONG, "LONG"}, {TokenType::FLOAT, "FLOAT"},
        {TokenType::DOUBLE, "DOUBLE"}, {TokenType::BOOL, "BOOL"},

        // Calificadores
        {TokenType::CONST, "CONST"}, {TokenType::VOLATILE, "VOLATILE"},
        {TokenType::CONSTEVAL, "CONSTEVAL"}, {TokenType::CONSTEXPR, "CONSTEXPR"},
        {TokenType::CONSTINIT, "CONSTINIT"}, {TokenType::MUTABLE, "MUTABLE"},

        // Especificadores de almacenamiento
        {TokenType::STATIC, "STATIC"}, {TokenType::EXTERN, "EXTERN"},
        {TokenType::INLINE, "INLINE"}, {TokenType::THREAD_LOCAL, "THREAD_LOCAL"},

        // Control de acceso
        {TokenType::PUBLIC, "PUBLIC"}, {TokenType::PRIVATE, "PRIVATE"}, {TokenType::PROTECTED, "PROTECTED"},

        // Estructuras de control
        {TokenType::IF, "IF"}, {TokenType::ELSE, "ELSE"}, {TokenType::WHILE, "WHILE"},
        {TokenType::FOR, "FOR"}, {TokenType::DO, "DO"}, {TokenType::SWITCH, "SWITCH"},
        {TokenType::CASE, "CASE"}, {TokenType::DEFAULT, "DEFAULT"}, {TokenType::BREAK, "BREAK"},
        {TokenType::CONTINUE, "CONTINUE"}, {TokenType::RETURN, "RETURN"}, {TokenType::GOTO, "GOTO"},

        // Estructuras de datos
        {TokenType::STRUCT, "STRUCT"}, {TokenType::CLASS, "CLASS"}, {TokenType::UNION, "UNION"},
        {TokenType::ENUM, "ENUM"},

        // Funciones y métodos
        {TokenType::VIRTUAL, "VIRTUAL"}, {TokenType::OVERRIDE, "OVERRIDE"}, {TokenType::FINAL, "FINAL"},
        {TokenType::NOEXCEPT, "NOEXCEPT"},

        // Excepciones
        {TokenType::TRY, "TRY"}, {TokenType::CATCH, "CATCH"}, {TokenType::THROW, "THROW"},

        // Templates y conceptos
        {TokenType::TEMPLATE, "TEMPLATE"}, {TokenType::TYPENAME, "TYPENAME"},
        {TokenType::CONCEPT, "CONCEPT"}, {TokenType::REQUIRES, "REQUIRES"},

        // Espacios de nombres
        {TokenType::NAMESPACE, "NAMESPACE"}, {TokenType::USING, "USING"},

        // Operadores y conversión
        {TokenType::OPERATOR, "OPERATOR"}, {TokenType::EXPLICIT, "EXPLICIT"},

        // Misceláneo
        {TokenType::SIZEOF, "SIZEOF"}, {TokenType::ALIGNOF, "ALIGNOF"}, {TokenType::ALIGNAS, "ALIGNAS"},
        {TokenType::TYPEID, "TYPEID"}, {TokenType::DECLTYPE, "DECLTYPE"}, {TokenType::AUTO, "AUTO"},

        // C++20 específico
        {TokenType::CO_AWAIT, "CO_AWAIT"}, {TokenType::CO_RETURN, "CO_RETURN"}, {TokenType::CO_YIELD, "CO_YIELD"},
        {TokenType::MODULE, "MODULE"}, {TokenType::IMPORT, "IMPORT"}, {TokenType::EXPORT, "EXPORT"},

        // Operadores aritméticos
        {TokenType::PLUS, "PLUS"}, {TokenType::MINUS, "MINUS"}, {TokenType::STAR, "STAR"},
        {TokenType::SLASH, "SLASH"}, {TokenType::PERCENT, "PERCENT"}, {TokenType::INCREMENT, "INCREMENT"},
        {TokenType::DECREMENT, "DECREMENT"},

        // Operadores de comparación
        {TokenType::EQUAL, "EQUAL"}, {TokenType::NOT_EQUAL, "NOT_EQUAL"}, {TokenType::LESS, "LESS"},
        {TokenType::GREATER, "GREATER"}, {TokenType::LESS_EQUAL, "LESS_EQUAL"}, {TokenType::GREATER_EQUAL, "GREATER_EQUAL"},
        {TokenType::SPACESHIP, "SPACESHIP"},

        // Operadores lógicos
        {TokenType::LOGICAL_AND, "LOGICAL_AND"}, {TokenType::LOGICAL_OR, "LOGICAL_OR"}, {TokenType::LOGICAL_NOT, "LOGICAL_NOT"},

        // Operadores bit a bit
        {TokenType::BIT_AND, "BIT_AND"}, {TokenType::BIT_OR, "BIT_OR"}, {TokenType::BIT_XOR, "BIT_XOR"},
        {TokenType::BIT_NOT, "BIT_NOT"}, {TokenType::LEFT_SHIFT, "LEFT_SHIFT"}, {TokenType::RIGHT_SHIFT, "RIGHT_SHIFT"},

        // Operadores de asignación
        {TokenType::ASSIGN, "ASSIGN"}, {TokenType::PLUS_ASSIGN, "PLUS_ASSIGN"}, {TokenType::MINUS_ASSIGN, "MINUS_ASSIGN"},
        {TokenType::MUL_ASSIGN, "MUL_ASSIGN"}, {TokenType::DIV_ASSIGN, "DIV_ASSIGN"}, {TokenType::MOD_ASSIGN, "MOD_ASSIGN"},
        {TokenType::AND_ASSIGN, "AND_ASSIGN"}, {TokenType::OR_ASSIGN, "OR_ASSIGN"}, {TokenType::XOR_ASSIGN, "XOR_ASSIGN"},
        {TokenType::LEFT_SHIFT_ASSIGN, "LEFT_SHIFT_ASSIGN"}, {TokenType::RIGHT_SHIFT_ASSIGN, "RIGHT_SHIFT_ASSIGN"},

        // Operadores misceláneos
        {TokenType::ARROW, "ARROW"}, {TokenType::ARROW_STAR, "ARROW_STAR"}, {TokenType::DOT, "DOT"},
        {TokenType::DOT_STAR, "DOT_STAR"}, {TokenType::QUESTION, "QUESTION"}, {TokenType::COLON, "COLON"},
        {TokenType::SEMICOLON, "SEMICOLON"}, {TokenType::COMMA, "COMMA"},

        // Paréntesis y brackets
        {TokenType::LEFT_PAREN, "LEFT_PAREN"}, {TokenType::RIGHT_PAREN, "RIGHT_PAREN"},
        {TokenType::LEFT_BRACKET, "LEFT_BRACKET"}, {TokenType::RIGHT_BRACKET, "RIGHT_BRACKET"},
        {TokenType::LEFT_BRACE, "LEFT_BRACE"}, {TokenType::RIGHT_BRACE, "RIGHT_BRACE"},

        // Preprocesador
        {TokenType::HASH, "HASH"}, {TokenType::HASH_HASH, "HASH_HASH"},

        // Otros
        {TokenType::ELLIPSIS, "ELLIPSIS"}, {TokenType::SCOPE_RESOLUTION, "SCOPE_RESOLUTION"},

        // Tokens especiales C++20
        {TokenType::USER_DEFINED_INTEGER_LITERAL, "USER_DEFINED_INTEGER_LITERAL"},
        {TokenType::USER_DEFINED_FLOAT_LITERAL, "USER_DEFINED_FLOAT_LITERAL"},
        {TokenType::USER_DEFINED_CHAR_LITERAL, "USER_DEFINED_CHAR_LITERAL"},
        {TokenType::USER_DEFINED_STRING_LITERAL, "USER_DEFINED_STRING_LITERAL"},

        {TokenType::THREE_WAY_COMPARISON, "THREE_WAY_COMPARISON"},
        {TokenType::MODULE_KEYWORD, "MODULE_KEYWORD"}, {TokenType::IMPORT_KEYWORD, "IMPORT_KEYWORD"}, {TokenType::EXPORT_KEYWORD, "EXPORT_KEYWORD"},
        {TokenType::CO_AWAIT_KEYWORD, "CO_AWAIT_KEYWORD"}, {TokenType::CO_RETURN_KEYWORD, "CO_RETURN_KEYWORD"}, {TokenType::CO_YIELD_KEYWORD, "CO_YIELD_KEYWORD"},
        {TokenType::CONCEPT_KEYWORD, "CONCEPT_KEYWORD"}, {TokenType::REQUIRES_KEYWORD, "REQUIRES_KEYWORD"}
    };

    auto it = tokenNames.find(type);
    if (it != tokenNames.end()) {
        return it->second;
    }
    return "UNKNOWN_TOKEN";
}

TokenType TokenUtils::getKeywordType(const std::string& identifier) {
    static const std::unordered_map<std::string, TokenType> keywords = {
        // Tipos fundamentales
        {"void", TokenType::VOID}, {"int", TokenType::INT}, {"char", TokenType::CHAR},
        {"short", TokenType::SHORT}, {"long", TokenType::LONG}, {"float", TokenType::FLOAT},
        {"double", TokenType::DOUBLE}, {"bool", TokenType::BOOL},

        // Calificadores
        {"const", TokenType::CONST}, {"volatile", TokenType::VOLATILE},
        {"consteval", TokenType::CONSTEVAL}, {"constexpr", TokenType::CONSTEXPR},
        {"constinit", TokenType::CONSTINIT}, {"mutable", TokenType::MUTABLE},

        // Especificadores de almacenamiento
        {"static", TokenType::STATIC}, {"extern", TokenType::EXTERN},
        {"inline", TokenType::INLINE}, {"thread_local", TokenType::THREAD_LOCAL},

        // Control de acceso
        {"public", TokenType::PUBLIC}, {"private", TokenType::PRIVATE}, {"protected", TokenType::PROTECTED},

        // Estructuras de control
        {"if", TokenType::IF}, {"else", TokenType::ELSE}, {"while", TokenType::WHILE},
        {"for", TokenType::FOR}, {"do", TokenType::DO}, {"switch", TokenType::SWITCH},
        {"case", TokenType::CASE}, {"default", TokenType::DEFAULT}, {"break", TokenType::BREAK},
        {"continue", TokenType::CONTINUE}, {"return", TokenType::RETURN}, {"goto", TokenType::GOTO},

        // Estructuras de datos
        {"struct", TokenType::STRUCT}, {"class", TokenType::CLASS}, {"union", TokenType::UNION},
        {"enum", TokenType::ENUM},

        // Funciones y métodos
        {"virtual", TokenType::VIRTUAL}, {"override", TokenType::OVERRIDE}, {"final", TokenType::FINAL},
        {"noexcept", TokenType::NOEXCEPT},

        // Excepciones
        {"try", TokenType::TRY}, {"catch", TokenType::CATCH}, {"throw", TokenType::THROW},

        // Templates y conceptos
        {"template", TokenType::TEMPLATE}, {"typename", TokenType::TYPENAME},
        {"concept", TokenType::CONCEPT}, {"requires", TokenType::REQUIRES},

        // Espacios de nombres
        {"namespace", TokenType::NAMESPACE}, {"using", TokenType::USING},

        // Operadores y conversión
        {"operator", TokenType::OPERATOR}, {"explicit", TokenType::EXPLICIT},

        // Misceláneo
        {"sizeof", TokenType::SIZEOF}, {"alignof", TokenType::ALIGNOF}, {"alignas", TokenType::ALIGNAS},
        {"typeid", TokenType::TYPEID}, {"decltype", TokenType::DECLTYPE}, {"auto", TokenType::AUTO},

        // C++20 específico
        {"co_await", TokenType::CO_AWAIT}, {"co_return", TokenType::CO_RETURN}, {"co_yield", TokenType::CO_YIELD},
        {"module", TokenType::MODULE}, {"import", TokenType::IMPORT}, {"export", TokenType::EXPORT},

        // Literales
        {"true", TokenType::TRUE_LITERAL}, {"false", TokenType::FALSE_LITERAL},
        {"nullptr", TokenType::NULLPTR_LITERAL}
    };

    auto it = keywords.find(identifier);
    if (it != keywords.end()) {
        return it->second;
    }
    return TokenType::IDENTIFIER;
}

int TokenUtils::getOperatorPrecedence(TokenType type) {
    switch (type) {
        case TokenType::SCOPE_RESOLUTION: return 1;  // ::

        case TokenType::INCREMENT: case TokenType::DECREMENT: return 2;  // ++ --

        case TokenType::DOT_STAR: case TokenType::ARROW_STAR: return 3;  // .* ->*

        case TokenType::STAR: case TokenType::SLASH: case TokenType::PERCENT: return 4;  // * / %

        case TokenType::PLUS: case TokenType::MINUS: return 5;  // + -

        case TokenType::LEFT_SHIFT: case TokenType::RIGHT_SHIFT: return 6;  // << >>

        case TokenType::SPACESHIP: return 7;  // <=>

        case TokenType::LESS: case TokenType::GREATER:
        case TokenType::LESS_EQUAL: case TokenType::GREATER_EQUAL: return 8;  // < > <= >=

        case TokenType::EQUAL: case TokenType::NOT_EQUAL: return 9;  // == !=

        case TokenType::BIT_AND: return 10;  // &

        case TokenType::BIT_XOR: return 11;  // ^

        case TokenType::BIT_OR: return 12;  // |

        case TokenType::LOGICAL_AND: return 13;  // &&

        case TokenType::LOGICAL_OR: return 14;  // ||

        case TokenType::QUESTION: return 15;  // ?

        case TokenType::ASSIGN: case TokenType::PLUS_ASSIGN: case TokenType::MINUS_ASSIGN:
        case TokenType::MUL_ASSIGN: case TokenType::DIV_ASSIGN: case TokenType::MOD_ASSIGN:
        case TokenType::AND_ASSIGN: case TokenType::OR_ASSIGN: case TokenType::XOR_ASSIGN:
        case TokenType::LEFT_SHIFT_ASSIGN: case TokenType::RIGHT_SHIFT_ASSIGN: return 16;  // = += -= *= /= %= &= |= ^= <<= >>=

        case TokenType::COMMA: return 17;  // ,

        default: return 0;  // No es operador
    }
}

bool TokenUtils::isUnaryOperator(TokenType type) {
    switch (type) {
        case TokenType::PLUS: case TokenType::MINUS: case TokenType::STAR:
        case TokenType::BIT_AND: case TokenType::BIT_NOT: case TokenType::LOGICAL_NOT:
        case TokenType::INCREMENT: case TokenType::DECREMENT:
            return true;
        default:
            return false;
    }
}

bool TokenUtils::isBinaryOperator(TokenType type) {
    switch (type) {
        case TokenType::PLUS: case TokenType::MINUS: case TokenType::STAR:
        case TokenType::SLASH: case TokenType::PERCENT: case TokenType::EQUAL:
        case TokenType::NOT_EQUAL: case TokenType::LESS: case TokenType::GREATER:
        case TokenType::LESS_EQUAL: case TokenType::GREATER_EQUAL: case TokenType::SPACESHIP:
        case TokenType::LOGICAL_AND: case TokenType::LOGICAL_OR: case TokenType::BIT_AND:
        case TokenType::BIT_OR: case TokenType::BIT_XOR: case TokenType::LEFT_SHIFT:
        case TokenType::RIGHT_SHIFT: case TokenType::ASSIGN: case TokenType::PLUS_ASSIGN:
        case TokenType::MINUS_ASSIGN: case TokenType::MUL_ASSIGN: case TokenType::DIV_ASSIGN:
        case TokenType::MOD_ASSIGN: case TokenType::AND_ASSIGN: case TokenType::OR_ASSIGN:
        case TokenType::XOR_ASSIGN: case TokenType::LEFT_SHIFT_ASSIGN: case TokenType::RIGHT_SHIFT_ASSIGN:
        case TokenType::COMMA:
            return true;
        default:
            return false;
    }
}

bool TokenUtils::isAssignmentOperator(TokenType type) {
    switch (type) {
        case TokenType::ASSIGN: case TokenType::PLUS_ASSIGN: case TokenType::MINUS_ASSIGN:
        case TokenType::MUL_ASSIGN: case TokenType::DIV_ASSIGN: case TokenType::MOD_ASSIGN:
        case TokenType::AND_ASSIGN: case TokenType::OR_ASSIGN: case TokenType::XOR_ASSIGN:
        case TokenType::LEFT_SHIFT_ASSIGN: case TokenType::RIGHT_SHIFT_ASSIGN:
            return true;
        default:
            return false;
    }
}

} // namespace cpp20::compiler::frontend::lexer
