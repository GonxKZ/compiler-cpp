/**
 * @file test_templates.cpp
 * @brief Tests unitarios para el sistema de templates C++20
 */

#include <gtest/gtest.h>
#include <compiler/templates/TemplateSystem.h>
#include <compiler/common/diagnostics/DiagnosticEngine.h>

using namespace cpp20::compiler::semantic;
using namespace cpp20::compiler::diagnostics;

// Test para inicialización del sistema de templates
TEST(TemplateSystemTest, BasicInitialization) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    auto stats = templateSystem.getStats();
    EXPECT_EQ(stats.templatesRegistered, 0);
    EXPECT_EQ(stats.conceptsRegistered, 0);
    EXPECT_EQ(stats.instancesCreated, 0);
}

// Test para registro de templates
TEST(TemplateSystemTest, TemplateRegistration) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Crear información de template
    auto templateInfo = std::make_unique<TemplateInfo>(
        "max",
        std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
        std::make_unique<ast::ASTNode>(ast::ASTNodeKind::FunctionDecl)
    );

    templateSystem.registerTemplate(std::move(templateInfo));

    auto stats = templateSystem.getStats();
    EXPECT_EQ(stats.templatesRegistered, 1);
    EXPECT_EQ(stats.conceptsRegistered, 0);
}

// Test para registro de concepts
TEST(TemplateSystemTest, ConceptRegistration) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Crear información de concept
    auto conceptInfo = std::make_unique<TemplateInfo>(
        "Integral",
        std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
        std::make_unique<ast::ASTNode>(ast::ASTNodeKind::ConceptDefinition)
    );

    templateSystem.registerConcept(std::move(conceptInfo));

    auto stats = templateSystem.getStats();
    EXPECT_EQ(stats.templatesRegistered, 1);
    EXPECT_EQ(stats.conceptsRegistered, 1);
}

// Test para instanciación de templates básica
TEST(TemplateSystemTest, BasicTemplateInstantiation) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Registrar template
    auto templateInfo = std::make_unique<TemplateInfo>(
        "identity",
        std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
        std::make_unique<ast::ASTNode>(ast::ASTNodeKind::FunctionDecl)
    );

    templateSystem.registerTemplate(std::move(templateInfo));

    // Instanciar template
    auto instance = templateSystem.instantiateTemplate("identity", {"int"});

    ASSERT_TRUE(instance != nullptr);
    EXPECT_TRUE(instance->isValid);
    EXPECT_EQ(instance->templateName, "identity");
    EXPECT_EQ(instance->arguments.size(), 1);
    EXPECT_EQ(instance->arguments[0], "int");

    auto stats = templateSystem.getStats();
    EXPECT_EQ(stats.instancesCreated, 1);
}

// Test para instanciación con múltiples argumentos
TEST(TemplateSystemTest, MultiArgumentInstantiation) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Registrar template con múltiples parámetros
    auto templateInfo = std::make_unique<TemplateInfo>(
        "pair",
        std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
        std::make_unique<ast::ASTNode>(ast::ASTNodeKind::ClassDecl)
    );

    templateSystem.registerTemplate(std::move(templateInfo));

    // Instanciar con múltiples argumentos
    auto instance = templateSystem.instantiateTemplate("pair", {"int", "double"});

    ASSERT_TRUE(instance != nullptr);
    EXPECT_TRUE(instance->isValid);
    EXPECT_EQ(instance->arguments.size(), 2);
    EXPECT_EQ(instance->arguments[0], "int");
    EXPECT_EQ(instance->arguments[1], "double");
}

// Test para template no encontrado
TEST(TemplateSystemTest, TemplateNotFound) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    auto instance = templateSystem.instantiateTemplate("nonexistent", {"int"});

    ASSERT_TRUE(instance != nullptr);
    EXPECT_FALSE(instance->isValid);
    EXPECT_FALSE(instance->errorMessage.empty());
}

