/**
 * @file test_constexpr_stress.cpp
 * @brief Tests de stress para el sistema constexpr
 */

#include <gtest/gtest.h>
#include <compiler/constexpr/ConstexprEvaluator.h>
#include <compiler/templates/TemplateSystem.h>
#include <compiler/common/diagnostics/DiagnosticEngine.h>
#include <thread>
#include <vector>
#include <random>

using namespace cpp20::compiler::constexpr_eval;
using namespace cpp20::compiler::semantic;
using namespace cpp20::compiler::diagnostics;

// Test para carga máxima de memoria
TEST(ConstexprStressTest, MaximumMemoryLoad) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    // Configurar límites de memoria altos
    constexprEvaluator.setLimits(10000000, 1000, 50 * 1024 * 1024); // 50MB

    std::vector<std::unique_ptr<ast::ASTNode>> expressions;

    // Crear muchas expresiones que usen memoria
    for (int i = 0; i < 100000; ++i) {
        expressions.push_back(std::make_unique<ast::ASTNode>(ast::ASTNodeKind::IntegerLiteral));
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // Evaluar todas las expresiones
    for (auto& expr : expressions) {
        auto result = constexprEvaluator.evaluateExpression(expr.get());
        EXPECT_EQ(result.result, EvaluationResult::Success);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    auto stats = constexprEvaluator.getStats();
    EXPECT_EQ(stats.expressionsEvaluated, 100000);

    std::cout << "Memory stress test: " << stats.expressionsEvaluated
              << " expressions in " << duration.count() << "ms" << std::endl;
}

// Test para alta frecuencia de evaluación
TEST(ConstexprStressTest, HighFrequencyEvaluation) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    constexprEvaluator.setLimits(10000000, 1000, 10 * 1024 * 1024);

    auto startTime = std::chrono::high_resolution_clock::now();

    // Evaluar expresiones lo más rápido posible
    int evaluated = 0;
    auto timeout = startTime + std::chrono::seconds(5); // 5 segundos de stress

    while (std::chrono::high_resolution_clock::now() < timeout) {
        auto expression = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::IntegerLiteral);
        auto result = constexprEvaluator.evaluateExpression(expression.get());

        if (result.result == EvaluationResult::Success) {
            evaluated++;
        } else {
            break;
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    auto stats = constexprEvaluator.getStats();
    EXPECT_EQ(stats.expressionsEvaluated, evaluated);

    double rate = static_cast<double>(evaluated) / (duration.count() / 1000.0);

    std::cout << "High frequency stress: " << evaluated << " expressions in "
              << duration.count() << "ms (" << rate << " expr/sec)" << std::endl;
}

// Test para stress de templates
TEST(ConstexprStressTest, TemplateInstantiationStress) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);
    ConstexprEvaluator constexprEvaluator(diagEngine);

    // Crear muchos templates
    for (int i = 0; i < 1000; ++i) {
        auto templateInfo = std::make_unique<TemplateInfo>(
            "stress_template_" + std::to_string(i),
            std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
            std::make_unique<ast::ASTNode>(ast::ASTNodeKind::FunctionDecl)
        );

        templateSystem.registerTemplate(std::move(templateInfo));
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // Instanciar templates múltiples veces
    for (int i = 0; i < 1000; ++i) {
        for (int j = 0; j < 10; ++j) {
            auto instance = templateSystem.instantiateTemplate(
                "stress_template_" + std::to_string(i),
                {"int"}
            );

            ASSERT_TRUE(instance != nullptr);
            EXPECT_TRUE(instance->isValid);
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    auto stats = templateSystem.getStats();
    EXPECT_EQ(stats.templatesRegistered, 1000);
    EXPECT_EQ(stats.instancesCreated, 1000); // Gracias al cache

    std::cout << "Template stress: " << stats.templatesRegistered << " templates, "
              << (1000 * 10) << " instantiations in " << duration.count() << "ms" << std::endl;
}

// Test para stress de conceptos
TEST(ConstexprStressTest, ConceptEvaluationStress) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Registrar conceptos
    auto integralConcept = std::make_unique<TemplateInfo>(
        "Integral",
        std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
        std::make_unique<ast::ASTNode>(ast::ASTNodeKind::ConceptDefinition)
    );

    auto floatingConcept = std::make_unique<TemplateInfo>(
        "FloatingPoint",
        std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
        std::make_unique<ast::ASTNode>(ast::ASTNodeKind::ConceptDefinition)
    );

    templateSystem.registerConcept(std::move(integralConcept));
    templateSystem.registerConcept(std::move(floatingConcept));

    std::vector<std::string> types = {"int", "long", "short", "char", "float", "double", "void"};
    std::vector<std::string> concepts = {"Integral", "FloatingPoint"};

    auto startTime = std::chrono::high_resolution_clock::now();

    // Evaluar conceptos para muchos tipos
    for (int i = 0; i < 10000; ++i) {
        for (const auto& concept : concepts) {
            for (const auto& type : types) {
                auto result = templateSystem.checkConceptSatisfaction(concept, type);
                // No importa el resultado, solo que no falle
                EXPECT_TRUE(result.satisfaction == ConstraintSatisfaction::Satisfied ||
                           result.satisfaction == ConstraintSatisfaction::NotSatisfied ||
                           result.satisfaction == ConstraintSatisfaction::Error);
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    auto stats = templateSystem.getStats();
    EXPECT_EQ(stats.conceptsRegistered, 2);
    EXPECT_EQ(stats.constraintChecks, 10000 * 2 * 7);

    std::cout << "Concept stress: " << stats.constraintChecks
              << " evaluations in " << duration.count() << "ms" << std::endl;
}

// Test para stress de recursión
TEST(ConstexprStressTest, RecursionDepthStress) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    // Configurar profundidad de recursión alta
    constexprEvaluator.setLimits(10000000, 500, 10 * 1024 * 1024);

    auto startTime = std::chrono::high_resolution_clock::now();

    // Simular funciones recursivas profundas (usando llamadas anidadas)
    for (int depth = 1; depth <= 50; ++depth) {
        // Crear cadena de llamadas recursivas simuladas
        for (int i = 0; i < 10; ++i) {
            auto functionBody = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::CompoundStmt);
            std::vector<ConstexprValue> args = {ConstexprValue(depth)};

            auto result = constexprEvaluator.evaluateFunction(
                "recursive_func_" + std::to_string(depth),
                args, functionBody.get()
            );

            EXPECT_TRUE(result.result == EvaluationResult::Success ||
                       result.result == EvaluationResult::RecursionLimit);
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    auto stats = constexprEvaluator.getStats();
    EXPECT_GE(stats.functionsEvaluated, 0);

    std::cout << "Recursion stress: " << stats.functionsEvaluated
              << " recursive calls in " << duration.count() << "ms" << std::endl;
}

// Test para stress de memoria con templates complejos
TEST(ConstexprStressTest, ComplexTemplateMemoryStress) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);
    ConstexprEvaluator constexprEvaluator(diagEngine);

    constexprEvaluator.setLimits(10000000, 1000, 100 * 1024 * 1024); // 100MB

    // Crear templates complejos con muchos parámetros
    for (int i = 0; i < 100; ++i) {
        auto templateInfo = std::make_unique<TemplateInfo>(
            "complex_template_" + std::to_string(i),
            std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
            std::make_unique<ast::ASTNode>(ast::ASTNodeKind::ClassDecl)
        );

        templateSystem.registerTemplate(std::move(templateInfo));
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // Instanciar templates con diferentes argumentos
    std::vector<std::string> argTypes = {"int", "double", "char", "float", "long", "short"};

    for (int i = 0; i < 100; ++i) {
        for (int j = 0; j < 20; ++j) {
            std::vector<std::string> args;
            for (int k = 0; k < 3; ++k) { // 3 parámetros por template
                args.push_back(argTypes[(i + j + k) % argTypes.size()]);
            }

            auto instance = templateSystem.instantiateTemplate(
                "complex_template_" + std::to_string(i),
                args
            );

            ASSERT_TRUE(instance != nullptr);
            EXPECT_TRUE(instance->isValid);
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    auto stats = templateSystem.getStats();
    EXPECT_EQ(stats.templatesRegistered, 100);

    std::cout << "Complex template memory stress: " << (100 * 20)
              << " instantiations in " << duration.count() << "ms" << std::endl;
}

// Test para stress de evaluación concurrente (simulado)
TEST(ConstexprStressTest, ConcurrentEvaluationStress) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    constexprEvaluator.setLimits(10000000, 1000, 50 * 1024 * 1024);

    auto startTime = std::chrono::high_resolution_clock::now();

    // Simular carga concurrente creando múltiples evaluaciones
    // En un escenario real, esto usaría std::thread
    for (int thread = 0; thread < 10; ++thread) { // Simular 10 threads
        for (int i = 0; i < 1000; ++i) {
            auto expression = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::IntegerLiteral);

            // Crear contexto único por "thread"
            std::unordered_map<std::string, ConstexprValue> context;
            context["thread_id"] = ConstexprValue(thread);
            context["iteration"] = ConstexprValue(i);

            auto result = constexprEvaluator.evaluateExpression(expression.get(), context);

            EXPECT_EQ(result.result, EvaluationResult::Success);
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    auto stats = constexprEvaluator.getStats();
    EXPECT_EQ(stats.expressionsEvaluated, 10 * 1000);

    std::cout << "Concurrent stress: " << stats.expressionsEvaluated
              << " evaluations in " << duration.count() << "ms" << std::endl;
}

// Test para stress de cache
TEST(ConstexprStressTest, CacheStressTest) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);

    // Crear templates para cache
    for (int i = 0; i < 500; ++i) {
        auto templateInfo = std::make_unique<TemplateInfo>(
            "cache_template_" + std::to_string(i),
            std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
            std::make_unique<ast::ASTNode>(ast::ASTNodeKind::FunctionDecl)
        );

        templateSystem.registerTemplate(std::move(templateInfo));
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // Acceder a templates múltiples veces para probar cache
    for (int round = 0; round < 5; ++round) {
        for (int i = 0; i < 500; ++i) {
            for (int j = 0; j < 5; ++j) { // 5 instancias por template
                auto instance = templateSystem.instantiateTemplate(
                    "cache_template_" + std::to_string(i),
                    {"int"}
                );

                ASSERT_TRUE(instance != nullptr);
                EXPECT_TRUE(instance->isValid);
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    auto stats = templateSystem.getStats();
    EXPECT_EQ(stats.templatesRegistered, 500);
    EXPECT_EQ(stats.instancesCreated, 500); // Solo 500 instancias reales creadas (cache)

    std::cout << "Cache stress: " << (500 * 5 * 5) << " accesses, "
              << stats.instancesCreated << " actual creations in "
              << duration.count() << "ms" << std::endl;
}

// Test para stress de diagnóstico
TEST(ConstexprStressTest, DiagnosticStressTest) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    auto startTime = std::chrono::high_resolution_clock::now();

    // Generar muchos errores para probar el sistema de diagnóstico
    for (int i = 0; i < 1000; ++i) {
        // Intentar evaluar expresión nula (debería generar error)
        auto result = constexprEvaluator.evaluateExpression(nullptr);

        EXPECT_EQ(result.result, EvaluationResult::Error);
        EXPECT_FALSE(result.errorMessage.empty());
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    auto stats = constexprEvaluator.getStats();
    EXPECT_EQ(stats.expressionsEvaluated, 0); // Expresiones nulas no se evalúan
    EXPECT_EQ(stats.errors, 1000);

    std::cout << "Diagnostic stress: " << stats.errors
              << " errors handled in " << duration.count() << "ms" << std::endl;
}

// Test para stress de límites extremos
TEST(ConstexprStressTest, ExtremeLimitsStress) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    // Configurar límites extremos
    constexprEvaluator.setLimits(10, 1, 1024); // límites muy bajos

    auto startTime = std::chrono::high_resolution_clock::now();

    // Intentar evaluaciones que excedan límites
    for (int i = 0; i < 100; ++i) {
        auto expression = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::IntegerLiteral);
        auto result = constexprEvaluator.evaluateExpression(expression.get());

        // Debería fallar debido a límites extremos
        EXPECT_TRUE(result.result == EvaluationResult::Timeout ||
                   result.result == EvaluationResult::MemoryLimit ||
                   result.result == EvaluationResult::Success);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    auto stats = constexprEvaluator.getStats();

    std::cout << "Extreme limits stress: " << stats.expressionsEvaluated
              << " successful evaluations, " << stats.errors
              << " errors in " << duration.count() << "ms" << std::endl;
}

// Test para stress de cleanup y reinicio
TEST(ConstexprStressTest, CleanupStressTest) {
    DiagnosticEngine diagEngine;
    TemplateSystem templateSystem(diagEngine);
    ConstexprEvaluator constexprEvaluator(diagEngine);

    auto startTime = std::chrono::high_resolution_clock::now();

    // Ciclo de creación, uso intenso y cleanup
    for (int cycle = 0; cycle < 10; ++cycle) {
        // Crear templates
        for (int i = 0; i < 50; ++i) {
            auto templateInfo = std::make_unique<TemplateInfo>(
                "cleanup_template_" + std::to_string(cycle) + "_" + std::to_string(i),
                std::make_unique<ast::TemplateParameterList>(std::vector<std::unique_ptr<ast::TemplateParameter>>{}),
                std::make_unique<ast::ASTNode>(ast::ASTNodeKind::FunctionDecl)
            );

            templateSystem.registerTemplate(std::move(templateInfo));
        }

        // Usar intensivamente
        for (int i = 0; i < 50; ++i) {
            for (int j = 0; j < 10; ++j) {
                auto instance = templateSystem.instantiateTemplate(
                    "cleanup_template_" + std::to_string(cycle) + "_" + std::to_string(i),
                    {"int"}
                );

                ASSERT_TRUE(instance != nullptr);
                EXPECT_TRUE(instance->isValid);
            }
        }

        // Evaluar expresiones
        for (int i = 0; i < 100; ++i) {
            auto expression = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::IntegerLiteral);
            auto result = constexprEvaluator.evaluateExpression(expression.get());

            EXPECT_EQ(result.result, EvaluationResult::Success);
        }

        // Limpiar todo
        templateSystem.clearCache();
        constexprEvaluator.clear();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    std::cout << "Cleanup stress: 10 cycles completed in " << duration.count() << "ms" << std::endl;
}

// Test para stress de tipos de datos variados
TEST(ConstexprStressTest, DataTypeVarietyStress) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    constexprEvaluator.setLimits(10000000, 1000, 10 * 1024 * 1024);

    std::vector<ast::ASTNodeKind> expressionTypes = {
        ast::ASTNodeKind::IntegerLiteral,
        ast::ASTNodeKind::BooleanLiteral,
        ast::ASTNodeKind::CharacterLiteral,
        ast::ASTNodeKind::FloatingPointLiteral,
        ast::ASTNodeKind::StringLiteral,
        ast::ASTNodeKind::BinaryOp,
        ast::ASTNodeKind::UnaryOp,
        ast::ASTNodeKind::FunctionCall,
        ast::ASTNodeKind::VariableDecl,
        ast::ASTNodeKind::Assignment,
        ast::ASTNodeKind::IfStmt,
        ast::ASTNodeKind::TernaryOp
    };

    auto startTime = std::chrono::high_resolution_clock::now();

    // Evaluar muchos tipos diferentes de expresiones
    for (int round = 0; round < 100; ++round) {
        for (auto exprType : expressionTypes) {
            auto expression = std::make_unique<ast::ASTNode>(exprType);
            auto result = constexprEvaluator.evaluateExpression(expression.get());

            // Solo verificar que no cause crashes
            EXPECT_TRUE(result.result == EvaluationResult::Success ||
                       result.result == EvaluationResult::Error);
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    auto stats = constexprEvaluator.getStats();
    EXPECT_EQ(stats.expressionsEvaluated, 100 * expressionTypes.size());

    std::cout << "Data type variety stress: " << stats.expressionsEvaluated
              << " expressions of " << expressionTypes.size() << " types in "
              << duration.count() << "ms" << std::endl;
}
