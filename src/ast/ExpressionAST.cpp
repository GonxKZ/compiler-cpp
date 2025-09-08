#include <compiler/ast/ExpressionAST.h>
#include <sstream>

namespace cpp20::compiler::ast {

// ============================================================================
// IntegerLiteral Implementation
// ============================================================================

void IntegerLiteral::accept(ASTVisitor* visitor) {
    // Implementación básica - en un compilador real esto visitaría el nodo
    if (visitor) {
        // visitor->visitIntegerLiteral(this);
    }
}

// ============================================================================
// FloatingPointLiteral Implementation
// ============================================================================

void FloatingPointLiteral::accept(ASTVisitor* visitor) {
    // Implementación básica - en un compilador real esto visitaría el nodo
    if (visitor) {
        // visitor->visitFloatingPointLiteral(this);
    }
}

// ============================================================================
// CharacterLiteral Implementation
// ============================================================================

void CharacterLiteral::accept(ASTVisitor* visitor) {
    // Implementación básica - en un compilador real esto visitaría el nodo
    if (visitor) {
        // visitor->visitCharacterLiteral(this);
    }
}

// ============================================================================
// StringLiteral Implementation
// ============================================================================

void StringLiteral::accept(ASTVisitor* visitor) {
    // Implementación básica - en un compilador real esto visitaría el nodo
    if (visitor) {
        // visitor->visitStringLiteral(this);
    }
}

// ============================================================================
// BooleanLiteral Implementation
// ============================================================================

void BooleanLiteral::accept(ASTVisitor* visitor) {
    // Implementación básica - en un compilador real esto visitaría el nodo
    if (visitor) {
        // visitor->visitBooleanLiteral(this);
    }
}

// ============================================================================
// BinaryOp Implementation
// ============================================================================

void BinaryOp::accept(ASTVisitor* visitor) {
    // Implementación básica - en un compilador real esto visitaría el nodo
    if (visitor) {
        // visitor->visitBinaryOp(this);
    }
}

// ============================================================================
// UnaryOp Implementation
// ============================================================================

void UnaryOp::accept(ASTVisitor* visitor) {
    // Implementación básica - en un compilador real esto visitaría el nodo
    if (visitor) {
        // visitor->visitUnaryOp(this);
    }
}

// ============================================================================
// FunctionCall Implementation
// ============================================================================

void FunctionCall::accept(ASTVisitor* visitor) {
    // Implementación básica - en un compilador real esto visitaría el nodo
    if (visitor) {
        // visitor->visitFunctionCall(this);
    }
}

// ============================================================================
// TernaryOp Implementation
// ============================================================================

void TernaryOp::accept(ASTVisitor* visitor) {
    // Implementación básica - en un compilador real esto visitaría el nodo
    if (visitor) {
        // visitor->visitTernaryOp(this);
    }
}

// ============================================================================
// Assignment Implementation
// ============================================================================

void Assignment::accept(ASTVisitor* visitor) {
    // Implementación básica - en un compilador real esto visitaría el nodo
    if (visitor) {
        // visitor->visitAssignment(this);
    }
}

} // namespace cpp20::compiler::ast
