/**
 * @file TemplateAST.h
 * @brief Nodos AST para templates y concepts C++20
 */

#pragma once

#include <compiler/ast/ASTNode.h>
#include <vector>
#include <memory>
#include <string>

namespace cpp20::compiler::ast {

/**
 * @brief Tipos de parámetros template
 */
enum class TemplateParameterType {
    Type,           // typename/class T
    NonType,        // int N
    Template        // template<typename> class Container
};

/**
 * @brief Parámetro template
 */
class TemplateParameter : public ASTNode {
public:
    TemplateParameter(TemplateParameterType type, const std::string& name,
                     std::unique_ptr<ASTNode> defaultValue = nullptr);

    TemplateParameterType getParameterType() const { return parameterType_; }
    const std::string& getName() const { return name_; }
    ASTNode* getDefaultValue() const { return defaultValue_.get(); }

    void accept(ASTVisitor& visitor) override;

private:
    TemplateParameterType parameterType_;
    std::string name_;
    std::unique_ptr<ASTNode> defaultValue_;
};

/**
 * @brief Lista de parámetros template
 */
class TemplateParameterList : public ASTNode {
public:
    explicit TemplateParameterList(std::vector<std::unique_ptr<TemplateParameter>> parameters);

    const std::vector<std::unique_ptr<TemplateParameter>>& getParameters() const { return parameters_; }
    void addParameter(std::unique_ptr<TemplateParameter> param);

    void accept(ASTVisitor& visitor) override;

private:
    std::vector<std::unique_ptr<TemplateParameter>> parameters_;
};

/**
 * @brief Declaración template
 */
class TemplateDeclaration : public ASTNode {
public:
    TemplateDeclaration(std::unique_ptr<TemplateParameterList> parameters,
                       std::unique_ptr<ASTNode> declaration);

    const TemplateParameterList* getParameters() const { return parameters_.get(); }
    ASTNode* getDeclaration() const { return declaration_.get(); }

    void accept(ASTVisitor& visitor) override;

private:
    std::unique_ptr<TemplateParameterList> parameters_;
    std::unique_ptr<ASTNode> declaration_;
};

/**
 * @brief Argumento template
 */
class TemplateArgument : public ASTNode {
public:
    enum class ArgumentType {
        Type,       // typename T
        Expression, // valor constexpr
        Template    // template<template<typename> class>
    };

    TemplateArgument(ArgumentType type, std::unique_ptr<ASTNode> value);

    ArgumentType getArgumentType() const { return argumentType_; }
    ASTNode* getValue() const { return value_.get(); }

    void accept(ASTVisitor& visitor) override;

private:
    ArgumentType argumentType_;
    std::unique_ptr<ASTNode> value_;
};

/**
 * @brief Lista de argumentos template
 */
class TemplateArgumentList : public ASTNode {
public:
    explicit TemplateArgumentList(std::vector<std::unique_ptr<TemplateArgument>> arguments);

    const std::vector<std::unique_ptr<TemplateArgument>>& getArguments() const { return arguments_; }
    void addArgument(std::unique_ptr<TemplateArgument> arg);

    void accept(ASTVisitor& visitor) override;

private:
    std::vector<std::unique_ptr<TemplateArgument>> arguments_;
};

/**
 * @brief Instanciación template
 */
class TemplateInstantiation : public ASTNode {
public:
    TemplateInstantiation(std::unique_ptr<ASTNode> templateName,
                         std::unique_ptr<TemplateArgumentList> arguments);

    ASTNode* getTemplateName() const { return templateName_.get(); }
    const TemplateArgumentList* getArguments() const { return arguments_.get(); }

    void accept(ASTVisitor& visitor) override;

private:
    std::unique_ptr<ASTNode> templateName_;
    std::unique_ptr<TemplateArgumentList> arguments_;
};

/**
 * @brief Especailización template
 */
class TemplateSpecialization : public ASTNode {
public:
    TemplateSpecialization(std::unique_ptr<ASTNode> templateName,
                          std::unique_ptr<TemplateArgumentList> arguments,
                          std::unique_ptr<ASTNode> body);

    ASTNode* getTemplateName() const { return templateName_.get(); }
    const TemplateArgumentList* getArguments() const { return arguments_.get(); }
    ASTNode* getBody() const { return body_.get(); }

    void accept(ASTVisitor& visitor) override;

private:
    std::unique_ptr<ASTNode> templateName_;
    std::unique_ptr<TemplateArgumentList> arguments_;
    std::unique_ptr<ASTNode> body_;
};

/**
 * @brief Concepto C++20
 */
class ConceptDefinition : public ASTNode {
public:
    ConceptDefinition(const std::string& name,
                     std::unique_ptr<TemplateParameterList> parameters,
                     std::unique_ptr<ASTNode> constraintExpression);

    const std::string& getName() const { return name_; }
    const TemplateParameterList* getParameters() const { return parameters_.get(); }
    ASTNode* getConstraintExpression() const { return constraintExpression_.get(); }

    void accept(ASTVisitor& visitor) override;

private:
    std::string name_;
    std::unique_ptr<TemplateParameterList> parameters_;
    std::unique_ptr<ASTNode> constraintExpression_;
};

/**
 * @brief Requires clause C++20
 */
class RequiresClause : public ASTNode {
public:
    explicit RequiresClause(std::unique_ptr<ASTNode> requirements);

    ASTNode* getRequirements() const { return requirements_.get(); }

    void accept(ASTVisitor& visitor) override;

private:
    std::unique_ptr<ASTNode> requirements_;
};

/**
 * @brief Requires expression C++20
 */
class RequiresExpression : public ASTNode {
public:
    RequiresExpression(std::unique_ptr<TemplateParameterList> parameters,
                      std::unique_ptr<ASTNode> requirements);

    const TemplateParameterList* getParameters() const { return parameters_.get(); }
    ASTNode* getRequirements() const { return requirements_.get(); }

    void accept(ASTVisitor& visitor) override;

private:
    std::unique_ptr<TemplateParameterList> parameters_;
    std::unique_ptr<ASTNode> requirements_;
};

/**
 * @brief Constraint C++20
 */
class ConstraintExpression : public ASTNode {
public:
    enum class ConstraintType {
        Atomic,         // Concept<T>
        Conjunction,    // A && B
        Disjunction,    // A || B
        LogicalAnd,     // A && B
        LogicalOr,      // A || B
        LogicalNot      // !A
    };

    ConstraintExpression(ConstraintType type,
                        std::unique_ptr<ASTNode> left,
                        std::unique_ptr<ASTNode> right = nullptr);

    ConstraintType getConstraintType() const { return constraintType_; }
    ASTNode* getLeft() const { return left_.get(); }
    ASTNode* getRight() const { return right_.get(); }

    void accept(ASTVisitor& visitor) override;

private:
    ConstraintType constraintType_;
    std::unique_ptr<ASTNode> left_;
    std::unique_ptr<ASTNode> right_;
};

} // namespace cpp20::compiler::ast
