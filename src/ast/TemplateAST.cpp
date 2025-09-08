/**
 * @file TemplateAST.cpp
 * @brief Implementación de nodos AST para templates
 */

#include <compiler/ast/TemplateAST.h>

namespace cpp20::compiler::ast {

// ============================================================================
// TemplateParameter - Implementación
// ============================================================================

TemplateParameter::TemplateParameter(TemplateParameterType type, const std::string& name,
                                   std::unique_ptr<ASTNode> defaultValue)
    : ASTNode(ASTNodeKind::TemplateParameter), parameterType_(type), name_(name),
      defaultValue_(std::move(defaultValue)) {
}

void TemplateParameter::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// ============================================================================
// TemplateParameterList - Implementación
// ============================================================================

TemplateParameterList::TemplateParameterList(std::vector<std::unique_ptr<TemplateParameter>> parameters)
    : ASTNode(ASTNodeKind::TemplateParameterList), parameters_(std::move(parameters)) {
}

void TemplateParameterList::addParameter(std::unique_ptr<TemplateParameter> param) {
    parameters_.push_back(std::move(param));
}

void TemplateParameterList::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// ============================================================================
// TemplateDeclaration - Implementación
// ============================================================================

TemplateDeclaration::TemplateDeclaration(std::unique_ptr<TemplateParameterList> parameters,
                                       std::unique_ptr<ASTNode> declaration)
    : ASTNode(ASTNodeKind::TemplateDeclaration), parameters_(std::move(parameters)),
      declaration_(std::move(declaration)) {
}

void TemplateDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// ============================================================================
// TemplateArgument - Implementación
// ============================================================================

TemplateArgument::TemplateArgument(ArgumentType type, std::unique_ptr<ASTNode> value)
    : ASTNode(ASTNodeKind::TemplateArgument), argumentType_(type), value_(std::move(value)) {
}

void TemplateArgument::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// ============================================================================
// TemplateArgumentList - Implementación
// ============================================================================

TemplateArgumentList::TemplateArgumentList(std::vector<std::unique_ptr<TemplateArgument>> arguments)
    : ASTNode(ASTNodeKind::TemplateArgumentList), arguments_(std::move(arguments)) {
}

void TemplateArgumentList::addArgument(std::unique_ptr<TemplateArgument> arg) {
    arguments_.push_back(std::move(arg));
}

void TemplateArgumentList::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// ============================================================================
// TemplateInstantiation - Implementación
// ============================================================================

TemplateInstantiation::TemplateInstantiation(std::unique_ptr<ASTNode> templateName,
                                           std::unique_ptr<TemplateArgumentList> arguments)
    : ASTNode(ASTNodeKind::TemplateInstantiation), templateName_(std::move(templateName)),
      arguments_(std::move(arguments)) {
}

void TemplateInstantiation::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// ============================================================================
// TemplateSpecialization - Implementación
// ============================================================================

TemplateSpecialization::TemplateSpecialization(std::unique_ptr<ASTNode> templateName,
                                             std::unique_ptr<TemplateArgumentList> arguments,
                                             std::unique_ptr<ASTNode> body)
    : ASTNode(ASTNodeKind::TemplateSpecialization), templateName_(std::move(templateName)),
      arguments_(std::move(arguments)), body_(std::move(body)) {
}

void TemplateSpecialization::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// ============================================================================
// ConceptDefinition - Implementación
// ============================================================================

ConceptDefinition::ConceptDefinition(const std::string& name,
                                   std::unique_ptr<TemplateParameterList> parameters,
                                   std::unique_ptr<ASTNode> constraintExpression)
    : ASTNode(ASTNodeKind::ConceptDefinition), name_(name),
      parameters_(std::move(parameters)), constraintExpression_(std::move(constraintExpression)) {
}

void ConceptDefinition::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// ============================================================================
// RequiresClause - Implementación
// ============================================================================

RequiresClause::RequiresClause(std::unique_ptr<ASTNode> requirements)
    : ASTNode(ASTNodeKind::RequiresClause), requirements_(std::move(requirements)) {
}

void RequiresClause::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// ============================================================================
// RequiresExpression - Implementación
// ============================================================================

RequiresExpression::RequiresExpression(std::unique_ptr<TemplateParameterList> parameters,
                                     std::unique_ptr<ASTNode> requirements)
    : ASTNode(ASTNodeKind::RequiresExpression), parameters_(std::move(parameters)),
      requirements_(std::move(requirements)) {
}

void RequiresExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// ============================================================================
// ConstraintExpression - Implementación
// ============================================================================

ConstraintExpression::ConstraintExpression(ConstraintType type,
                                         std::unique_ptr<ASTNode> left,
                                         std::unique_ptr<ASTNode> right)
    : ASTNode(ASTNodeKind::ConstraintExpression), constraintType_(type),
      left_(std::move(left)), right_(std::move(right)) {
}

void ConstraintExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}
