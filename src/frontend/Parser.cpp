/**
 * @file Parser.cpp
 * @brief Implementación del Parser C++20
 */

#include <compiler/frontend/Parser.h>
#include <algorithm>

namespace cpp20::compiler::frontend {

// ============================================================================
// Parser - Implementación
// ============================================================================

Parser::Parser(const std::vector<lexer::Token>& tokens,
               diagnostics::DiagnosticEngine& diagEngine,
               const ParserConfig& config)
    : tokens_(tokens), diagEngine_(diagEngine), config_(config), stats_() {
}

Parser::~Parser() = default;

std::unique_ptr<ast::TranslationUnit> Parser::parse() {
    success_ = true;
    currentTokenIndex_ = 0;

    auto translationUnit = parseTranslationUnit();

    if (!isAtEnd()) {
        reportError("tokens inesperados al final del archivo", currentLocation());
        success_ = false;
    }

    return translationUnit;
}

const lexer::Token& Parser::currentToken() const {
    if (currentTokenIndex_ >= tokens_.size()) {
        static lexer::Token eof(lexer::TokenType::END_OF_FILE, "",
                               diagnostics::SourceLocation("", 0, 0));
        return eof;
    }
    return tokens_[currentTokenIndex_];
}

const lexer::Token& Parser::peekToken(size_t offset) const {
    size_t index = currentTokenIndex_ + offset;
    if (index >= tokens_.size()) {
        static lexer::Token eof(lexer::TokenType::END_OF_FILE, "",
                               diagnostics::SourceLocation("", 0, 0));
        return eof;
    }
    return tokens_[index];
}

const lexer::Token& Parser::consumeToken() {
    const lexer::Token& token = currentToken();
    if (currentTokenIndex_ < tokens_.size()) {
        ++currentTokenIndex_;
        ++stats_.tokensConsumed;
    }
    return token;
}

bool Parser::checkToken(lexer::TokenType type) const {
    return currentToken().getType() == type;
}

bool Parser::matchToken(lexer::TokenType type) {
    if (checkToken(type)) {
        consumeToken();
        return true;
    }
    return false;
}

bool Parser::isAtEnd() const {
    return currentTokenIndex_ >= tokens_.size() ||
           currentToken().getType() == lexer::TokenType::END_OF_FILE;
}

void Parser::reportError(const std::string& message, const diagnostics::SourceLocation& location) {
    std::cerr << "Error de parsing en " << location.toString() << ": " << message << std::endl;
    success_ = false;
    ++stats_.errorsReported;
}

void Parser::recoverFromError() {
    // Recuperación simple: saltar hasta punto de sincronización
    while (!isAtEnd()) {
        lexer::TokenType type = currentToken().getType();
        if (type == lexer::TokenType::SEMICOLON ||
            type == lexer::TokenType::RIGHT_BRACE) {
            consumeToken();
            break;
        }
        consumeToken();
    }
    ++stats_.errorRecoveries;
}

// === PARSING DE UNIDADES DE TRADUCCIÓN ===

std::unique_ptr<ast::TranslationUnit> Parser::parseTranslationUnit() {
    auto translationUnit = std::make_unique<ast::TranslationUnit>();
    ++stats_.nodesCreated;

    while (!isAtEnd()) {
        try {
            auto declaration = parseExternalDeclaration();
            if (declaration) {
                // En un AST real, añadiríamos la declaración a la unidad de traducción
                ++stats_.nodesCreated;
            }
        } catch (const std::exception& e) {
            reportError(std::string("error durante parsing: ") + e.what(), currentLocation());
            if (config_.enableErrorRecovery) {
                recoverFromError();
            } else {
                break;
            }
        }
    }

    return translationUnit;
}

std::unique_ptr<ast::ASTNode> Parser::parseExternalDeclaration() {
    // Simplificado: solo declaraciones básicas
    if (ParserUtils::canStartDeclaration(currentToken())) {
        return parseDeclaration();
    }

    // Si no es una declaración, asumir expresión
    return parseExpression();
}

// === PARSING DE DECLARACIONES ===

std::unique_ptr<ast::ASTNode> Parser::parseDeclaration() {
    // Simplificado: asumir declaración de variable o función
    if (peekToken(1).getType() == lexer::TokenType::LEFT_PAREN ||
        peekToken(2).getType() == lexer::TokenType::LEFT_PAREN) {
        return parseFunctionDeclaration();
    } else {
        return parseVariableDeclaration();
    }
}

std::unique_ptr<ast::ASTNode> Parser::parseFunctionDeclaration() {
    // Parsear tipo de retorno
    auto typeSpecifiers = parseTypeSpecifiers();
    if (typeSpecifiers.empty()) {
        reportError("se esperaba especificador de tipo", currentLocation());
        return nullptr;
    }

    // Parsear nombre de función
    if (!checkToken(lexer::TokenType::IDENTIFIER)) {
        reportError("se esperaba nombre de función", currentLocation());
        return nullptr;
    }
    std::string functionName = consumeToken().getLexeme();

    // Parsear parámetros
    if (!matchToken(lexer::TokenType::LEFT_PAREN)) {
        reportError("se esperaba '(' en declaración de función", currentLocation());
        return nullptr;
    }

    auto parameters = parseParameterList();

    if (!matchToken(lexer::TokenType::RIGHT_PAREN)) {
        reportError("se esperaba ')' en declaración de función", currentLocation());
        return nullptr;
    }

    // Parsear cuerpo (opcional para declaraciones)
    std::unique_ptr<ast::ASTNode> body = nullptr;
    if (checkToken(lexer::TokenType::LEFT_BRACE)) {
        body = parseCompoundStatement();
    } else if (!matchToken(lexer::TokenType::SEMICOLON)) {
        reportError("se esperaba ';' o '{' en declaración de función", currentLocation());
    }

    ++stats_.nodesCreated;
    return std::make_unique<ast::ASTNode>(ast::ASTNodeKind::FunctionDecl);
}

std::unique_ptr<ast::ASTNode> Parser::parseVariableDeclaration() {
    // Parsear especificadores de tipo
    auto typeSpecifiers = parseTypeSpecifiers();
    if (typeSpecifiers.empty()) {
        reportError("se esperaba especificador de tipo", currentLocation());
        return nullptr;
    }

    // Parsear declarador
    auto declarator = parseDeclarator();
    if (!declarator) {
        reportError("se esperaba declarador", currentLocation());
        return nullptr;
    }

    // Parsear inicializador opcional
    if (matchToken(lexer::TokenType::ASSIGN)) {
        auto initializer = parseAssignmentExpression();
        if (!initializer) {
            reportError("se esperaba expresión de inicialización", currentLocation());
        }
    }

    if (!matchToken(lexer::TokenType::SEMICOLON)) {
        reportError("se esperaba ';'", currentLocation());
    }

    ++stats_.nodesCreated;
    return std::make_unique<ast::ASTNode>(ast::ASTNodeKind::VariableDecl);
}

std::vector<std::string> Parser::parseTypeSpecifiers() {
    std::vector<std::string> specifiers;

    while (ParserUtils::isTypeKeyword(currentToken().getLexeme()) ||
           checkToken(lexer::TokenType::CONST) ||
           checkToken(lexer::TokenType::VOLATILE)) {
        specifiers.push_back(consumeToken().getLexeme());
    }

    return specifiers;
}

std::unique_ptr<ast::ASTNode> Parser::parseDeclarator() {
    if (!checkToken(lexer::TokenType::IDENTIFIER)) {
        return nullptr;
    }

    consumeToken(); // Consumir identificador
    ++stats_.nodesCreated;
    return std::make_unique<ast::ASTNode>(ast::ASTNodeKind::Identifier);
}

std::vector<std::unique_ptr<ast::ASTNode>> Parser::parseParameterList() {
    std::vector<std::unique_ptr<ast::ASTNode>> parameters;

    if (checkToken(lexer::TokenType::RIGHT_PAREN)) {
        return parameters; // Lista vacía
    }

    while (true) {
        // Parsear tipo de parámetro
        auto typeSpecifiers = parseTypeSpecifiers();
        if (typeSpecifiers.empty()) {
            reportError("se esperaba tipo de parámetro", currentLocation());
            break;
        }

        // Parsear nombre de parámetro (opcional)
        if (checkToken(lexer::TokenType::IDENTIFIER)) {
            consumeToken();
        }

        ++stats_.nodesCreated;
        parameters.push_back(std::make_unique<ast::ASTNode>(ast::ASTNodeKind::ParameterDecl));

        if (!matchToken(lexer::TokenType::COMMA)) {
            break;
        }
    }

    return parameters;
}

// === PARSING DE EXPRESIONES ===

std::unique_ptr<ast::ASTNode> Parser::parseExpression() {
    return parseAssignmentExpression();
}

std::unique_ptr<ast::ASTNode> Parser::parseAssignmentExpression() {
    auto left = parseConditionalExpression();

    if (ParserUtils::isAssignmentOperator(currentToken().getType())) {
        lexer::TokenType op = consumeToken().getType();
        auto right = parseAssignmentExpression();

        ++stats_.nodesCreated;
        return std::make_unique<ast::ASTNode>(ast::ASTNodeKind::BinaryOperator);
    }

    return left;
}

std::unique_ptr<ast::ASTNode> Parser::parseConditionalExpression() {
    auto condition = parseLogicalOrExpression();

    if (matchToken(lexer::TokenType::QUESTION)) {
        auto trueExpr = parseExpression();
        if (!matchToken(lexer::TokenType::COLON)) {
            reportError("se esperaba ':' en expresión condicional", currentLocation());
            return nullptr;
        }
        auto falseExpr = parseConditionalExpression();

        ++stats_.nodesCreated;
        return std::make_unique<ast::ASTNode>(ast::ASTNodeKind::ConditionalOperator);
    }

    return condition;
}

std::unique_ptr<ast::ASTNode> Parser::parseLogicalOrExpression() {
    auto left = parseLogicalAndExpression();

    while (matchToken(lexer::TokenType::LOGICAL_OR)) {
        auto right = parseLogicalAndExpression();
        ++stats_.nodesCreated;
        left = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::BinaryOperator);
    }

