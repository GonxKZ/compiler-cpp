/**
 * @file IR.cpp
 * @brief Implementación de la Representación Intermedia (IR)
 */

#include <compiler/ir/IR.h>
#include <sstream>

namespace cpp20::compiler::ir {

// ============================================================================
// Implementaciones de operandos
// ============================================================================

std::string Immediate::toString() const {
    std::stringstream ss;

    switch (type_.type) {
        case IRType::Bool:
            ss << (std::get<bool>(value_) ? "true" : "false");
            break;
        case IRType::Char:
            ss << "'" << std::get<int64_t>(value_) << "'";
            break;
        case IRType::Int:
        case IRType::Long:
        case IRType::LongLong:
            ss << std::get<int64_t>(value_);
            break;
        case IRType::Float:
        case IRType::Double:
            ss << std::get<double>(value_);
            break;
        default:
            ss << "<immediate>";
            break;
    }

    return ss.str();
}

// ============================================================================
// Implementaciones de instrucciones
// ============================================================================

std::string BinaryInstruction::toString() const {
    if (!result_) return "<invalid binary instruction>";

    std::stringstream ss;
    ss << result_->toString() << " = ";

    std::string opStr;
    switch (opcode_) {
        case IROpcode::Add: opStr = "add"; break;
        case IROpcode::Sub: opStr = "sub"; break;
        case IROpcode::Mul: opStr = "mul"; break;
        case IROpcode::Div: opStr = "div"; break;
        case IROpcode::Mod: opStr = "mod"; break;
        case IROpcode::And: opStr = "and"; break;
        case IROpcode::Or: opStr = "or"; break;
        case IROpcode::Xor: opStr = "xor"; break;
        case IROpcode::Shl: opStr = "shl"; break;
        case IROpcode::Shr: opStr = "shr"; break;
        case IROpcode::CmpEQ: opStr = "cmp eq"; break;
        case IROpcode::CmpNE: opStr = "cmp ne"; break;
        case IROpcode::CmpLT: opStr = "cmp lt"; break;
        case IROpcode::CmpLE: opStr = "cmp le"; break;
        case IROpcode::CmpGT: opStr = "cmp gt"; break;
        case IROpcode::CmpGE: opStr = "cmp ge"; break;
        default: opStr = "<unknown>"; break;
    }

    ss << opStr << " " << operands_[0]->toString() << ", " << operands_[1]->toString();
    return ss.str();
}

std::string UnaryInstruction::toString() const {
    if (!result_) return "<invalid unary instruction>";

    std::stringstream ss;
    ss << result_->toString() << " = ";

    std::string opStr;
    switch (opcode_) {
        case IROpcode::Neg: opStr = "neg"; break;
        case IROpcode::Not: opStr = "not"; break;
        default: opStr = "<unknown>"; break;
    }

    ss << opStr << " " << operands_[0]->toString();
    return ss.str();
}

std::string LoadInstruction::toString() const {
    if (!result_) return "<invalid load instruction>";

    std::stringstream ss;
    ss << result_->toString() << " = load " << operands_[0]->toString();
    return ss.str();
}

std::string StoreInstruction::toString() const {
    std::stringstream ss;
    ss << "store " << operands_[0]->toString() << ", " << operands_[1]->toString();
    return ss.str();
}

std::string BranchInstruction::toString() const {
    std::stringstream ss;

    if (opcode_ == IROpcode::BrCond) {
        ss << "br " << operands_[0]->toString() << ", "
           << operands_[1]->toString() << ", " << operands_[2]->toString();
    } else {
        ss << "br " << operands_[0]->toString();
    }

    return ss.str();
}

std::string ReturnInstruction::toString() const {
    std::stringstream ss;
    ss << "ret";

    if (!operands_.empty()) {
        ss << " " << operands_[0]->toString();
    }

    return ss.str();
}

std::string CallInstruction::toString() const {
    std::stringstream ss;

    if (result_) {
        ss << result_->toString() << " = ";
    }

    ss << "call " << operands_[0]->toString() << "(";

    for (size_t i = 1; i < operands_.size(); ++i) {
        if (i > 1) ss << ", ";
        ss << operands_[i]->toString();
    }

    ss << ")";
    return ss.str();
}

// ============================================================================
// Implementaciones de bloques y funciones
// ============================================================================

std::string BasicBlock::toString() const {
    std::stringstream ss;
    ss << name_ << ":\n";

    for (const auto& inst : instructions_) {
        ss << "  " << inst->toString() << "\n";
    }

    return ss.str();
}

std::string IRFunction::toString() const {
    std::stringstream ss;

    // Firma de la función
    ss << "define " << returnType_.typeName << " @" << name_ << "(";

    for (size_t i = 0; i < paramTypes_.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << paramTypes_[i].typeName;
        if (i < paramNames_.size()) {
            ss << " %" << paramNames_[i];
        }
    }

    ss << ") {\n";

    // Bloques básicos
    for (const auto& block : blocks_) {
        ss << block->toString();
    }

    ss << "}\n";
    return ss.str();
}

std::string IRGlobalVariable::toString() const {
    std::stringstream ss;
    ss << "@" << name_ << " = global " << type_.typeName;

    if (initializer_) {
        ss << " " << initializer_->toString();
    } else {
        ss << " zeroinitializer";
    }

    return ss.str();
}

std::string IRModule::toString() const {
    std::stringstream ss;
    ss << "; Module: " << name_ << "\n\n";

    // Variables globales
    for (const auto& global : globals_) {
        ss << global->toString() << "\n";
    }

    if (!globals_.empty()) {
        ss << "\n";
    }

    // Funciones
    for (const auto& func : functions_) {
        ss << func->toString() << "\n";
    }

    return ss.str();
}

// ============================================================================
// IRBuilder - Implementación
// ============================================================================

IRBuilder::IRBuilder() = default;

std::shared_ptr<Register> IRBuilder::createRegister(const TypeInfo& type) {
    return std::make_shared<Register>(nextRegisterId_++, type);
}

std::shared_ptr<Immediate> IRBuilder::createImmediate(int64_t value, const TypeInfo& type) {
    return std::make_shared<Immediate>(value, type);
}

std::shared_ptr<Immediate> IRBuilder::createImmediate(double value, const TypeInfo& type) {
    return std::make_shared<Immediate>(value, type);
}

std::shared_ptr<Immediate> IRBuilder::createImmediate(bool value, const TypeInfo& type) {
    return std::make_shared<Immediate>(value, type);
}

std::shared_ptr<Label> IRBuilder::createLabel(const std::string& name) {
    if (name.empty()) {
        return std::make_shared<Label>("L" + std::to_string(nextLabelId_++));
    }
    return std::make_shared<Label>(name);
}

std::shared_ptr<GlobalVar> IRBuilder::createGlobal(const std::string& name, const TypeInfo& type) {
    return std::make_shared<GlobalVar>(name, type);
}

std::shared_ptr<Parameter> IRBuilder::createParameter(int index, const TypeInfo& type) {
    return std::make_shared<Parameter>(index, type);
}

std::unique_ptr<BinaryInstruction> IRBuilder::createBinary(
    IROpcode opcode,
    std::shared_ptr<IROperand> left,
    std::shared_ptr<IROperand> right,
    const TypeInfo& resultType) {

    auto inst = std::make_unique<BinaryInstruction>(opcode, left, right, resultType);
    inst->setResult(createRegister(resultType));
    return inst;
}

std::unique_ptr<UnaryInstruction> IRBuilder::createUnary(
    IROpcode opcode,
    std::shared_ptr<IROperand> operand,
    const TypeInfo& resultType) {

    auto inst = std::make_unique<UnaryInstruction>(opcode, operand, resultType);
    inst->setResult(createRegister(resultType));
    return inst;
}

std::unique_ptr<LoadInstruction> IRBuilder::createLoad(
    std::shared_ptr<IROperand> address,
    const TypeInfo& resultType) {

    auto inst = std::make_unique<LoadInstruction>(address, resultType);
    inst->setResult(createRegister(resultType));
    return inst;
}

std::unique_ptr<StoreInstruction> IRBuilder::createStore(
    std::shared_ptr<IROperand> value,
    std::shared_ptr<IROperand> address) {

    return std::make_unique<StoreInstruction>(value, address);
}

std::unique_ptr<BranchInstruction> IRBuilder::createBranch(std::shared_ptr<Label> target) {
    return std::make_unique<BranchInstruction>(target);
}

std::unique_ptr<BranchInstruction> IRBuilder::createConditionalBranch(
    std::shared_ptr<IROperand> condition,
    std::shared_ptr<Label> trueLabel,
    std::shared_ptr<Label> falseLabel) {

    return std::make_unique<BranchInstruction>(condition, trueLabel, falseLabel);
}

std::unique_ptr<ReturnInstruction> IRBuilder::createReturn(std::shared_ptr<IROperand> value) {
    return std::make_unique<ReturnInstruction>(value);
}

std::unique_ptr<CallInstruction> IRBuilder::createCall(
    std::shared_ptr<IROperand> function,
    const std::vector<std::shared_ptr<IROperand>>& args,
    const TypeInfo& resultType) {

    auto inst = std::make_unique<CallInstruction>(function, args, resultType);
    if (resultType.type != IRType::Void) {
        inst->setResult(createRegister(resultType));
    }
    return inst;
}

std::unique_ptr<BasicBlock> IRBuilder::createBasicBlock(const std::string& name) {
    return std::make_unique<BasicBlock>(name);
}

std::unique_ptr<IRFunction> IRBuilder::createFunction(
    const std::string& name,
    const TypeInfo& returnType,
    const std::vector<TypeInfo>& paramTypes) {

    return std::make_unique<IRFunction>(name, returnType, paramTypes);
}

std::unique_ptr<IRGlobalVariable> IRBuilder::createGlobalVariable(
    const std::string& name,
    const TypeInfo& type,
    std::shared_ptr<IROperand> initializer) {

    return std::make_unique<IRGlobalVariable>(name, type, initializer);
}

std::unique_ptr<IRModule> IRBuilder::createModule(const std::string& name) {
    return std::make_unique<IRModule>(name);
}

} // namespace cpp20::compiler::ir