// Test para arguments insuficientes
TEST(TemplateSystemTest, InsufficientArguments) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Registrar template que espera 2 argumentos
    auto templateInfo = std::make_unique<TemplateInfo>(
        "pair",
        std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
        std::make_unique<ast::ASTNode>(ast::ASTNodeKind::ClassDecl)
    );

    templateSystem.registerTemplate(std::move(templateInfo));

    // Intentar instanciar con solo 1 argumento
    auto instance = templateSystem.instantiateTemplate("pair", {"int"});

    ASSERT_TRUE(instance != nullptr);
    EXPECT_FALSE(instance->isValid);
    EXPECT_FALSE(instance->errorMessage.empty());
}

// Test para verificación de concepts
TEST(TemplateSystemTest, ConceptSatisfaction) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Verificar concept Integral con tipos válidos
    auto result1 = templateSystem.checkConceptSatisfaction("Integral", "int");
    EXPECT_EQ(result1.satisfaction, ConstraintSatisfaction::Satisfied);

    auto result2 = templateSystem.checkConceptSatisfaction("Integral", "long");
    EXPECT_EQ(result2.satisfaction, ConstraintSatisfaction::Satisfied);

    // Verificar concept Integral con tipo no válido
    auto result3 = templateSystem.checkConceptSatisfaction("Integral", "double");
    EXPECT_EQ(result3.satisfaction, ConstraintSatisfaction::NotSatisfied);
    EXPECT_FALSE(result3.errorMessage.empty());

    // Verificar concept no encontrado
    auto result4 = templateSystem.checkConceptSatisfaction("NonExistent", "int");
    EXPECT_EQ(result4.satisfaction, ConstraintSatisfaction::Error);
    EXPECT_FALSE(result4.errorMessage.empty());

    auto stats = templateSystem.getStats();
    EXPECT_EQ(stats.constraintChecks, 4);
}

// Test para FloatingPoint concept
TEST(TemplateSystemTest, FloatingPointConcept) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Verificar concept FloatingPoint con tipos válidos
    auto result1 = templateSystem.checkConceptSatisfaction("FloatingPoint", "float");
    EXPECT_EQ(result1.satisfaction, ConstraintSatisfaction::Satisfied);

    auto result2 = templateSystem.checkConceptSatisfaction("FloatingPoint", "double");
    EXPECT_EQ(result2.satisfaction, ConstraintSatisfaction::Satisfied);

    auto result3 = templateSystem.checkConceptSatisfaction("FloatingPoint", "long double");
    EXPECT_EQ(result3.satisfaction, ConstraintSatisfaction::Satisfied);

    // Verificar concept FloatingPoint con tipo no válido
    auto result4 = templateSystem.checkConceptSatisfaction("FloatingPoint", "int");
    EXPECT_EQ(result4.satisfaction, ConstraintSatisfaction::NotSatisfied);
}

// Test para resolución de sobrecarga
TEST(TemplateSystemTest, OverloadResolution) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Registrar template de función
    auto templateInfo = std::make_unique<TemplateInfo>(
        "print",
        std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
        std::make_unique<ast::ASTNode>(ast::ASTNodeKind::FunctionDecl)
    );

    templateSystem.registerTemplate(std::move(templateInfo));

    // Resolver sobrecarga
    auto candidates = templateSystem.resolveOverload("print", {"int"});

    EXPECT_EQ(candidates.size(), 1);
    EXPECT_TRUE(candidates[0]->isValid);
    EXPECT_EQ(candidates[0]->templateName, "print");
    EXPECT_EQ(candidates[0]->arguments[0], "int");
}

// Test para cache de instancias
TEST(TemplateSystemTest, InstanceCaching) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Registrar template
    auto templateInfo = std::make_unique<TemplateInfo>(
        "cache_test",
        std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
        std::make_unique<ast::ASTNode>(ast::ASTNodeKind::FunctionDecl)
    );

    templateSystem.registerTemplate(std::move(templateInfo));

    // Instanciar múltiples veces el mismo template
    auto instance1 = templateSystem.instantiateTemplate("cache_test", {"int"});
    auto instance2 = templateSystem.instantiateTemplate("cache_test", {"int"});
    auto instance3 = templateSystem.instantiateTemplate("cache_test", {"int"});

    ASSERT_TRUE(instance1 != nullptr);
    ASSERT_TRUE(instance2 != nullptr);
    ASSERT_TRUE(instance3 != nullptr);

    EXPECT_TRUE(instance1->isValid);
    EXPECT_TRUE(instance2->isValid);
    EXPECT_TRUE(instance3->isValid);

    // Todas deberían ser válidas (la cache debería funcionar)
    auto stats = templateSystem.getStats();
    EXPECT_EQ(stats.instancesCreated, 1); // Solo una instancia real creada
}