    return left;
}

std::unique_ptr<ast::ASTNode> Parser::parseLogicalAndExpression() {
    auto left = parseBitwiseOrExpression();

    while (matchToken(lexer::TokenType::LOGICAL_AND)) {
        auto right = parseBitwiseOrExpression();
        ++stats_.nodesCreated;
        left = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::BinaryOperator);
    }

    return left;
}

std::unique_ptr<ast::ASTNode> Parser::parseBitwiseOrExpression() {
    auto left = parseBitwiseXorExpression();

    while (matchToken(lexer::TokenType::BIT_OR)) {
        auto right = parseBitwiseXorExpression();
        ++stats_.nodesCreated;
        left = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::BinaryOperator);
    }

    return left;
}

std::unique_ptr<ast::ASTNode> Parser::parseBitwiseXorExpression() {
    auto left = parseBitwiseAndExpression();

    while (matchToken(lexer::TokenType::BIT_XOR)) {
        auto right = parseBitwiseAndExpression();
        ++stats_.nodesCreated;
        left = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::BinaryOperator);
    }

    return left;
}

std::unique_ptr<ast::ASTNode> Parser::parseBitwiseAndExpression() {
    auto left = parseEqualityExpression();

    while (matchToken(lexer::TokenType::BIT_AND)) {
        auto right = parseEqualityExpression();
        ++stats_.nodesCreated;
        left = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::BinaryOperator);
    }

    return left;
}

