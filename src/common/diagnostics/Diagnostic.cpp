/**
 * @file Diagnostic.cpp
 * @brief Implementación de la clase Diagnostic
 */

#include <compiler/common/diagnostics/Diagnostic.h>

namespace cpp20::compiler::diagnostics {

// Diagnostic implementation
Diagnostic::Diagnostic(
    DiagnosticLevel level,
    DiagnosticCode code,
    SourceLocation location,
    std::string message
)
    : level_(level), code_(code), location_(location), message_(std::move(message)) {
}

std::string Diagnostic::formatMessage() const {
    // TODO: Implement proper message formatting with arguments
    return message_;
}

DiagnosticCategory Diagnostic::category() const {
    // Map diagnostic codes to categories
    auto codeValue = static_cast<int>(code_);

    if (codeValue >= 1000 && codeValue < 2000) return DiagnosticCategory::Lexical;
    if (codeValue >= 2000 && codeValue < 3000) return DiagnosticCategory::Syntactic;
    if (codeValue >= 3000 && codeValue < 4000) return DiagnosticCategory::Semantic;
    if (codeValue >= 4000 && codeValue < 5000) return DiagnosticCategory::Template;
    if (codeValue >= 5000 && codeValue < 6000) return DiagnosticCategory::Constexpr;
    if (codeValue >= 6000 && codeValue < 7000) return DiagnosticCategory::Optimization;
    if (codeValue >= 7000 && codeValue < 8000) return DiagnosticCategory::Deprecated;

    return DiagnosticCategory::Semantic; // Default
}

// Factory methods
Diagnostic Diagnostic::error(DiagnosticCode code, SourceLocation loc, std::string msg) {
    return Diagnostic(DiagnosticLevel::Error, code, loc, std::move(msg));
}

Diagnostic Diagnostic::warning(DiagnosticCode code, SourceLocation loc, std::string msg) {
    return Diagnostic(DiagnosticLevel::Warning, code, loc, std::move(msg));
}

Diagnostic Diagnostic::note(DiagnosticCode code, SourceLocation loc, std::string msg) {
    return Diagnostic(DiagnosticLevel::Note, code, loc, std::move(msg));
}

Diagnostic Diagnostic::fatal(DiagnosticCode code, SourceLocation loc, std::string msg) {
    return Diagnostic(DiagnosticLevel::Fatal, code, loc, std::move(msg));
}

} // namespace cpp20::compiler::diagnostics