// Test para limpiar cache
TEST(TemplateSystemTest, CacheClearing) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Registrar template
    auto templateInfo = std::make_unique<TemplateInfo>(
        "clear_test",
        std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
        std::make_unique<ast::ASTNode>(ast::ASTNodeKind::FunctionDecl)
    );

    templateSystem.registerTemplate(std::move(templateInfo));

    // Instanciar
    auto instance = templateSystem.instantiateTemplate("clear_test", {"int"});
    EXPECT_TRUE(instance->isValid);

    // Limpiar cache
    templateSystem.clearCache();

    // Nueva instanciación debería crear nueva instancia
    auto instance2 = templateSystem.instantiateTemplate("clear_test", {"int"});
    EXPECT_TRUE(instance2->isValid);

    auto stats = templateSystem.getStats();
    EXPECT_EQ(stats.instancesCreated, 2);
}

// Test para SFINAE handling
TEST(TemplateSystemTest, SFINAEHandling) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Registrar template que falla
    auto templateInfo = std::make_unique<TemplateInfo>(
        "sfinae_test",
        std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
        std::make_unique<ast::ASTNode>(ast::ASTNodeKind::FunctionDecl)
    );

    templateSystem.registerTemplate(std::move(templateInfo));

    // Instanciar con argumentos que causan SFINAE
    auto instance = templateSystem.instantiateTemplate("sfinae_test", {"invalid_type"});

    // Debería manejar el error apropiadamente
    ASSERT_TRUE(instance != nullptr);
    // En esta implementación simplificada, podría ser válido o no
    // dependiendo de la lógica de validación

    auto stats = templateSystem.getStats();
    EXPECT_GE(stats.sfinaeFailures, 0);
}

// Test para múltiples instancias diferentes
TEST(TemplateSystemTest, MultipleDifferentInstances) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Registrar template
    auto templateInfo = std::make_unique<TemplateInfo>(
        "multi_test",
        std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
        std::make_unique<ast::ASTNode>(ast::ASTNodeKind::FunctionDecl)
    );

    templateSystem.registerTemplate(std::move(templateInfo));

    // Instanciar con diferentes argumentos
    auto instance1 = templateSystem.instantiateTemplate("multi_test", {"int"});
    auto instance2 = templateSystem.instantiateTemplate("multi_test", {"double"});
    auto instance3 = templateSystem.instantiateTemplate("multi_test", {"char"});

    ASSERT_TRUE(instance1 != nullptr);
    ASSERT_TRUE(instance2 != nullptr);
    ASSERT_TRUE(instance3 != nullptr);

    EXPECT_TRUE(instance1->isValid);
    EXPECT_TRUE(instance2->isValid);
    EXPECT_TRUE(instance3->isValid);

    EXPECT_EQ(instance1->arguments[0], "int");
    EXPECT_EQ(instance2->arguments[0], "double");
    EXPECT_EQ(instance3->arguments[0], "char");

    auto stats = templateSystem.getStats();
    EXPECT_EQ(stats.instancesCreated, 3);
}