std::unique_ptr<ast::ASTNode> Parser::parseEqualityExpression() {
    auto left = parseRelationalExpression();

    while (matchToken(lexer::TokenType::EQUAL) ||
           matchToken(lexer::TokenType::NOT_EQUAL)) {
        auto right = parseRelationalExpression();
        ++stats_.nodesCreated;
        left = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::BinaryOperator);
    }

    return left;
}

std::unique_ptr<ast::ASTNode> Parser::parseRelationalExpression() {
    auto left = parseShiftExpression();

    while (matchToken(lexer::TokenType::LESS) ||
           matchToken(lexer::TokenType::GREATER) ||
           matchToken(lexer::TokenType::LESS_EQUAL) ||
           matchToken(lexer::TokenType::GREATER_EQUAL)) {
        auto right = parseShiftExpression();
        ++stats_.nodesCreated;
        left = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::BinaryOperator);
    }

    return left;
}

std::unique_ptr<ast::ASTNode> Parser::parseShiftExpression() {
    auto left = parseAdditiveExpression();

    while (matchToken(lexer::TokenType::LEFT_SHIFT) ||
           matchToken(lexer::TokenType::RIGHT_SHIFT)) {
        auto right = parseAdditiveExpression();
        ++stats_.nodesCreated;
        left = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::BinaryOperator);
    }

    return left;
}

