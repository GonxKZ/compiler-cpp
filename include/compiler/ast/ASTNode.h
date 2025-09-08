#pragma once

#include <compiler/common/diagnostics/SourceLocation.h>
#include <memory>
#include <vector>
#include <string>

namespace cpp20::compiler::ast {

/**
 * @brief Tipos de nodos AST
 */
enum class ASTNodeKind {
    // Expresiones
    Literal,
    Identifier,
    BinaryOp,
    UnaryOp,
    Call,
    MemberAccess,
    ArrayAccess,
    Cast,
    Conditional,
    Lambda,
    New,
    Delete,

    // Declaraciones
    VariableDecl,
    FunctionDecl,
    ClassDecl,
    EnumDecl,
    UsingDecl,
    TypeAliasDecl,

    // Sentencias
    CompoundStmt,
    IfStmt,
    WhileStmt,
    ForStmt,
    ReturnStmt,
    BreakStmt,
    ContinueStmt,
    SwitchStmt,
    CaseStmt,
    DefaultStmt,

    // Otros
    TranslationUnit,
    NamespaceDecl,
    TemplateDecl,
    ConceptDecl,
    RequiresExpr
};

/**
 * @brief Clase base para todos los nodos del AST
 */
class ASTNode {
public:
    ASTNode(ASTNodeKind kind, diagnostics::SourceLocation location)
        : kind_(kind), location_(location) {}
    virtual ~ASTNode() = default;

    // Getters
    ASTNodeKind kind() const { return kind_; }
    const diagnostics::SourceLocation& location() const { return location_; }

    // Utilidades
    virtual void accept(class ASTVisitor* visitor) = 0;
    virtual std::string toString() const = 0;

    // Información de tipos (se establece durante análisis semántico)
    const class types::Type* type() const { return type_; }
    void setType(const class types::Type* type) { type_ = type; }

private:
    ASTNodeKind kind_;
    diagnostics::SourceLocation location_;
    const class types::Type* type_ = nullptr;
};

/**
 * @brief Nodo raíz del AST - Unidad de traducción
 */
class TranslationUnit : public ASTNode {
public:
    TranslationUnit(diagnostics::SourceLocation location)
        : ASTNode(ASTNodeKind::TranslationUnit, location) {}

    void addDeclaration(std::unique_ptr<ASTNode> decl) {
        declarations_.push_back(std::move(decl));
    }

    const std::vector<std::unique_ptr<ASTNode>>& declarations() const {
        return declarations_;
    }

    void accept(class ASTVisitor* visitor) override;
    std::string toString() const override;

private:
    std::vector<std::unique_ptr<ASTNode>> declarations_;
};

/**
 * @brief Visitor pattern para recorrer el AST
 */
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // Visit methods para cada tipo de nodo
    virtual void visit(TranslationUnit* node) = 0;

    // Métodos helper
    virtual void visitChildren(ASTNode* node);
    virtual void visitNode(ASTNode* node);
};

} // namespace cpp20::compiler::ast
