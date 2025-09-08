#pragma once

#include <compiler/common/diagnostics/SourceLocation.h>
#include <string>

namespace cpp20::compiler::frontend {

/**
 * @brief Tipos de tokens
 */
enum class TokenKind {
    // Literals
    Identifier,
    Number,
    String,

    // Operators
    Plus,
    Minus,
    Star,
    Slash,
    Equal,

    // Punctuation
    Semicolon,
    LParen,
    RParen,
    LBrace,
    RBrace,

    // Special
    EndOfFile
};

/**
 * @brief Representa un token léxico
 */
class Token {
public:
    Token(TokenKind kind, std::string text, diagnostics::SourceLocation location);

    TokenKind kind() const { return kind_; }
    const std::string& text() const { return text_; }
    const diagnostics::SourceLocation& location() const { return location_; }

    std::string toString() const;

private:
    TokenKind kind_;
    std::string text_;
    diagnostics::SourceLocation location_;
};

} // namespace cpp20::compiler::frontend