std::unique_ptr<ast::ASTNode> Parser::parseAdditiveExpression() {
    auto left = parseMultiplicativeExpression();

    while (matchToken(lexer::TokenType::PLUS) ||
           matchToken(lexer::TokenType::MINUS)) {
        auto right = parseMultiplicativeExpression();
        ++stats_.nodesCreated;
        left = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::BinaryOperator);
    }

    return left;
}

std::unique_ptr<ast::ASTNode> Parser::parseMultiplicativeExpression() {
    auto left = parseUnaryExpression();

    while (matchToken(lexer::TokenType::STAR) ||
           matchToken(lexer::TokenType::SLASH) ||
           matchToken(lexer::TokenType::PERCENT)) {
        auto right = parseUnaryExpression();
        ++stats_.nodesCreated;
        left = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::BinaryOperator);
    }

    return left;
}

std::unique_ptr<ast::ASTNode> Parser::parseUnaryExpression() {
    if (matchToken(lexer::TokenType::PLUS) ||
        matchToken(lexer::TokenType::MINUS) ||
        matchToken(lexer::TokenType::LOGICAL_NOT) ||
        matchToken(lexer::TokenType::BIT_NOT)) {
        auto operand = parseUnaryExpression();
        ++stats_.nodesCreated;
        return std::make_unique<ast::ASTNode>(ast::ASTNodeKind::UnaryOperator);
    }

    return parsePrimaryExpression();
}

std::unique_ptr<ast::ASTNode> Parser::parsePrimaryExpression() {
    if (matchToken(lexer::TokenType::IDENTIFIER)) {
        ++stats_.nodesCreated;
        return std::make_unique<ast::ASTNode>(ast::ASTNodeKind::Identifier);
    }

    if (matchToken(lexer::TokenType::INTEGER_LITERAL) ||
        matchToken(lexer::TokenType::FLOAT_LITERAL) ||
        matchToken(lexer::TokenType::CHAR_LITERAL) ||
        matchToken(lexer::TokenType::STRING_LITERAL) ||
        matchToken(lexer::TokenType::TRUE_LITERAL) ||
        matchToken(lexer::TokenType::FALSE_LITERAL) ||
        matchToken(lexer::TokenType::NULLPTR_LITERAL)) {
        ++stats_.nodesCreated;
        return std::make_unique<ast::ASTNode>(ast::ASTNodeKind::Literal);
    }

    if (matchToken(lexer::TokenType::LEFT_PAREN)) {
        auto expr = parseExpression();
        if (!matchToken(lexer::TokenType::RIGHT_PAREN)) {
            reportError("se esperaba ')'", currentLocation());
        }
        return expr;
    }

    reportError("expresión primaria inválida", currentLocation());
    return nullptr;
}

