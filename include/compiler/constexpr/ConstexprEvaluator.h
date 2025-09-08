/**
 * @file ConstexprEvaluator.h
 * @brief Sistema de evaluación constexpr C++20
 */

#pragma once

#include <compiler/ast/ASTNode.h>
#include <compiler/common/diagnostics/DiagnosticEngine.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <variant>
#include <optional>

namespace cpp20::compiler::constexpr_eval {

/**
 * @brief Estado de evaluación constexpr
 */
enum class EvaluationResult {
    Success,           // Evaluación exitosa
    Error,            // Error durante evaluación
    NotConstexpr,     // Expresión no es constexpr
    Timeout,          // Timeout de evaluación
    RecursionLimit,   // Límite de recursión excedido
    MemoryLimit       // Límite de memoria excedido
};

/**
 * @brief Valor constexpr
 */
class ConstexprValue {
public:
    enum class ValueType {
        Integer,        // int, long, etc.
        FloatingPoint,  // float, double
        Boolean,        // bool
        Character,      // char
        String,         // const char*
        Pointer,        // Punteros (limitados)
        Nullptr,        // nullptr_t
        Reference,      // Referencias
        Uninitialized   // No inicializado
    };

    ConstexprValue() : type_(ValueType::Uninitialized) {}
    ConstexprValue(int val) : type_(ValueType::Integer), intValue_(val) {}
    ConstexprValue(long val) : type_(ValueType::Integer), intValue_(static_cast<int>(val)) {}
    ConstexprValue(bool val) : type_(ValueType::Boolean), boolValue_(val) {}
    ConstexprValue(char val) : type_(ValueType::Character), charValue_(val) {}
    ConstexprValue(double val) : type_(ValueType::FloatingPoint), doubleValue_(val) {}
    ConstexprValue(float val) : type_(ValueType::FloatingPoint), doubleValue_(static_cast<double>(val)) {}
    ConstexprValue(const std::string& val) : type_(ValueType::String), stringValue_(val) {}

    ValueType getType() const { return type_; }

    bool isInteger() const { return type_ == ValueType::Integer; }
    bool isBoolean() const { return type_ == ValueType::Boolean; }
    bool isCharacter() const { return type_ == ValueType::Character; }
    bool isFloatingPoint() const { return type_ == ValueType::FloatingPoint; }
    bool isString() const { return type_ == ValueType::String; }
    bool isPointer() const { return type_ == ValueType::Pointer; }
    bool isNullptr() const { return type_ == ValueType::Nullptr; }
    bool isReference() const { return type_ == ValueType::Reference; }
    bool isUninitialized() const { return type_ == ValueType::Uninitialized; }

    int asInteger() const { return intValue_; }
    bool asBoolean() const { return boolValue_; }
    char asCharacter() const { return charValue_; }
    double asFloatingPoint() const { return doubleValue_; }
    const std::string& asString() const { return stringValue_; }

    std::string toString() const;

private:
    ValueType type_;
    int intValue_ = 0;
    bool boolValue_ = false;
    char charValue_ = 0;
    double doubleValue_ = 0.0;
    std::string stringValue_;
};

/**
 * @brief Resultado de evaluación constexpr
 */
struct EvaluationContext {
    EvaluationResult result;
    ConstexprValue value;
    std::string errorMessage;
    std::vector<std::string> diagnosticNotes;
    size_t stepsExecuted = 0;

    EvaluationContext() : result(EvaluationResult::Success) {}
    EvaluationContext(EvaluationResult res) : result(res) {}
    EvaluationContext(EvaluationResult res, const std::string& msg)
        : result(res), errorMessage(msg) {}
};

/**
 * @brief Variable en scope constexpr
 */
struct ConstexprVariable {
    std::string name;
    ConstexprValue value;
    bool isConst = true;
    bool isInitialized = false;

