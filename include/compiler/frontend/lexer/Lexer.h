#pragma once

#include <compiler/frontend/lexer/Token.h>
#include <compiler/common/diagnostics/SourceManager.h>
#include <compiler/common/diagnostics/DiagnosticEngine.h>
#include <vector>
#include <string>
#include <memory>

namespace cpp20::compiler::frontend {

/**
 * @brief Analizador léxico (lexer) del compilador
 */
class Lexer {
public:
    Lexer(std::shared_ptr<diagnostics::SourceManager> sourceManager,
          std::shared_ptr<diagnostics::DiagnosticEngine> diagnosticEngine);

    /**
     * @brief Tokeniza el código fuente
     * @param source Código fuente a tokenizar
     * @param fileId ID del archivo fuente
     * @return Vector de tokens
     */
    std::vector<Token> tokenize(const std::string& source, uint32_t fileId);

private:
    std::shared_ptr<diagnostics::SourceManager> sourceManager_;
    std::shared_ptr<diagnostics::DiagnosticEngine> diagnosticEngine_;
};

} // namespace cpp20::compiler::frontend