// === PARSING DE SENTENCIAS ===

std::unique_ptr<ast::ASTNode> Parser::parseStatement() {
    if (matchToken(lexer::TokenType::LEFT_BRACE)) {
        return parseCompoundStatement();
    }

    if (matchToken(lexer::TokenType::IF)) {
        return parseIfStatement();
    }

    if (matchToken(lexer::TokenType::WHILE)) {
        return parseWhileStatement();
    }

    if (matchToken(lexer::TokenType::FOR)) {
        return parseForStatement();
    }

    if (matchToken(lexer::TokenType::RETURN)) {
        return parseReturnStatement();
    }

    // Sentencia de expresión
    return parseExpressionStatement();
}

std::unique_ptr<ast::ASTNode> Parser::parseCompoundStatement() {
    ++stats_.nodesCreated;
    auto compoundStmt = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::CompoundStmt);

    while (!checkToken(lexer::TokenType::RIGHT_BRACE) && !isAtEnd()) {
        auto stmt = parseStatement();
        if (stmt) {
            // Añadir a la lista de sentencias
        }
    }

    if (!matchToken(lexer::TokenType::RIGHT_BRACE)) {
        reportError("se esperaba '}'", currentLocation());
    }

    return compoundStmt;
}

std::unique_ptr<ast::ASTNode> Parser::parseIfStatement() {
    if (!matchToken(lexer::TokenType::LEFT_PAREN)) {
        reportError("se esperaba '(' después de 'if'", currentLocation());
        return nullptr;
    }

    auto condition = parseExpression();

    if (!matchToken(lexer::TokenType::RIGHT_PAREN)) {
        reportError("se esperaba ')'", currentLocation());
        return nullptr;
    }

    auto thenStmt = parseStatement();
    std::unique_ptr<ast::ASTNode> elseStmt = nullptr;

    if (matchToken(lexer::TokenType::ELSE)) {
        elseStmt = parseStatement();
    }

    ++stats_.nodesCreated;
    return std::make_unique<ast::ASTNode>(ast::ASTNodeKind::IfStmt);
}

std::unique_ptr<ast::ASTNode> Parser::parseWhileStatement() {
    if (!matchToken(lexer::TokenType::LEFT_PAREN)) {
        reportError("se esperaba '(' después de 'while'", currentLocation());
        return nullptr;
    }

    auto condition = parseExpression();

    if (!matchToken(lexer::TokenType::RIGHT_PAREN)) {
        reportError("se esperaba ')'", currentLocation());
        return nullptr;
    }

    auto body = parseStatement();

    ++stats_.nodesCreated;
    return std::make_unique<ast::ASTNode>(ast::ASTNodeKind::WhileStmt);
}

std::unique_ptr<ast::ASTNode> Parser::parseForStatement() {
    if (!matchToken(lexer::TokenType::LEFT_PAREN)) {
        reportError("se esperaba '(' después de 'for'", currentLocation());
        return nullptr;
    }

    // Parsear inicialización (opcional)
    std::unique_ptr<ast::ASTNode> init = nullptr;
    if (!checkToken(lexer::TokenType::SEMICOLON)) {
        if (ParserUtils::canStartDeclaration(currentToken())) {
            init = parseDeclaration();
        } else {
            init = parseExpression();
        }
    }

    if (!matchToken(lexer::TokenType::SEMICOLON)) {
        reportError("se esperaba ';'", currentLocation());
    }

    // Parsear condición (opcional)
    std::unique_ptr<ast::ASTNode> condition = nullptr;
    if (!checkToken(lexer::TokenType::SEMICOLON)) {
        condition = parseExpression();
    }

    if (!matchToken(lexer::TokenType::SEMICOLON)) {
        reportError("se esperaba ';'", currentLocation());
    }

    // Parsear incremento (opcional)
    std::unique_ptr<ast::ASTNode> increment = nullptr;
    if (!checkToken(lexer::TokenType::RIGHT_PAREN)) {
        increment = parseExpression();
    }

    if (!matchToken(lexer::TokenType::RIGHT_PAREN)) {
        reportError("se esperaba ')'", currentLocation());
    }

    auto body = parseStatement();

    ++stats_.nodesCreated;
    return std::make_unique<ast::ASTNode>(ast::ASTNodeKind::ForStmt);
}