    ConstexprVariable() = default;
    ConstexprVariable(const std::string& n, const ConstexprValue& v, bool isConst_ = true)
        : name(n), value(v), isConst(isConst_), isInitialized(true) {}
};

/**
 * @brief Scope de evaluación constexpr
 */
class EvaluationScope {
public:
    EvaluationScope() = default;

    void declareVariable(const std::string& name, const ConstexprValue& value, bool isConst = true);
    bool hasVariable(const std::string& name) const;
    const ConstexprVariable* getVariable(const std::string& name) const;
    bool updateVariable(const std::string& name, const ConstexprValue& value);

    void pushScope();
    void popScope();

private:
    std::vector<std::unordered_map<std::string, ConstexprVariable>> scopes_;
};

/**
 * @brief Memoria abstracta para evaluación constexpr
 */
class AbstractMemory {
public:
    struct MemoryObject {
        std::string type;
        size_t size = 0;
        std::vector<char> data;
        bool isInitialized = false;

        MemoryObject(const std::string& t, size_t s) : type(t), size(s), data(s, 0) {}
    };

    size_t allocate(const std::string& type, size_t size);
    bool deallocate(size_t address);
    MemoryObject* getObject(size_t address);
    const MemoryObject* getObject(size_t address) const;

    size_t getTotalAllocated() const { return totalAllocated_; }
    void clear();

private:
    std::unordered_map<size_t, std::unique_ptr<MemoryObject>> objects_;
    size_t nextAddress_ = 1; // 0 es nullptr
    size_t totalAllocated_ = 0;
};

/**
 * @brief Máquina Virtual para evaluación constexpr
 */
class ConstexprVM {
public:
    /**
     * @brief Constructor
     */
    ConstexprVM(diagnostics::DiagnosticEngine& diagEngine);

    /**
     * @brief Destructor
     */
    ~ConstexprVM();

    /**
     * @brief Evaluar expresión constexpr
     */
    EvaluationContext evaluate(const ast::ASTNode* expression,
                              const std::unordered_map<std::string, ConstexprValue>& parameters = {});

    /**
     * @brief Verificar si expresión es constexpr válida
     */
    bool isValidConstexpr(const ast::ASTNode* expression,
                         std::string& errorMessage);

    /**
     * @brief Configurar límites de evaluación
     */
    void setLimits(size_t maxSteps = 1000000,
                  size_t maxRecursion = 100,
                  size_t maxMemory = 1024 * 1024); // 1MB

    /**
     * @brief Obtener estadísticas de evaluación
     */
    struct VMStats {
        size_t evaluationsPerformed = 0;
        size_t stepsExecuted = 0;
        size_t maxRecursionDepth = 0;
        size_t memoryPeak = 0;
        size_t errors = 0;
    };
    VMStats getStats() const { return stats_; }

    /**
     * @brief Limpiar estado
     */
    void clear();

private:
    diagnostics::DiagnosticEngine& diagEngine_;
    EvaluationScope scope_;
    AbstractMemory memory_;
    VMStats stats_;

    // Límites
    size_t maxSteps_ = 1000000;
    size_t maxRecursion_ = 100;
    size_t maxMemory_ = 1024 * 1024;
    size_t currentRecursion_ = 0;

    /**
     * @brief Evaluar expresión específica
     */
    EvaluationContext evaluateExpression(const ast::ASTNode* node);

    /**
     * @brief Evaluar literal
     */
    EvaluationContext evaluateLiteral(const ast::ASTNode* node);

    /**
     * @brief Evaluar operador binario
     */
    EvaluationContext evaluateBinaryOp(const ast::ASTNode* node);

    /**
     * @brief Evaluar operador unario
     */
    EvaluationContext evaluateUnaryOp(const ast::ASTNode* node);

    /**
     * @brief Evaluar llamada a función
     */
    EvaluationContext evaluateFunctionCall(const ast::ASTNode* node);

    /**
     * @brief Evaluar variable
     */
    EvaluationContext evaluateVariable(const ast::ASTNode* node);

