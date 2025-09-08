/**
 * @file SourceLocation.cpp
 * @brief Implementación de SourceLocation y SourceRange
 */

#include <compiler/common/diagnostics/SourceLocation.h>
#include <format>

namespace cpp20::compiler::diagnostics {

// SourceLocation implementation
std::string SourceLocation::toString() const {
    if (!isValid()) {
        return "<invalid>";
    }
    return std::format("line {}, column {}", line_, column_);
}

// SourceRange implementation
std::string SourceRange::toString() const {
    if (!isValid()) {
        return "<invalid>";
    }
    return std::format("{} - {}", start_.toString(), end_.toString());
}

} // namespace cpp20::compiler::diagnostics