std::unique_ptr<ast::ASTNode> Parser::parseReturnStatement() {
    std::unique_ptr<ast::ASTNode> expr = nullptr;

    if (!checkToken(lexer::TokenType::SEMICOLON)) {
        expr = parseExpression();
    }

    if (!matchToken(lexer::TokenType::SEMICOLON)) {
        reportError("se esperaba ';'", currentLocation());
    }

    ++stats_.nodesCreated;
    return std::make_unique<ast::ASTNode>(ast::ASTNodeKind::ReturnStmt);
}

std::unique_ptr<ast::ASTNode> Parser::parseExpressionStatement() {
    auto expr = parseExpression();

    if (!matchToken(lexer::TokenType::SEMICOLON)) {
        reportError("se esperaba ';'", currentLocation());
    }

    ++stats_.nodesCreated;
    return std::make_unique<ast::ASTNode>(ast::ASTNodeKind::ExprStmt);
}

// === UTILIDADES ===

int Parser::getOperatorPrecedence(lexer::TokenType type) const {
    return lexer::TokenUtils::getOperatorPrecedence(type);
}

bool Parser::isRightAssociative(lexer::TokenType type) const {
    // Operadores de asignación son asociativos a la derecha
    return lexer::TokenUtils::isAssignmentOperator(type);
}

diagnostics::SourceLocation Parser::currentLocation() const {
    return currentToken().getLocation();
}

// ============================================================================
// ParserUtils - Implementación
// ============================================================================

bool ParserUtils::canStartDeclaration(const lexer::Token& token) {
    return isTypeKeyword(token.getLexeme()) ||
           token.getType() == lexer::TokenType::CONST ||
           token.getType() == lexer::TokenType::VOLATILE ||
           token.getType() == lexer::TokenType::STATIC ||
           token.getType() == lexer::TokenType::EXTERN ||
           token.getType() == lexer::TokenType::INLINE;
}

bool ParserUtils::canStartExpression(const lexer::Token& token) {
    return token.getType() == lexer::TokenType::IDENTIFIER ||
           token.isLiteral() ||
           token.getType() == lexer::TokenType::LEFT_PAREN ||
           isUnaryOperator(token.getType());
}

bool ParserUtils::canStartStatement(const lexer::Token& token) {
    return canStartExpression(token) ||
           canStartDeclaration(token) ||
           token.getType() == lexer::TokenType::LEFT_BRACE ||
           token.getType() == lexer::TokenType::IF ||
           token.getType() == lexer::TokenType::WHILE ||
           token.getType() == lexer::TokenType::FOR ||
           token.getType() == lexer::TokenType::RETURN ||
           token.getType() == lexer::TokenType::BREAK ||
           token.getType() == lexer::TokenType::CONTINUE;
}

bool ParserUtils::isTypeKeyword(const std::string& word) {
    static const std::vector<std::string> typeKeywords = {
        "void", "bool", "char", "short", "int", "long", "float", "double",
        "signed", "unsigned", "struct", "class", "union", "enum", "auto"
    };

    return std::find(typeKeywords.begin(), typeKeywords.end(), word) != typeKeywords.end();
}

bool ParserUtils::isAssignmentOperator(lexer::TokenType type) {
    return lexer::TokenUtils::isAssignmentOperator(type);
}

bool ParserUtils::isBinaryOperator(lexer::TokenType type) {
    return lexer::TokenUtils::isBinaryOperator(type);
}

bool ParserUtils::isUnaryOperator(lexer::TokenType type) {
    return lexer::TokenUtils::isUnaryOperator(type);
}

} // namespace cpp20::compiler::frontend