    /**
     * @brief Evaluar asignación
     */
    EvaluationContext evaluateAssignment(const ast::ASTNode* node);

    /**
     * @brief Evaluar declaración
     */
    EvaluationContext evaluateDeclaration(const ast::ASTNode* node);

    /**
     * @brief Evaluar if constexpr
     */
    EvaluationContext evaluateIfConstexpr(const ast::ASTNode* node);

    /**
     * @brief Evaluar operador ternario
     */
    EvaluationContext evaluateTernaryOp(const ast::ASTNode* node);

    /**
     * @brief Verificar límites
     */
    bool checkLimits();

    /**
     * @brief Incrementar contador de pasos
     */
    void incrementSteps();

    /**
     * @brief Crear error de evaluación
     */
    EvaluationContext createError(const std::string& message,
                                 const std::vector<std::string>& notes = {});

    /**
     * @brief Obtener valor de variable
     */
    std::optional<ConstexprValue> getVariableValue(const std::string& name);

    /**
     * @brief Establecer valor de variable
     */
    bool setVariableValue(const std::string& name, const ConstexprValue& value);
};

/**
 * @brief Sistema de evaluación constexpr C++20
 */
class ConstexprEvaluator {
public:
    /**
     * @brief Constructor
     */
    ConstexprEvaluator(diagnostics::DiagnosticEngine& diagEngine);

    /**
     * @brief Destructor
     */
    ~ConstexprEvaluator();

    /**
     * @brief Evaluar función constexpr
     */
    EvaluationContext evaluateFunction(const std::string& functionName,
                                     const std::vector<ConstexprValue>& arguments,
                                     const ast::ASTNode* functionBody);

    /**
     * @brief Evaluar expresión constexpr
     */
    EvaluationContext evaluateExpression(const ast::ASTNode* expression,
                                       const std::unordered_map<std::string, ConstexprValue>& context = {});

    /**
     * @brief Verificar si función es constexpr válida
     */
    bool isConstexprFunction(const ast::ASTNode* functionDecl,
                           std::string& errorMessage);

    /**
     * @brief Verificar si expresión es constexpr válida
     */
    bool isConstexprExpression(const ast::ASTNode* expression,
                             std::string& errorMessage);

    /**
     * @brief Registrar función constexpr
     */
    void registerConstexprFunction(const std::string& name,
                                 const ast::ASTNode* functionDecl);

    /**
     * @brief Configurar límites
     */
    void setLimits(size_t maxSteps = 1000000,
                  size_t maxRecursion = 100,
                  size_t maxMemory = 1024 * 1024);

    /**
     * @brief Obtener estadísticas
     */
    struct EvaluatorStats {
        size_t functionsEvaluated = 0;
        size_t expressionsEvaluated = 0;
        size_t totalSteps = 0;
        size_t errors = 0;
        size_t timeSpentMs = 0;
    };
    EvaluatorStats getStats() const;

    /**
     * @brief Limpiar cache y estado
     */
    void clear();

private:
    diagnostics::DiagnosticEngine& diagEngine_;
    std::unique_ptr<ConstexprVM> vm_;
    std::unordered_map<std::string, const ast::ASTNode*> constexprFunctions_;
    EvaluatorStats stats_;

    /**
     * @brief Verificar que función cumple reglas constexpr
     */
    bool validateConstexprFunction(const ast::ASTNode* functionDecl,
                                 std::string& errorMessage);

    /**
     * @brief Verificar que expresión cumple reglas constexpr
     */
    bool validateConstexprExpression(const ast::ASTNode* expression,
                                   std::string& errorMessage);

    /**
     * @brief Preparar contexto para evaluación
     */
    std::unordered_map<std::string, ConstexprValue> prepareContext(
        const ast::ASTNode* functionDecl,
        const std::vector<ConstexprValue>& arguments);
};

} // namespace cpp20::compiler::constexpr_eval