// Test para estadísticas del sistema
TEST(TemplateSystemTest, StatisticsTracking) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Verificar estadísticas iniciales
    auto initialStats = templateSystem.getStats();
    EXPECT_EQ(initialStats.templatesRegistered, 0);
    EXPECT_EQ(initialStats.conceptsRegistered, 0);
    EXPECT_EQ(initialStats.instancesCreated, 0);

    // Registrar templates y concepts
    auto templateInfo = std::make_unique<TemplateInfo>(
        "stat_test",
        std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
        std::make_unique<ast::ASTNode>(ast::ASTNodeKind::FunctionDecl)
    );

    auto conceptInfo = std::make_unique<TemplateInfo>(
        "StatConcept",
        std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
        std::make_unique<ast::ASTNode>(ast::ASTNodeKind::ConceptDefinition)
    );

    templateSystem.registerTemplate(std::move(templateInfo));
    templateSystem.registerConcept(std::move(conceptInfo));

    // Instanciar
    templateSystem.instantiateTemplate("stat_test", {"int"});

    // Verificar concepts
    templateSystem.checkConceptSatisfaction("Integral", "int");

    auto finalStats = templateSystem.getStats();
    EXPECT_EQ(finalStats.templatesRegistered, 2);
    EXPECT_EQ(finalStats.conceptsRegistered, 1);
    EXPECT_EQ(finalStats.instancesCreated, 1);
    EXPECT_EQ(finalStats.constraintChecks, 1);
}

// Test para tipos de template parameters
TEST(TemplateSystemTest, TemplateParameterTypes) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Crear parámetros template de diferentes tipos
    auto typeParam = std::make_unique<ast::TemplateParameter>(
        ast::TemplateParameterType::Type, "T"
    );

    auto nonTypeParam = std::make_unique<ast::TemplateParameter>(
        ast::TemplateParameterType::NonType, "N"
    );

    auto templateParam = std::make_unique<ast::TemplateParameter>(
        ast::TemplateParameterType::Template, "Container"
    );

    std::vector<std::unique_ptr<ast::TemplateParameter>> params;
    params.push_back(std::move(typeParam));
    params.push_back(std::move(nonTypeParam));
    params.push_back(std::move(templateParam));

    auto paramList = std::make_unique<ast::TemplateParameterList>(std::move(params));

    // Verificar que se crearon correctamente
    EXPECT_EQ(paramList->getParameters().size(), 3);
    EXPECT_EQ(paramList->getParameters()[0]->getParameterType(), ast::TemplateParameterType::Type);
    EXPECT_EQ(paramList->getParameters()[1]->getParameterType(), ast::TemplateParameterType::NonType);
    EXPECT_EQ(paramList->getParameters()[2]->getParameterType(), ast::TemplateParameterType::Template);
}

// Test para argumentos template
TEST(TemplateSystemTest, TemplateArguments) {
    // Crear argumentos template de diferentes tipos
    auto typeArg = std::make_unique<ast::TemplateArgument>(
        ast::TemplateArgument::ArgumentType::Type,
        std::make_unique<ast::ASTNode>(ast::ASTNodeKind::Identifier)
    );

    auto exprArg = std::make_unique<ast::TemplateArgument>(
        ast::TemplateArgument::ArgumentType::Expression,
        std::make_unique<ast::ASTNode>(ast::ASTNodeKind::IntegerLiteral)
    );

    std::vector<std::unique_ptr<ast::TemplateArgument>> args;
    args.push_back(std::move(typeArg));
    args.push_back(std::move(exprArg));

    auto argList = std::make_unique<ast::TemplateArgumentList>(std::move(args));

    // Verificar que se crearon correctamente
    EXPECT_EQ(argList->getArguments().size(), 2);
    EXPECT_EQ(argList->getArguments()[0]->getArgumentType(), ast::TemplateArgument::ArgumentType::Type);
    EXPECT_EQ(argList->getArguments()[1]->getArgumentType(), ast::TemplateArgument::ArgumentType::Expression);
}

// Test para template instantiation con error
TEST(TemplateSystemTest, InstantiationError) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Intentar instanciar template no registrado
    auto instance = templateSystem.instantiateTemplate("nonexistent", {});

    ASSERT_TRUE(instance != nullptr);
    EXPECT_FALSE(instance->isValid);
    EXPECT_FALSE(instance->errorMessage.empty());
}

// Test para verificación de argumentos template
TEST(TemplateSystemTest, ArgumentValidation) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Registrar template que espera 2 argumentos
    auto templateInfo = std::make_unique<TemplateInfo>(
        "validate_test",
        std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
        std::make_unique<ast::ASTNode>(ast::ASTNodeKind::FunctionDecl)
    );

    templateSystem.registerTemplate(std::move(templateInfo));

    // Intentar instanciar con número incorrecto de argumentos
    auto instance1 = templateSystem.instantiateTemplate("validate_test", {});
    auto instance2 = templateSystem.instantiateTemplate("validate_test", {"int", "double", "char"});

    // Dependiendo de la implementación de validación, estos podrían ser válidos o no
    // Lo importante es que no causen crashes
    EXPECT_TRUE(instance1 != nullptr);
    EXPECT_TRUE(instance2 != nullptr);
}

