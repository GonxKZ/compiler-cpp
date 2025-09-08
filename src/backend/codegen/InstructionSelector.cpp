/**
 * @file InstructionSelector.cpp
 * @brief Implementación del selector de instrucciones x86-64
 */

#include <compiler/backend/codegen/InstructionSelector.h>
#include <sstream>
#include <algorithm>

namespace cpp20::compiler::backend {

// ============================================================================
// InstructionSelector - Implementación
// ============================================================================

InstructionSelector::InstructionSelector(const abi::ABIContract& abiContract)
    : abiContract_(abiContract) {
}

InstructionSelector::~InstructionSelector() = default;

std::vector<X86Instruction> InstructionSelector::selectInstructions(
    const ir::IRFunction& function,
    const std::unordered_map<int, RegisterMapping>& registerMap) {

    std::vector<X86Instruction> instructions;

    // Procesar cada bloque básico
    for (const auto& block : function.getBasicBlocks()) {
        // Etiqueta del bloque
        X86Instruction labelInst;
        labelInst.opcode = X86Opcode::NOP; // Placeholder para etiqueta
        labelInst.comment = block->getName() + ":";
        instructions.push_back(labelInst);

        // Procesar instrucciones del bloque
        for (const auto& inst : block->getInstructions()) {
            auto selected = selectInstruction(*inst, registerMap);
            instructions.insert(instructions.end(), selected.begin(), selected.end());
        }
    }

    // Optimizar secuencia final
    optimizeInstructionSequence(instructions);

    return instructions;
}

std::vector<X86Instruction> InstructionSelector::selectInstruction(
    const ir::IRInstruction& instruction,
    const std::unordered_map<int, RegisterMapping>& registerMap) {

    switch (instruction.getOpcode()) {
        case ir::IROpcode::Add:
        case ir::IROpcode::Sub:
        case ir::IROpcode::Mul:
        case ir::IROpcode::Div:
        case ir::IROpcode::Mod:
        case ir::IROpcode::And:
        case ir::IROpcode::Or:
        case ir::IROpcode::Xor:
        case ir::IROpcode::CmpEQ:
        case ir::IROpcode::CmpNE:
        case ir::IROpcode::CmpLT:
        case ir::IROpcode::CmpLE:
        case ir::IROpcode::CmpGT:
        case ir::IROpcode::CmpGE:
            return selectBinaryOperation(
                static_cast<const ir::BinaryInstruction&>(instruction), registerMap);

        case ir::IROpcode::Neg:
        case ir::IROpcode::Not:
            return selectUnaryOperation(
                static_cast<const ir::UnaryInstruction&>(instruction), registerMap);

        case ir::IROpcode::Load:
            return selectLoad(
                static_cast<const ir::LoadInstruction&>(instruction), registerMap);

        case ir::IROpcode::Store:
            return selectStore(
                static_cast<const ir::StoreInstruction&>(instruction), registerMap);

        case ir::IROpcode::Br:
        case ir::IROpcode::BrCond:
            return selectBranch(
                static_cast<const ir::BranchInstruction&>(instruction));

        case ir::IROpcode::Ret:
            return selectReturn(
                static_cast<const ir::ReturnInstruction&>(instruction), registerMap);

        case ir::IROpcode::Call:
            return selectCall(
                static_cast<const ir::CallInstruction&>(instruction), registerMap);

        default:
            // Instrucción no soportada
            return {};
    }
}

std::vector<X86Instruction> InstructionSelector::generateFunctionPrologue(
    const ir::IRFunction& function,
    size_t stackSize) {

    std::vector<X86Instruction> prologue;

    // Push callee-saved registers
    for (int i = 0; i < 16; ++i) {
        if (abiContract_.isCalleeSavedRegister(i)) {
            X86Instruction pushInst(X86Opcode::PUSH);
            pushInst.operands.push_back(createRegisterOperand(static_cast<X86Register>(i)));
            prologue.push_back(pushInst);
        }
    }

    // Set up frame pointer
    X86Instruction movInst(X86Opcode::MOV);
    movInst.operands.push_back(createRegisterOperand(X86Register::RBP));
    movInst.operands.push_back(createRegisterOperand(X86Register::RSP));
    prologue.push_back(movInst);

    // Allocate stack space
    if (stackSize > 0) {
        X86Instruction subInst(X86Opcode::SUB);
        subInst.operands.push_back(createRegisterOperand(X86Register::RSP));
        subInst.operands.push_back(createImmediateOperand(stackSize));
        prologue.push_back(subInst);
    }

    return prologue;
}

std::vector<X86Instruction> InstructionSelector::generateFunctionEpilogue(
    const ir::IRFunction& function) {

    std::vector<X86Instruction> epilogue;

    // Deallocate stack space (simplificado)
    X86Instruction movInst(X86Opcode::MOV);
    movInst.operands.push_back(createRegisterOperand(X86Register::RSP));
    movInst.operands.push_back(createRegisterOperand(X86Register::RBP));
    epilogue.push_back(movInst);

    // Pop callee-saved registers (en orden inverso)
    for (int i = 15; i >= 0; --i) {
        if (abiContract_.isCalleeSavedRegister(i)) {
            X86Instruction popInst(X86Opcode::POP);
            popInst.operands.push_back(createRegisterOperand(static_cast<X86Register>(i)));
            epilogue.push_back(popInst);
        }
    }

    // Return
    X86Instruction retInst(X86Opcode::RET);
    epilogue.push_back(retInst);

    return epilogue;
}

std::string InstructionSelector::instructionsToAssembly(const std::vector<X86Instruction>& instructions) {
    std::stringstream ss;

    for (const auto& inst : instructions) {
        if (!inst.comment.empty() && inst.opcode == X86Opcode::NOP) {
            // Es una etiqueta o comentario
            ss << inst.comment << "\n";
            continue;
        }

        ss << "    " << opcodeToString(inst.opcode);

        for (size_t i = 0; i < inst.operands.size(); ++i) {
            if (i > 0) ss << ",";
            ss << " ";

            const auto& operand = inst.operands[i];
            switch (operand.mode) {
                case AddressingMode::Register:
                    ss << registerToString(operand.reg);
                    break;
                case AddressingMode::Immediate:
                    ss << operand.immediate;
                    break;
                case AddressingMode::MemoryDirect:
                    ss << "[" << operand.immediate << "]";
                    break;
                case AddressingMode::MemoryIndirect:
                    ss << "[" << registerToString(operand.reg) << "]";
                    break;
                case AddressingMode::MemoryBaseDisp:
                    ss << "[" << registerToString(operand.reg);
                    if (operand.displacement != 0) {
                        ss << (operand.displacement > 0 ? "+" : "") << operand.displacement;
                    }
                    ss << "]";
                    break;
                default:
                    ss << "<unknown>";
                    break;
            }
        }

        if (!inst.comment.empty()) {
            ss << "  ; " << inst.comment;
        }

        ss << "\n";
    }

    return ss.str();
}

// ============================================================================
// Métodos privados
// ============================================================================

std::vector<X86Instruction> InstructionSelector::selectBinaryOperation(
    const ir::BinaryInstruction& instruction,
    const std::unordered_map<int, RegisterMapping>& registerMap) {

    std::vector<X86Instruction> instructions;

    auto resultReg = getPhysicalRegister(instruction.getResult()->getId(), registerMap);
    auto leftOp = convertOperand(instruction.getOperands()[0], registerMap);
    auto rightOp = convertOperand(instruction.getOperands()[1], registerMap);

    X86Opcode opcode;
    switch (instruction.getOpcode()) {
        case ir::IROpcode::Add: opcode = X86Opcode::ADD; break;
        case ir::IROpcode::Sub: opcode = X86Opcode::SUB; break;
        case ir::IROpcode::And: opcode = X86Opcode::AND; break;
        case ir::IROpcode::Or: opcode = X86Opcode::OR; break;
        case ir::IROpcode::Xor: opcode = X86Opcode::XOR; break;
        default: return {}; // No soportado aún
    }

    // Movemos el operando izquierdo al registro resultado
    auto moveInsts = generateMove(createRegisterOperand(resultReg), leftOp);
    instructions.insert(instructions.end(), moveInsts.begin(), moveInsts.end());

    // Aplicamos la operación
    X86Instruction opInst(opcode);
    opInst.operands.push_back(createRegisterOperand(resultReg));
    opInst.operands.push_back(rightOp);
    instructions.push_back(opInst);

    return instructions;
}

std::vector<X86Instruction> InstructionSelector::selectUnaryOperation(
    const ir::UnaryInstruction& instruction,
    const std::unordered_map<int, RegisterMapping>& registerMap) {

    std::vector<X86Instruction> instructions;

    auto resultReg = getPhysicalRegister(instruction.getResult()->getId(), registerMap);
    auto operand = convertOperand(instruction.getOperands()[0], registerMap);

    // Movemos el operando al registro resultado
    auto moveInsts = generateMove(createRegisterOperand(resultReg), operand);
    instructions.insert(instructions.end(), moveInsts.begin(), moveInsts.end());

    // Aplicamos la operación unaria
    X86Opcode opcode = (instruction.getOpcode() == ir::IROpcode::Neg) ?
                       X86Opcode::NEG : X86Opcode::NOT;

    X86Instruction opInst(opcode);
    opInst.operands.push_back(createRegisterOperand(resultReg));
    instructions.push_back(opInst);

    return instructions;
}

std::vector<X86Instruction> InstructionSelector::selectLoad(
    const ir::LoadInstruction& instruction,
    const std::unordered_map<int, RegisterMapping>& registerMap) {

    std::vector<X86Instruction> instructions;

    auto resultReg = getPhysicalRegister(instruction.getResult()->getId(), registerMap);
    auto addressOp = convertOperand(instruction.getOperands()[0], registerMap);

    X86Instruction loadInst(X86Opcode::MOV);
    loadInst.operands.push_back(createRegisterOperand(resultReg));
    loadInst.operands.push_back(addressOp);
    instructions.push_back(loadInst);

    return instructions;
}

std::vector<X86Instruction> InstructionSelector::selectStore(
    const ir::StoreInstruction& instruction,
    const std::unordered_map<int, RegisterMapping>& registerMap) {

    std::vector<X86Instruction> instructions;

    auto valueOp = convertOperand(instruction.getOperands()[0], registerMap);
    auto addressOp = convertOperand(instruction.getOperands()[1], registerMap);

    X86Instruction storeInst(X86Opcode::MOV);
    storeInst.operands.push_back(addressOp);
    storeInst.operands.push_back(valueOp);
    instructions.push_back(storeInst);

    return instructions;
}

std::vector<X86Instruction> InstructionSelector::selectBranch(
    const ir::BranchInstruction& instruction) {

    std::vector<X86Instruction> instructions;

    if (instruction.getOpcode() == ir::IROpcode::BrCond) {
        // Branch condicional - asumimos que el primer operando es la condición
        // En un compilador real, esto sería más complejo
        X86Instruction jmpInst(X86Opcode::JNE);
        // Agregar etiqueta de destino (simplificado)
        instructions.push_back(jmpInst);
    } else {
        // Branch incondicional
        X86Instruction jmpInst(X86Opcode::JMP);
        // Agregar etiqueta de destino (simplificado)
        instructions.push_back(jmpInst);
    }

    return instructions;
}

std::vector<X86Instruction> InstructionSelector::selectReturn(
    const ir::ReturnInstruction& instruction,
    const std::unordered_map<int, RegisterMapping>& registerMap) {

    std::vector<X86Instruction> instructions;

    if (!instruction.getOperands().empty()) {
        // Retorno con valor - mover al registro RAX
        auto valueOp = convertOperand(instruction.getOperands()[0], registerMap);
        auto moveInsts = generateMove(createRegisterOperand(X86Register::RAX), valueOp);
        instructions.insert(instructions.end(), moveInsts.begin(), moveInsts.end());
    }

    X86Instruction retInst(X86Opcode::RET);
    instructions.push_back(retInst);

    return instructions;
}

std::vector<X86Instruction> InstructionSelector::selectCall(
    const ir::CallInstruction& instruction,
    const std::unordered_map<int, RegisterMapping>& registerMap) {

    std::vector<X86Instruction> instructions;

    // Configurar argumentos según ABI (simplificado)
    // En un compilador real, esto seguiría las reglas completas del ABI

    // Llamar a la función
    X86Instruction callInst(X86Opcode::CALL);
    // El operando 0 es la función a llamar (simplificado)
    instructions.push_back(callInst);

    // Si hay resultado, mover de RAX
    if (instruction.getResult()) {
        auto resultReg = getPhysicalRegister(instruction.getResult()->getId(), registerMap);
        X86Instruction movInst(X86Opcode::MOV);
        movInst.operands.push_back(createRegisterOperand(resultReg));
        movInst.operands.push_back(createRegisterOperand(X86Register::RAX));
        instructions.push_back(movInst);
    }

    return instructions;
}

X86Operand InstructionSelector::convertOperand(
    const std::shared_ptr<ir::IROperand>& operand,
    const std::unordered_map<int, RegisterMapping>& registerMap) {

    switch (operand->getKind()) {
        case ir::IROperand::Kind::Register: {
            auto regOp = std::static_pointer_cast<ir::Register>(operand);
            X86Register physReg = getPhysicalRegister(regOp->getId(), registerMap);
            return createRegisterOperand(physReg);
        }
        case ir::IROperand::Kind::Immediate: {
            auto immOp = std::static_pointer_cast<ir::Immediate>(operand);
            return createImmediateOperand(std::get<int64_t>(immOp->getValue()));
        }
        case ir::IROperand::Kind::Label: {
            // Manejo simplificado de etiquetas
            X86Operand op(AddressingMode::Immediate);
            return op;
        }
        default:
            return createRegisterOperand(X86Register::RAX); // Placeholder
    }
}

X86Register InstructionSelector::getPhysicalRegister(
    int virtualReg,
    const std::unordered_map<int, RegisterMapping>& registerMap) {

    auto it = registerMap.find(virtualReg);
    if (it != registerMap.end()) {
        return it->second.physicalReg;
    }

    // Fallback - asignar registro por defecto
    return static_cast<X86Register>(virtualReg % 16);
}

X86Operand InstructionSelector::createImmediateOperand(int64_t value) {
    X86Operand op(AddressingMode::Immediate);
    op.immediate = value;
    return op;
}

X86Operand InstructionSelector::createRegisterOperand(X86Register reg) {
    X86Operand op(AddressingMode::Register);
    op.reg = reg;
    return op;
}

X86Operand InstructionSelector::createMemoryOperand(X86Register base, int32_t displacement) {
    X86Operand op(AddressingMode::MemoryBaseDisp);
    op.reg = base;
    op.displacement = displacement;
    return op;
}

std::string InstructionSelector::registerToString(X86Register reg) const {
    static const char* registerNames[] = {
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
        "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp",
        "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d",
        "ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
        "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w",
        "al", "bl", "cl", "dl", "sil", "dil", "bpl", "spl",
        "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b",
        "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
        "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15"
    };

    int index = static_cast<int>(reg);
    if (index >= 0 && index < 72) {
        return registerNames[index];
    }

    return "<unknown>";
}

std::string InstructionSelector::opcodeToString(X86Opcode opcode) const {
    static const char* opcodeNames[] = {
        "mov", "movzx", "movsx", "lea",
        "add", "sub", "imul", "idiv", "inc", "dec", "neg",
        "and", "or", "xor", "not", "shl", "shr", "sar",
        "cmp", "test", "jmp", "je", "jne", "jl", "jle", "jg", "jge",
        "jb", "jbe", "ja", "jae", "js", "jns", "jc", "jnc",
        "call", "ret", "leave", "enter",
        "push", "pop",
        "movss", "movsd", "addss", "addsd", "subss", "subsd",
        "mulss", "mulsd", "divss", "divsd", "comiss", "comisd",
        "movaps", "movups", "addps", "addpd",
        "nop", "hlt",
        "lock", "rep", "repz", "repnz"
    };

    int index = static_cast<int>(opcode);
    if (index >= 0 && index < 60) {
        return opcodeNames[index];
    }

    return "<unknown>";
}

std::vector<X86Instruction> InstructionSelector::generateMove(X86Operand dst, X86Operand src) {
    std::vector<X86Instruction> instructions;

    X86Instruction movInst(X86Opcode::MOV);
    movInst.operands.push_back(dst);
    movInst.operands.push_back(src);
    instructions.push_back(movInst);

    return instructions;
}

void InstructionSelector::optimizeInstructionSequence(std::vector<X86Instruction>& instructions) {
    // Optimizaciones simples:
    // - Eliminar movs redundantes
    // - Fusionar operaciones consecutivas

    // Implementación simplificada - en un compilador real esto sería mucho más sofisticado
    std::vector<X86Instruction> optimized;

    for (const auto& inst : instructions) {
        // Evitar NOPs innecesarios
        if (inst.opcode == X86Opcode::NOP && inst.comment.empty()) {
            continue;
        }
        optimized.push_back(inst);
    }

    instructions = std::move(optimized);
}

} // namespace cpp20::compiler::backend
