/**
 * @file IR.h
 * @brief Representación Intermedia (IR) del compilador C++20
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>

namespace cpp20::compiler::ir {

/**
 * @brief Tipos de datos en la IR
 */
enum class IRType {
    Void,
    Bool,
    Char,
    Short,
    Int,
    Long,
    LongLong,
    Float,
    Double,
    Pointer,    // Puntero genérico
    Array,      // Array
    Struct,     // Struct/class
    Function    // Tipo función
};

/**
 * @brief Información de tipo con tamaño y alineación
 */
struct TypeInfo {
    IRType type;
    size_t size = 0;        // Tamaño en bytes
    size_t alignment = 1;   // Alineación en bytes
    std::string typeName;   // Nombre del tipo (para structs, etc.)

    TypeInfo(IRType t = IRType::Void, size_t sz = 0, size_t align = 1,
             const std::string& name = "")
        : type(t), size(sz), alignment(align), typeName(name) {}
};

/**
 * @brief Operandos de las instrucciones IR
 */
class IROperand {
public:
    enum class Kind {
        Register,       // Registro virtual
        Immediate,      // Valor inmediato
        Label,          // Etiqueta
        Global,         // Variable global
        Parameter       // Parámetro de función
    };

    IROperand(Kind k = Kind::Register) : kind_(k) {}

    Kind getKind() const { return kind_; }

    // Métodos para diferentes tipos de operandos
    virtual std::string toString() const = 0;
    virtual ~IROperand() = default;

protected:
    Kind kind_;
};

/**
 * @brief Registro virtual
 */
class Register : public IROperand {
public:
    Register(int id, const TypeInfo& type)
        : IROperand(Kind::Register), id_(id), type_(type) {}

    int getId() const { return id_; }
    const TypeInfo& getType() const { return type_; }

    std::string toString() const override {
        return "%r" + std::to_string(id_);
    }

private:
    int id_;
    TypeInfo type_;
};

/**
 * @brief Valor inmediato
 */
class Immediate : public IROperand {
public:
    using ValueType = std::variant<int64_t, double, bool, std::string>;

    Immediate(ValueType value, const TypeInfo& type)
        : IROperand(Kind::Immediate), value_(value), type_(type) {}

    const ValueType& getValue() const { return value_; }
    const TypeInfo& getType() const { return type_; }

    std::string toString() const override;

private:
    ValueType value_;
    TypeInfo type_;
};

/**
 * @brief Etiqueta
 */
class Label : public IROperand {
public:
    Label(const std::string& name)
        : IROperand(Kind::Label), name_(name) {}

    const std::string& getName() const { return name_; }

    std::string toString() const override {
        return name_;
    }

private:
    std::string name_;
};

/**
 * @brief Variable global
 */
class GlobalVar : public IROperand {
public:
    GlobalVar(const std::string& name, const TypeInfo& type)
        : IROperand(Kind::Global), name_(name), type_(type) {}

    const std::string& getName() const { return name_; }
    const TypeInfo& getType() const { return type_; }

    std::string toString() const override {
        return "@" + name_;
    }

private:
    std::string name_;
    TypeInfo type_;
};

/**
 * @brief Parámetro de función
 */
class Parameter : public IROperand {
public:
    Parameter(int index, const TypeInfo& type)
        : IROperand(Kind::Parameter), index_(index), type_(type) {}

    int getIndex() const { return index_; }
    const TypeInfo& getType() const { return type_; }

    std::string toString() const override {
        return "%arg" + std::to_string(index_);
    }

private:
    int index_;
    TypeInfo type_;
};

/**
 * @brief Operaciones de la IR (formato de tres direcciones)
 */
enum class IROpcode {
    // Operaciones aritméticas
    Add, Sub, Mul, Div, Mod, Neg,

    // Operaciones de comparación
    CmpEQ, CmpNE, CmpLT, CmpLE, CmpGT, CmpGE,

    // Operaciones lógicas
    And, Or, Xor, Not, Shl, Shr,

    // Operaciones de memoria
    Load, Store, Alloca, GetElementPtr,

    // Operaciones de control de flujo
    Br, BrCond, Call, Ret,

    // Operaciones de conversión
    Trunc, ZExt, SExt, FPTrunc, FPExt, FPToSI, SIToFP,

    // Operaciones especiales
    Phi, Select
};

/**
 * @brief Instrucción IR
 */
class IRInstruction {
public:
    IRInstruction(IROpcode opcode, const TypeInfo& resultType = TypeInfo())
        : opcode_(opcode), resultType_(resultType) {}

    virtual ~IRInstruction() = default;

    IROpcode getOpcode() const { return opcode_; }
    const TypeInfo& getResultType() const { return resultType_; }

    // Operandos
    void addOperand(std::shared_ptr<IROperand> operand) {
        operands_.push_back(operand);
    }

