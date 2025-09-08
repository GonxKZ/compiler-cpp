/**
 * @file ExceptionIR.cpp
 * @brief Implementación del soporte para excepciones en la IR
 */

#include <compiler/ir/ExceptionIR.h>
#include <sstream>

namespace cpp20::compiler::ir {

// ============================================================================
// InvokeInstruction - Implementación
// ============================================================================

std::string InvokeInstruction::toString() const {
    std::stringstream ss;

    if (result_) {
        ss << result_->toString() << " = ";
    }

    ss << "invoke " << operands_[0]->toString() << "(";

    for (size_t i = 1; i < operands_.size(); ++i) {
        if (i > 1) ss << ", ";
        ss << operands_[i]->toString();
    }

    ss << ") to label %" << invokeInfo_->normalBlock
       << " unwind label %" << invokeInfo_->unwindBlock;

    return ss.str();
}

// ============================================================================
// LandingPadInstruction - Implementación
// ============================================================================

std::string LandingPadInstruction::toString() const {
    std::stringstream ss;

    if (result_) {
        ss << result_->toString() << " = ";
    }

    ss << "landingpad ";

    if (!catchTypes_.empty()) {
        ss << "catch ";
        for (size_t i = 0; i < catchTypes_.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << catchTypes_[i];
        }
    }

    if (!cleanupActions_.empty()) {
        if (!catchTypes_.empty()) ss << " ";
        ss << "cleanup";
        for (const auto& action : cleanupActions_) {
            ss << " [" << action << "]";
        }
    }

    return ss.str();
}

// ============================================================================
// ResumeInstruction - Implementación
// ============================================================================

std::string ResumeInstruction::toString() const {
    std::stringstream ss;
    ss << "resume";

    if (!operands_.empty()) {
        ss << " " << operands_[0]->toString();
    }

    return ss.str();
}

// ============================================================================
// ExceptionHandler - Implementación
// ============================================================================

ExceptionHandler::ExceptionHandler() = default;

ExceptionHandler::~ExceptionHandler() = default;

std::shared_ptr<ExceptionRegion> ExceptionHandler::createExceptionRegion(
    uint32_t startBlock, uint32_t endBlock,
    ExceptionAction action, const std::string& exceptionType) {

    auto region = std::make_shared<ExceptionRegion>(
        nextRegionId_++, startBlock, endBlock, 0, action, exceptionType);

    addExceptionRegion(region);
    return region;
}

void ExceptionHandler::addExceptionRegion(std::shared_ptr<ExceptionRegion> region) {
    exceptionRegions_.push_back(region);
}

std::shared_ptr<InvokeInfo> ExceptionHandler::createInvokeInfo(uint32_t normalBlock, uint32_t unwindBlock) {
    return std::make_shared<InvokeInfo>(normalBlock, unwindBlock);
}

void ExceptionHandler::clear() {
    exceptionRegions_.clear();
    nextRegionId_ = 1;
}

// ============================================================================
// ExceptionIRBuilder - Implementación
// ============================================================================

ExceptionIRBuilder::ExceptionIRBuilder(IRBuilder& irBuilder, ExceptionHandler& exceptionHandler)
    : irBuilder_(irBuilder), exceptionHandler_(exceptionHandler) {
}

std::unique_ptr<InvokeInstruction> ExceptionIRBuilder::createInvoke(
    std::shared_ptr<IROperand> function,
    const std::vector<std::shared_ptr<IROperand>>& args,
    const TypeInfo& resultType,
    uint32_t normalBlock,
    uint32_t unwindBlock) {

    // Crear información de invoke
    auto invokeInfo = exceptionHandler_.createInvokeInfo(normalBlock, unwindBlock);

    // Crear la instrucción invoke
    auto invokeInst = std::make_unique<InvokeInstruction>(
        function, args, resultType, invokeInfo);

    // Establecer resultado si hay tipo de retorno
    if (resultType.type != IRType::Void) {
        invokeInst->setResult(irBuilder_.createRegister(resultType));
    }

    return invokeInst;
}

std::unique_ptr<LandingPadInstruction> ExceptionIRBuilder::createLandingPad(
    const std::vector<std::string>& catchTypes,
    const std::vector<std::string>& cleanupActions,
    const TypeInfo& resultType) {

    auto landingPadInst = std::make_unique<LandingPadInstruction>(
        catchTypes, cleanupActions, resultType);

    // Establecer resultado
    landingPadInst->setResult(irBuilder_.createRegister(resultType));

    return landingPadInst;
}

std::unique_ptr<ResumeInstruction> ExceptionIRBuilder::createResume(std::shared_ptr<IROperand> exceptionValue) {
    return std::make_unique<ResumeInstruction>(exceptionValue);
}

std::unique_ptr<BasicBlock> ExceptionIRBuilder::createLandingPadBlock(uint32_t blockId) {
    auto block = irBuilder_.createBasicBlock("landingpad_" + std::to_string(blockId));

    // Crear landing pad instruction
    std::vector<std::string> catchTypes = {"..."}; // catch-all
    std::vector<std::string> cleanupActions = {"cleanup_stack", "destroy_locals"};

    TypeInfo exceptionType(IRType::Pointer, 8, 8, "std::exception_ptr");
    auto landingPadInst = createLandingPad(catchTypes, cleanupActions, exceptionType);

    block->addInstruction(std::move(landingPadInst));

    return block;
}

std::unique_ptr<BasicBlock> ExceptionIRBuilder::createCleanupBlock(uint32_t blockId) {
    auto block = irBuilder_.createBasicBlock("cleanup_" + std::to_string(blockId));

    // Crear instrucciones de cleanup (simplificado)
    // En un compilador real, esto incluiría llamadas a destructores,
    // liberación de recursos, etc.

    TypeInfo voidType(IRType::Void);
    auto resumeInst = createResume(nullptr);
    block->addInstruction(std::move(resumeInst));

    return block;
}

} // namespace cpp20::compiler::ir
