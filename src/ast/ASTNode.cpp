/**
 * @file ASTNode.cpp
 * @brief Implementación básica del AST
 */

#include <compiler/ast/ASTNode.h>

namespace cpp20::compiler::ast {

// ========================================================================
// ASTNode implementation
// ========================================================================

// Constructor y destructor ya están definidos inline en el header

// ========================================================================
// TranslationUnit implementation
// ========================================================================

// Constructor ya está definido inline en el header

void TranslationUnit::accept(ASTVisitor* visitor) {
    if (visitor) {
        visitor->visit(this);
    }
}

std::string TranslationUnit::toString() const {
    return "TranslationUnit(" + std::to_string(declarations_.size()) + " declarations)";
}

// ========================================================================
// ASTVisitor implementation
// ========================================================================

void ASTVisitor::visitChildren(ASTNode* node) {
    // Base implementation - override in derived classes
    (void)node;
}

void ASTVisitor::visitNode(ASTNode* node) {
    if (node) {
        node->accept(this);
    }
}

// ========================================================================
// ASTVisitor visit methods (placeholders)
// ========================================================================

void ASTVisitor::visit(TranslationUnit* node) {
    // Visit children
    for (const auto& decl : node->declarations()) {
        visitNode(decl.get());
    }
}

} // namespace cpp20::compiler::ast