    const std::vector<std::shared_ptr<IROperand>>& getOperands() const {
        return operands_;
    }

    // Resultado (registro destino)
    void setResult(std::shared_ptr<Register> result) {
        result_ = result;
    }

    std::shared_ptr<Register> getResult() const { return result_; }

    virtual std::string toString() const = 0;

protected:
    IROpcode opcode_;
    TypeInfo resultType_;
    std::vector<std::shared_ptr<IROperand>> operands_;
    std::shared_ptr<Register> result_;
};

/**
 * @brief Instrucción binaria (Add, Sub, Mul, etc.)
 */
class BinaryInstruction : public IRInstruction {
public:
    BinaryInstruction(IROpcode opcode, std::shared_ptr<IROperand> left,
                     std::shared_ptr<IROperand> right, const TypeInfo& resultType)
        : IRInstruction(opcode, resultType) {
        addOperand(left);
        addOperand(right);
    }

    std::string toString() const override;
};

/**
 * @brief Instrucción unaria (Neg, Not, etc.)
 */
class UnaryInstruction : public IRInstruction {
public:
    UnaryInstruction(IROpcode opcode, std::shared_ptr<IROperand> operand,
                    const TypeInfo& resultType)
        : IRInstruction(opcode, resultType) {
        addOperand(operand);
    }

    std::string toString() const override;
};

/**
 * @brief Instrucción de carga (Load)
 */
class LoadInstruction : public IRInstruction {
public:
    LoadInstruction(std::shared_ptr<IROperand> address, const TypeInfo& resultType)
        : IRInstruction(IROpcode::Load, resultType) {
        addOperand(address);
    }

    std::string toString() const override;
};

/**
 * @brief Instrucción de almacenamiento (Store)
 */
class StoreInstruction : public IRInstruction {
public:
    StoreInstruction(std::shared_ptr<IROperand> value, std::shared_ptr<IROperand> address)
        : IRInstruction(IROpcode::Store) {
        addOperand(value);
        addOperand(address);
    }

    std::string toString() const override;
};

/**
 * @brief Instrucción de salto condicional
 */
class BranchInstruction : public IRInstruction {
public:
    BranchInstruction(std::shared_ptr<IROperand> condition,
                     std::shared_ptr<Label> trueLabel,
                     std::shared_ptr<Label> falseLabel)
        : IRInstruction(IROpcode::BrCond) {
        addOperand(condition);
        addOperand(trueLabel);
        addOperand(falseLabel);
    }

    BranchInstruction(std::shared_ptr<Label> target)
        : IRInstruction(IROpcode::Br) {
        addOperand(target);
    }

    std::string toString() const override;
};

/**
 * @brief Instrucción de retorno
 */
class ReturnInstruction : public IRInstruction {
public:
    ReturnInstruction(std::shared_ptr<IROperand> value = nullptr)
        : IRInstruction(IROpcode::Ret) {
        if (value) {
            addOperand(value);
        }
    }

    std::string toString() const override;
};

/**
 * @brief Instrucción de llamada a función
 */
class CallInstruction : public IRInstruction {
public:
    CallInstruction(std::shared_ptr<IROperand> function,
                   const std::vector<std::shared_ptr<IROperand>>& args,
                   const TypeInfo& resultType)
        : IRInstruction(IROpcode::Call, resultType) {
        addOperand(function);
        for (auto& arg : args) {
            addOperand(arg);
        }
    }

    std::string toString() const override;
};

/**
 * @brief Bloque básico de instrucciones
 */
class BasicBlock {
public:
    BasicBlock(const std::string& name)
        : name_(name), label_(std::make_shared<Label>(name)) {}

    void addInstruction(std::unique_ptr<IRInstruction> instruction) {
        instructions_.push_back(std::move(instruction));
    }

    const std::string& getName() const { return name_; }
    const std::vector<std::unique_ptr<IRInstruction>>& getInstructions() const {
        return instructions_;
    }

    std::shared_ptr<Label> getLabel() const { return label_; }

    std::string toString() const;

private:
    std::string name_;
    std::shared_ptr<Label> label_;
    std::vector<std::unique_ptr<IRInstruction>> instructions_;
};

/**
 * @brief Función en IR
 */
class IRFunction {
public:
    IRFunction(const std::string& name, const TypeInfo& returnType,
               const std::vector<TypeInfo>& paramTypes)
        : name_(name), returnType_(returnType), paramTypes_(paramTypes) {}

    void addBasicBlock(std::unique_ptr<BasicBlock> block) {
        blocks_.push_back(std::move(block));
    }

    void addParameter(const std::string& name, const TypeInfo& type) {
        paramNames_.push_back(name);
        paramTypes_.push_back(type);
    }

