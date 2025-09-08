#pragma once

#include "ASTNode.h"
#include <string>
#include <vector>
#include <memory>

namespace cpp20::compiler::ast {

/**
 * @brief Nodo para literales enteros
 */
class IntegerLiteral : public ASTNode {
public:
    explicit IntegerLiteral(int64_t value, diagnostics::SourceLocation location)
        : ASTNode(ASTNodeKind::IntegerLiteral, location), value_(value) {}

    int64_t getValue() const { return value_; }
    void accept(ASTVisitor* visitor) override;
    std::string toString() const override { return std::to_string(value_); }

private:
    int64_t value_;
};

/**
 * @brief Nodo para literales de punto flotante
 */
class FloatingPointLiteral : public ASTNode {
public:
    explicit FloatingPointLiteral(double value, diagnostics::SourceLocation location)
        : ASTNode(ASTNodeKind::FloatingPointLiteral, location), value_(value) {}

    double getValue() const { return value_; }
    void accept(ASTVisitor* visitor) override;
    std::string toString() const override { return std::to_string(value_); }

private:
    double value_;
};

/**
 * @brief Nodo para literales de caracteres
 */
class CharacterLiteral : public ASTNode {
public:
    explicit CharacterLiteral(char value, diagnostics::SourceLocation location)
        : ASTNode(ASTNodeKind::CharacterLiteral, location), value_(value) {}

    char getValue() const { return value_; }
    void accept(ASTVisitor* visitor) override;
    std::string toString() const override { return std::string(1, value_); }

private:
    char value_;
};

/**
 * @brief Nodo para literales de cadenas
 */
class StringLiteral : public ASTNode {
public:
    explicit StringLiteral(std::string value, diagnostics::SourceLocation location)
        : ASTNode(ASTNodeKind::StringLiteral, location), value_(std::move(value)) {}

    const std::string& getValue() const { return value_; }
    void accept(ASTVisitor* visitor) override;
    std::string toString() const override { return "\"" + value_ + "\""; }

private:
    std::string value_;
};

/**
 * @brief Nodo para literales booleanos
 */
class BooleanLiteral : public ASTNode {
public:
    explicit BooleanLiteral(bool value, diagnostics::SourceLocation location)
        : ASTNode(ASTNodeKind::BooleanLiteral, location), value_(value) {}

    bool getValue() const { return value_; }
    void accept(ASTVisitor* visitor) override;
    std::string toString() const override { return value_ ? "true" : "false"; }

private:
    bool value_;
};

/**
 * @brief Nodo para operaciones binarias
 */
class BinaryOp : public ASTNode {
public:
    enum class OpKind {
        Add, Subtract, Multiply, Divide, Modulo,
        Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual,
        LogicalAnd, LogicalOr, BitwiseAnd, BitwiseOr, BitwiseXor
    };

    BinaryOp(std::unique_ptr<ASTNode> left, std::unique_ptr<ASTNode> right,
             OpKind op, diagnostics::SourceLocation location)
        : ASTNode(ASTNodeKind::BinaryOp, location),
          left_(std::move(left)), right_(std::move(right)), op_(op) {}

    const ASTNode* getLeft() const { return left_.get(); }
    const ASTNode* getRight() const { return right_.get(); }
    OpKind getOp() const { return op_; }

    void accept(ASTVisitor* visitor) override;
    std::string toString() const override;

private:
    std::unique_ptr<ASTNode> left_;
    std::unique_ptr<ASTNode> right_;
    OpKind op_;
};

/**
 * @brief Nodo para operaciones unarias
 */
class UnaryOp : public ASTNode {
public:
    enum class OpKind {
        Plus, Minus, Not, BitwiseNot, AddressOf, Dereference
    };

    UnaryOp(std::unique_ptr<ASTNode> operand, OpKind op, diagnostics::SourceLocation location)
        : ASTNode(ASTNodeKind::UnaryOp, location),
          operand_(std::move(operand)), op_(op) {}

    const ASTNode* getOperand() const { return operand_.get(); }
    OpKind getOp() const { return op_; }

    void accept(ASTVisitor* visitor) override;
    std::string toString() const override;

private:
    std::unique_ptr<ASTNode> operand_;
    OpKind op_;
};

/**
 * @brief Nodo para llamadas a función
 */
class FunctionCall : public ASTNode {
public:
    FunctionCall(std::unique_ptr<ASTNode> callee,
                 std::vector<std::unique_ptr<ASTNode>> arguments,
                 diagnostics::SourceLocation location)
        : ASTNode(ASTNodeKind::FunctionCall, location),
          callee_(std::move(callee)), arguments_(std::move(arguments)) {}

    const ASTNode* getCallee() const { return callee_.get(); }
    const std::vector<std::unique_ptr<ASTNode>>& getArguments() const { return arguments_; }

    void accept(ASTVisitor* visitor) override;
    std::string toString() const override;

private:
    std::unique_ptr<ASTNode> callee_;
    std::vector<std::unique_ptr<ASTNode>> arguments_;
};

/**
 * @brief Nodo para operaciones ternarias
 */
class TernaryOp : public ASTNode {
public:
    TernaryOp(std::unique_ptr<ASTNode> condition,
              std::unique_ptr<ASTNode> trueExpr,
              std::unique_ptr<ASTNode> falseExpr,
              diagnostics::SourceLocation location)
        : ASTNode(ASTNodeKind::TernaryOp, location),
          condition_(std::move(condition)),
          trueExpr_(std::move(trueExpr)),
          falseExpr_(std::move(falseExpr)) {}

    const ASTNode* getCondition() const { return condition_.get(); }
    const ASTNode* getTrueExpr() const { return trueExpr_.get(); }
    const ASTNode* getFalseExpr() const { return falseExpr_.get(); }

    void accept(ASTVisitor* visitor) override;
    std::string toString() const override;

private:
    std::unique_ptr<ASTNode> condition_;
    std::unique_ptr<ASTNode> trueExpr_;
    std::unique_ptr<ASTNode> falseExpr_;
};

/**
 * @brief Nodo para asignaciones
 */
class Assignment : public ASTNode {
public:
    enum class OpKind {
        Assign, AddAssign, SubtractAssign, MultiplyAssign, DivideAssign, ModuloAssign,
        BitwiseAndAssign, BitwiseOrAssign, BitwiseXorAssign
    };

    Assignment(std::unique_ptr<ASTNode> left, std::unique_ptr<ASTNode> right,
               OpKind op, diagnostics::SourceLocation location)
        : ASTNode(ASTNodeKind::Assignment, location),
          left_(std::move(left)), right_(std::move(right)), op_(op) {}

    const ASTNode* getLeft() const { return left_.get(); }
    const ASTNode* getRight() const { return right_.get(); }
    OpKind getOp() const { return op_; }

    void accept(ASTVisitor* visitor) override;
    std::string toString() const override;

private:
    std::unique_ptr<ASTNode> left_;
    std::unique_ptr<ASTNode> right_;
    OpKind op_;
};

} // namespace cpp20::compiler::ast