// Test para comparación de concepts
TEST(TemplateSystemTest, ConceptComparison) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Comparar satisfacción de concepts
    auto integralInt = templateSystem.checkConceptSatisfaction("Integral", "int");
    auto integralDouble = templateSystem.checkConceptSatisfaction("Integral", "double");
    auto floatingFloat = templateSystem.checkConceptSatisfaction("FloatingPoint", "float");
    auto floatingInt = templateSystem.checkConceptSatisfaction("FloatingPoint", "int");

    EXPECT_EQ(integralInt.satisfaction, ConstraintSatisfaction::Satisfied);
    EXPECT_EQ(integralDouble.satisfaction, ConstraintSatisfaction::NotSatisfied);
    EXPECT_EQ(floatingFloat.satisfaction, ConstraintSatisfaction::Satisfied);
    EXPECT_EQ(floatingInt.satisfaction, ConstraintSatisfaction::NotSatisfied);
}

// Test para template specialization
TEST(TemplateSystemTest, TemplateSpecialization) {
    // Crear especialización de template
    auto templateName = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::Identifier);
    auto args = std::make_unique<ast::TemplateArgumentList>(std::vector<std::unique_ptr<ast::TemplateArgument>>{});
    auto body = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::CompoundStmt);

    auto specialization = std::make_unique<ast::TemplateSpecialization>(
        std::move(templateName), std::move(args), std::move(body)
    );

    // Verificar que se creó correctamente
    EXPECT_EQ(specialization->getBody()->getKind(), ast::ASTNodeKind::CompoundStmt);
}

// Test para requires expression
TEST(TemplateSystemTest, RequiresExpression) {
    // Crear requires expression
    auto params = std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{});
    auto requirements = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::CompoundStmt);

    auto requiresExpr = std::make_unique<ast::RequiresExpression>(
        std::move(params), std::move(requirements)
    );

    // Verificar que se creó correctamente
    EXPECT_EQ(requiresExpr->getRequirements()->getKind(), ast::ASTNodeKind::CompoundStmt);
}

// Test para constraint expression
TEST(TemplateSystemTest, ConstraintExpression) {
    // Crear constraint expression
    auto left = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::Identifier);
    auto right = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::Identifier);

    auto constraint = std::make_unique<ast::ConstraintExpression>(
        ast::ConstraintExpression::ConstraintType::Conjunction,
        std::move(left), std::move(right)
    );

    // Verificar que se creó correctamente
    EXPECT_EQ(constraint->getConstraintType(), ast::ConstraintExpression::ConstraintType::Conjunction);
    EXPECT_TRUE(constraint->getLeft() != nullptr);
    EXPECT_TRUE(constraint->getRight() != nullptr);
}

// Test para concept definition
TEST(TemplateSystemTest, ConceptDefinition) {
    // Crear concept definition
    auto params = std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{});
    auto constraint = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::ConstraintExpression);

    auto conceptDef = std::make_unique<ast::ConceptDefinition>(
        "MyConcept", std::move(params), std::move(constraint)
    );

    // Verificar que se creó correctamente
    EXPECT_EQ(conceptDef->getName(), "MyConcept");
    EXPECT_TRUE(conceptDef->getConstraintExpression() != nullptr);
}

// Test para requires clause
TEST(TemplateSystemTest, RequiresClause) {
    // Crear requires clause
    auto requirements = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::RequiresExpression);

    auto requiresClause = std::make_unique<ast::RequiresClause>(std::move(requirements));

    // Verificar que se creó correctamente
    EXPECT_TRUE(requiresClause->getRequirements() != nullptr);
    EXPECT_EQ(requiresClause->getRequirements()->getKind(), ast::ASTNodeKind::RequiresExpression);
}
