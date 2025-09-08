/**
 * @file ExceptionIR.h
 * @brief Soporte para excepciones en la Representación Intermedia (IR)
 */

#pragma once

#include <compiler/ir/IR.h>
#include <vector>
#include <memory>

namespace cpp20::compiler::ir {

/**
 * @brief Tipos de acciones de exception handling
 */
enum class ExceptionAction {
    None,           // Sin manejo de excepciones
    Cleanup,        // Cleanup (finally-like)
    Catch,          // Catch handler
    Terminate       // Terminate (unexpected exception)
};

/**
 * @brief Información de una región de exception handling
 */
struct ExceptionRegion {
    uint32_t regionId;
    uint32_t startBlock;     // ID del bloque donde comienza la región
    uint32_t endBlock;       // ID del bloque donde termina la región
    uint32_t landingPadBlock; // ID del bloque landing pad
    ExceptionAction action;
    std::string exceptionType; // Tipo de excepción manejada (vacío = catch-all)
    std::vector<std::string> cleanupActions; // Acciones de cleanup

    ExceptionRegion(uint32_t id, uint32_t start, uint32_t end, uint32_t landing,
                   ExceptionAction act, const std::string& type = "")
        : regionId(id), startBlock(start), endBlock(end), landingPadBlock(landing),
          action(act), exceptionType(type) {}
};

/**
 * @brief Información de una instrucción invoke
 */
struct InvokeInfo {
    uint32_t normalBlock;        // Bloque destino si no hay excepción
    uint32_t unwindBlock;        // Bloque destino si hay excepción
    std::shared_ptr<ExceptionRegion> exceptionRegion;

    InvokeInfo(uint32_t normal, uint32_t unwind)
        : normalBlock(normal), unwindBlock(unwind) {}
};

/**
 * @brief Instrucción invoke (llamada que puede lanzar excepciones)
 */
class InvokeInstruction : public IRInstruction {
public:
    InvokeInstruction(std::shared_ptr<IROperand> function,
                     const std::vector<std::shared_ptr<IROperand>>& args,
                     const TypeInfo& resultType,
                     std::shared_ptr<InvokeInfo> invokeInfo)
        : IRInstruction(IROpcode::Call, resultType), invokeInfo_(invokeInfo) {
        addOperand(function);
        for (auto& arg : args) {
            addOperand(arg);
        }
    }

    std::shared_ptr<InvokeInfo> getInvokeInfo() const { return invokeInfo_; }

    std::string toString() const override;

private:
    std::shared_ptr<InvokeInfo> invokeInfo_;
};

/**
 * @brief Instrucción landing pad
 */
class LandingPadInstruction : public IRInstruction {
public:
    LandingPadInstruction(const std::vector<std::string>& catchTypes,
                         const std::vector<std::string>& cleanupActions,
                         const TypeInfo& resultType)
        : IRInstruction(IROpcode::Phi, resultType),
          catchTypes_(catchTypes), cleanupActions_(cleanupActions) {}

    const std::vector<std::string>& getCatchTypes() const { return catchTypes_; }
    const std::vector<std::string>& getCleanupActions() const { return cleanupActions_; }

    std::string toString() const override;

private:
    std::vector<std::string> catchTypes_;
    std::vector<std::string> cleanupActions_;
};

/**
 * @brief Instrucción resume (relanzar excepción)
 */
class ResumeInstruction : public IRInstruction {
public:
    ResumeInstruction(std::shared_ptr<IROperand> exceptionValue)
        : IRInstruction(IROpcode::Ret) {
        if (exceptionValue) {
            addOperand(exceptionValue);
        }
    }

    std::string toString() const override;
};

/**
 * @brief Gestor de excepciones en la IR
 */
class ExceptionHandler {
public:
    /**
     * @brief Constructor
     */
    ExceptionHandler();

    /**
     * @brief Destructor
     */
    ~ExceptionHandler();

    /**
     * @brief Crea una nueva región de exception handling
     */
    std::shared_ptr<ExceptionRegion> createExceptionRegion(
        uint32_t startBlock, uint32_t endBlock,
        ExceptionAction action, const std::string& exceptionType = "");

    /**
     * @brief Añade una región de exception handling
     */
    void addExceptionRegion(std::shared_ptr<ExceptionRegion> region);

    /**
     * @brief Obtiene todas las regiones de exception handling
     */
    const std::vector<std::shared_ptr<ExceptionRegion>>& getExceptionRegions() const {
        return exceptionRegions_;
    }

    /**
     * @brief Crea información de invoke
     */
    std::shared_ptr<InvokeInfo> createInvokeInfo(uint32_t normalBlock, uint32_t unwindBlock);

    /**
     * @brief Verifica si una función tiene exception handling
     */
    bool hasExceptionHandling() const { return !exceptionRegions_.empty(); }

    /**
     * @brief Obtiene el número de regiones de exception handling
     */
    size_t getRegionCount() const { return exceptionRegions_.size(); }

    /**
     * @brief Limpia todas las regiones de exception handling
     */
    void clear();

private:
    std::vector<std::shared_ptr<ExceptionRegion>> exceptionRegions_;
    uint32_t nextRegionId_ = 1;
};

/**
 * @brief Constructor de excepciones para la IR
 */
class ExceptionIRBuilder {
public:
    /**
     * @brief Constructor
     */
    ExceptionIRBuilder(IRBuilder& irBuilder, ExceptionHandler& exceptionHandler);

    /**
     * @brief Crea una instrucción invoke
     */
    std::unique_ptr<InvokeInstruction> createInvoke(
        std::shared_ptr<IROperand> function,
        const std::vector<std::shared_ptr<IROperand>>& args,
        const TypeInfo& resultType,
        uint32_t normalBlock,
        uint32_t unwindBlock);

    /**
     * @brief Crea una instrucción landing pad
     */
    std::unique_ptr<LandingPadInstruction> createLandingPad(
        const std::vector<std::string>& catchTypes,
        const std::vector<std::string>& cleanupActions,
        const TypeInfo& resultType);

    /**
     * @brief Crea una instrucción resume
     */
    std::unique_ptr<ResumeInstruction> createResume(std::shared_ptr<IROperand> exceptionValue);

    /**
     * @brief Crea bloques para exception handling
     */
    std::unique_ptr<BasicBlock> createLandingPadBlock(uint32_t blockId);

    /**
     * @brief Crea bloque de cleanup
     */
    std::unique_ptr<BasicBlock> createCleanupBlock(uint32_t blockId);

private:
    IRBuilder& irBuilder_;
    ExceptionHandler& exceptionHandler_;
};

} // namespace cpp20::compiler::ir