    const std::string& getName() const { return name_; }
    const TypeInfo& getReturnType() const { return returnType_; }
    const std::vector<TypeInfo>& getParamTypes() const { return paramTypes_; }
    const std::vector<std::string>& getParamNames() const { return paramNames_; }
    const std::vector<std::unique_ptr<BasicBlock>>& getBasicBlocks() const { return blocks_; }

    std::string toString() const;

private:
    std::string name_;
    TypeInfo returnType_;
    std::vector<TypeInfo> paramTypes_;
    std::vector<std::string> paramNames_;
    std::vector<std::unique_ptr<BasicBlock>> blocks_;
};

/**
 * @brief Variable global en IR
 */
class IRGlobalVariable {
public:
    IRGlobalVariable(const std::string& name, const TypeInfo& type,
                    std::shared_ptr<IROperand> initializer = nullptr)
        : name_(name), type_(type), initializer_(initializer) {}

    const std::string& getName() const { return name_; }
    const TypeInfo& getType() const { return type_; }
    std::shared_ptr<IROperand> getInitializer() const { return initializer_; }

    std::string toString() const;

private:
    std::string name_;
    TypeInfo type_;
    std::shared_ptr<IROperand> initializer_;
};

/**
 * @brief Módulo IR completo
 */
class IRModule {
public:
    IRModule(const std::string& name) : name_(name) {}

    void addFunction(std::unique_ptr<IRFunction> function) {
        functions_.push_back(std::move(function));
    }

    void addGlobalVariable(std::unique_ptr<IRGlobalVariable> global) {
        globals_.push_back(std::move(global));
    }

    const std::string& getName() const { return name_; }
    const std::vector<std::unique_ptr<IRFunction>>& getFunctions() const { return functions_; }
    const std::vector<std::unique_ptr<IRGlobalVariable>>& getGlobals() const { return globals_; }

    std::string toString() const;

private:
    std::string name_;
    std::vector<std::unique_ptr<IRFunction>> functions_;
    std::vector<std::unique_ptr<IRGlobalVariable>> globals_;
};

/**
 * @brief Constructor de IR
 */
class IRBuilder {
public:
    IRBuilder();

    // Creación de operandos
    std::shared_ptr<Register> createRegister(const TypeInfo& type);
    std::shared_ptr<Immediate> createImmediate(int64_t value, const TypeInfo& type);
    std::shared_ptr<Immediate> createImmediate(double value, const TypeInfo& type);
    std::shared_ptr<Immediate> createImmediate(bool value, const TypeInfo& type);
    std::shared_ptr<Label> createLabel(const std::string& name);
    std::shared_ptr<GlobalVar> createGlobal(const std::string& name, const TypeInfo& type);
    std::shared_ptr<Parameter> createParameter(int index, const TypeInfo& type);

    // Creación de instrucciones
    std::unique_ptr<BinaryInstruction> createBinary(IROpcode opcode,
                                                   std::shared_ptr<IROperand> left,
                                                   std::shared_ptr<IROperand> right,
                                                   const TypeInfo& resultType);

    std::unique_ptr<UnaryInstruction> createUnary(IROpcode opcode,
                                                 std::shared_ptr<IROperand> operand,
                                                 const TypeInfo& resultType);

    std::unique_ptr<LoadInstruction> createLoad(std::shared_ptr<IROperand> address,
                                               const TypeInfo& resultType);

    std::unique_ptr<StoreInstruction> createStore(std::shared_ptr<IROperand> value,
                                                 std::shared_ptr<IROperand> address);

    std::unique_ptr<BranchInstruction> createBranch(std::shared_ptr<Label> target);

    std::unique_ptr<BranchInstruction> createConditionalBranch(std::shared_ptr<IROperand> condition,
                                                              std::shared_ptr<Label> trueLabel,
                                                              std::shared_ptr<Label> falseLabel);

    std::unique_ptr<ReturnInstruction> createReturn(std::shared_ptr<IROperand> value = nullptr);

    std::unique_ptr<CallInstruction> createCall(std::shared_ptr<IROperand> function,
                                               const std::vector<std::shared_ptr<IROperand>>& args,
                                               const TypeInfo& resultType);

    // Gestión de bloques
    std::unique_ptr<BasicBlock> createBasicBlock(const std::string& name);
    std::unique_ptr<IRFunction> createFunction(const std::string& name,
                                              const TypeInfo& returnType,
                                              const std::vector<TypeInfo>& paramTypes);
    std::unique_ptr<IRGlobalVariable> createGlobalVariable(const std::string& name,
                                                          const TypeInfo& type,
                                                          std::shared_ptr<IROperand> initializer = nullptr);
    std::unique_ptr<IRModule> createModule(const std::string& name);

private:
    int nextRegisterId_ = 0;
    int nextLabelId_ = 0;
};

} // namespace cpp20::compiler::ir
