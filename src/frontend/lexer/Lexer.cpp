/**
 * @file Lexer.cpp
 * @brief Implementación básica del lexer
 */

#include <compiler/frontend/lexer/Lexer.h>
#include <compiler/common/diagnostics/DiagnosticEngine.h>
#include <cctype>

namespace cpp20::compiler::frontend {

// ========================================================================
// Token implementation
// ========================================================================

Token::Token(TokenKind kind, std::string text, diagnostics::SourceLocation location)
    : kind_(kind), text_(std::move(text)), location_(location) {}

std::string Token::toString() const {
    std::string result;
    switch (kind_) {
        case TokenKind::Identifier: result = "identifier"; break;
        case TokenKind::Number: result = "number"; break;
        case TokenKind::String: result = "string"; break;
        case TokenKind::Plus: result = "+"; break;
        case TokenKind::Minus: result = "-"; break;
        case TokenKind::Star: result = "*"; break;
        case TokenKind::Slash: result = "/"; break;
        case TokenKind::Equal: result = "="; break;
        case TokenKind::Semicolon: result = ";"; break;
        case TokenKind::LParen: result = "("; break;
        case TokenKind::RParen: result = ")"; break;
        case TokenKind::LBrace: result = "{"; break;
        case TokenKind::RBrace: result = "}"; break;
        case TokenKind::EndOfFile: result = "EOF"; break;
        default: result = "unknown"; break;
    }
    return result + " '" + text_ + "'";
}

// ========================================================================
// Lexer implementation
// ========================================================================

Lexer::Lexer(std::shared_ptr<diagnostics::SourceManager> sourceManager,
             std::shared_ptr<diagnostics::DiagnosticEngine> diagnosticEngine)
    : sourceManager_(std::move(sourceManager)),
      diagnosticEngine_(std::move(diagnosticEngine)) {}

std::vector<Token> Lexer::tokenize(const std::string& source, uint32_t fileId) {
    std::vector<Token> tokens;
    size_t pos = 0;
    int line = 1;
    int column = 1;

    while (pos < source.size()) {
        char c = source[pos];

        // Skip whitespace
        if (std::isspace(c)) {
            if (c == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            pos++;
            continue;
        }

        // Create location
        diagnostics::SourceLocation location(line, column, static_cast<uint32_t>(pos), fileId);

        // Single character tokens
        switch (c) {
            case '+':
                tokens.emplace_back(TokenKind::Plus, "+", location);
                pos++; column++;
                continue;
            case '-':
                tokens.emplace_back(TokenKind::Minus, "-", location);
                pos++; column++;
                continue;
            case '*':
                tokens.emplace_back(TokenKind::Star, "*", location);
                pos++; column++;
                continue;
            case '/':
                tokens.emplace_back(TokenKind::Slash, "/", location);
                pos++; column++;
                continue;
            case '=':
                tokens.emplace_back(TokenKind::Equal, "=", location);
                pos++; column++;
                continue;
            case ';':
                tokens.emplace_back(TokenKind::Semicolon, ";", location);
                pos++; column++;
                continue;
            case '(':
                tokens.emplace_back(TokenKind::LParen, "(", location);
                pos++; column++;
                continue;
            case ')':
                tokens.emplace_back(TokenKind::RParen, ")", location);
                pos++; column++;
                continue;
            case '{':
                tokens.emplace_back(TokenKind::LBrace, "{", location);
                pos++; column++;
                continue;
            case '}':
                tokens.emplace_back(TokenKind::RBrace, "}", location);
                pos++; column++;
                continue;
        }

        // Numbers
        if (std::isdigit(c)) {
            std::string number;
            while (pos < source.size() && std::isdigit(source[pos])) {
                number += source[pos];
                pos++; column++;
            }
            tokens.emplace_back(TokenKind::Number, number, location);
            continue;
        }

        // Identifiers
        if (std::isalpha(c) || c == '_') {
            std::string identifier;
            while (pos < source.size() &&
                   (std::isalnum(source[pos]) || source[pos] == '_')) {
                identifier += source[pos];
                pos++; column++;
            }
            tokens.emplace_back(TokenKind::Identifier, identifier, location);
            continue;
        }

        // Unknown character
        diagnosticEngine_->reportError(
            diagnostics::DiagnosticCode::ERR_LEX_INVALID_CHARACTER,
            location,
            std::string("Invalid character: ") + c
        );
        pos++; column++;
    }

    // Add EOF token
    diagnostics::SourceLocation eofLocation(line, column, static_cast<uint32_t>(pos), fileId);
    tokens.emplace_back(TokenKind::EndOfFile, "", eofLocation);

    return tokens;
}

} // namespace cpp20::compiler::frontend
