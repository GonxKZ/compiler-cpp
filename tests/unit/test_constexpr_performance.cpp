/**
 * @file test_constexpr_performance.cpp
 * @brief Tests de performance para el sistema constexpr
 */

#include <gtest/gtest.h>
#include <compiler/constexpr/ConstexprEvaluator.h>
#include <compiler/common/diagnostics/DiagnosticEngine.h>
#include <chrono>
#include <vector>

using namespace cpp20::compiler::constexpr_eval;
using namespace cpp20::compiler::diagnostics;

// Test para medir performance de evaluación básica
TEST(ConstexprPerformanceTest, BasicEvaluationPerformance) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    // Configurar límites generosos
    constexprEvaluator.setLimits(1000000, 100, 1024 * 1024);

    auto startTime = std::chrono::high_resolution_clock::now();

    // Evaluar 1000 expresiones simples
    for (int i = 0; i < 1000; ++i) {
        auto expression = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::IntegerLiteral);
        auto result = constexprEvaluator.evaluateExpression(expression.get());

        ASSERT_EQ(result.result, EvaluationResult::Success);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    // Verificar estadísticas
    auto stats = constexprEvaluator.getStats();
    EXPECT_EQ(stats.expressionsEvaluated, 1000);
    EXPECT_LT(duration.count(), 1000); // Debe ser razonablemente rápido

    std::cout << "Basic evaluation performance: " << duration.count() << "ms for 1000 expressions" << std::endl;
}

// Test para medir performance de evaluación compleja
TEST(ConstexprPerformanceTest, ComplexEvaluationPerformance) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    // Configurar límites
    constexprEvaluator.setLimits(1000000, 100, 1024 * 1024);

    auto startTime = std::chrono::high_resolution_clock::now();

    // Evaluar expresiones de diferentes tipos
    std::vector<ast::ASTNodeKind> expressionTypes = {
        ast::ASTNodeKind::IntegerLiteral,
        ast::ASTNodeKind::BooleanLiteral,
        ast::ASTNodeKind::CharacterLiteral,
        ast::ASTNodeKind::FloatingPointLiteral,
        ast::ASTNodeKind::StringLiteral,
        ast::ASTNodeKind::BinaryOp,
        ast::ASTNodeKind::UnaryOp
    };

    for (int i = 0; i < 500; ++i) {
        for (auto exprType : expressionTypes) {
            auto expression = std::make_unique<ast::ASTNode>(exprType);
            auto result = constexprEvaluator.evaluateExpression(expression.get());

            EXPECT_EQ(result.result, EvaluationResult::Success);
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    auto stats = constexprEvaluator.getStats();
    EXPECT_EQ(stats.expressionsEvaluated, 500 * 7); // 500 iteraciones * 7 tipos

    std::cout << "Complex evaluation performance: " << duration.count() << "ms for 3500 expressions" << std::endl;
}

// Test para medir límites de memoria
TEST(ConstexprPerformanceTest, MemoryLimitsPerformance) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    // Configurar límites de memoria bajos
    constexprEvaluator.setLimits(1000000, 100, 64 * 1024); // 64KB

    auto startTime = std::chrono::high_resolution_clock::now();

    // Evaluar expresiones hasta接近 límite de memoria
    for (int i = 0; i < 1000; ++i) {
        auto expression = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::IntegerLiteral);
        auto result = constexprEvaluator.evaluateExpression(expression.get());

        if (result.result != EvaluationResult::Success) {
            break; // Detener si se alcanza el límite
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    auto stats = constexprEvaluator.getStats();
    EXPECT_GE(stats.expressionsEvaluated, 0);

    std::cout << "Memory limits test completed in: " << duration.count() << "ms" << std::endl;
}

// Test para medir límites de recursión
TEST(ConstexprPerformanceTest, RecursionLimitsPerformance) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    // Configurar límites de recursión bajos
    constexprEvaluator.setLimits(1000000, 10, 1024 * 1024); // Recursión limitada

    auto startTime = std::chrono::high_resolution_clock::now();

    // Evaluar funciones que podrían causar recursión (simulado)
    for (int i = 0; i < 100; ++i) {
        auto functionBody = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::CompoundStmt);
        std::vector<ConstexprValue> args = {ConstexprValue(i)};

        auto result = constexprEvaluator.evaluateFunction("recursive_func", args, functionBody.get());

        EXPECT_TRUE(result.result == EvaluationResult::Success ||
                   result.result == EvaluationResult::RecursionLimit);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    auto stats = constexprEvaluator.getStats();
    EXPECT_GE(stats.functionsEvaluated, 0);

    std::cout << "Recursion limits test completed in: " << duration.count() << "ms" << std::endl;
}

// Test para medir performance de funciones complejas
TEST(ConstexprPerformanceTest, ComplexFunctionPerformance) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    constexprEvaluator.setLimits(1000000, 50, 1024 * 1024);

    // Registrar función compleja
    auto functionDecl = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::FunctionDecl);
    constexprEvaluator.registerConstexprFunction("complex_computation", functionDecl.get());

    auto startTime = std::chrono::high_resolution_clock::now();

    // Evaluar función compleja múltiples veces
    for (int i = 0; i < 100; ++i) {
        auto functionBody = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::CompoundStmt);
        std::vector<ConstexprValue> args = {ConstexprValue(i), ConstexprValue(i * 2)};

        auto result = constexprEvaluator.evaluateFunction("complex_computation", args, functionBody.get());

        EXPECT_EQ(result.result, EvaluationResult::Success);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    auto stats = constexprEvaluator.getStats();
    EXPECT_EQ(stats.functionsEvaluated, 100);

    std::cout << "Complex function performance: " << duration.count() << "ms for 100 evaluations" << std::endl;
}

// Test para medir escalabilidad con muchos templates
TEST(ConstexprPerformanceTest, TemplateScalabilityPerformance) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    constexprEvaluator.setLimits(1000000, 100, 1024 * 1024);

    auto startTime = std::chrono::high_resolution_clock::now();

    // Simular evaluación de muchos templates pequeños
    for (int i = 0; i < 1000; ++i) {
        // Crear expresión template-like
        auto expression = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::IntegerLiteral);

        // Crear contexto con variables template
        std::unordered_map<std::string, ConstexprValue> context;
        context["T" + std::to_string(i)] = ConstexprValue(i);
        context["N" + std::to_string(i)] = ConstexprValue(i * 2);

        auto result = constexprEvaluator.evaluateExpression(expression.get(), context);

        EXPECT_EQ(result.result, EvaluationResult::Success);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    auto stats = constexprEvaluator.getStats();
    EXPECT_EQ(stats.expressionsEvaluated, 1000);

    std::cout << "Template scalability performance: " << duration.count() << "ms for 1000 template evaluations" << std::endl;
}

// Test para medir consumo de memoria
TEST(ConstexprPerformanceTest, MemoryConsumptionPerformance) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    // Configurar límites de memoria progresivamente
    std::vector<size_t> memoryLimits = {16 * 1024, 32 * 1024, 64 * 1024, 128 * 1024};

    for (auto limit : memoryLimits) {
        constexprEvaluator.clear();
        constexprEvaluator.setLimits(1000000, 100, limit);

        auto startTime = std::chrono::high_resolution_clock::now();

        // Evaluar expresiones hasta alcanzar límite de memoria
        int expressionsEvaluated = 0;
        for (int i = 0; i < 10000; ++i) {
            auto expression = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::IntegerLiteral);
            auto result = constexprEvaluator.evaluateExpression(expression.get());

            if (result.result != EvaluationResult::Success) {
                break;
            }
            expressionsEvaluated++;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        auto stats = constexprEvaluator.getStats();
        EXPECT_EQ(stats.expressionsEvaluated, expressionsEvaluated);

        std::cout << "Memory limit " << (limit / 1024) << "KB: "
                  << expressionsEvaluated << " expressions in " << duration.count() << "ms" << std::endl;
    }
}

// Test para medir throughput
TEST(ConstexprPerformanceTest, ThroughputPerformance) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    constexprEvaluator.setLimits(10000000, 1000, 10 * 1024 * 1024); // Límites altos

    auto startTime = std::chrono::high_resolution_clock::now();

    // Evaluar el mayor número posible de expresiones en 1 segundo
    int expressionsEvaluated = 0;
    auto timeout = startTime + std::chrono::seconds(1);

    while (std::chrono::high_resolution_clock::now() < timeout) {
        auto expression = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::IntegerLiteral);
        auto result = constexprEvaluator.evaluateExpression(expression.get());

        if (result.result == EvaluationResult::Success) {
            expressionsEvaluated++;
        } else {
            break;
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    auto stats = constexprEvaluator.getStats();
    EXPECT_EQ(stats.expressionsEvaluated, expressionsEvaluated);

    double throughput = static_cast<double>(expressionsEvaluated) / (duration.count() / 1000.0);

    std::cout << "Throughput performance: " << throughput << " expressions/second" << std::endl;
    std::cout << "Total expressions: " << expressionsEvaluated << " in " << duration.count() << "ms" << std::endl;
}

// Test para medir latencia de primera evaluación
TEST(ConstexprPerformanceTest, FirstEvaluationLatency) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    // Limpiar cualquier estado previo
    constexprEvaluator.clear();

    auto startTime = std::chrono::high_resolution_clock::now();

    // Primera evaluación
    auto expression = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::IntegerLiteral);
    auto result = constexprEvaluator.evaluateExpression(expression.get());

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    EXPECT_EQ(result.result, EvaluationResult::Success);

    std::cout << "First evaluation latency: " << duration.count() << " microseconds" << std::endl;
    EXPECT_LT(duration.count(), 10000); // Debe ser razonablemente rápido (< 10ms)
}

// Test para medir latencia promedio
TEST(ConstexprPerformanceTest, AverageLatencyPerformance) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    constexprEvaluator.clear();

    const int numEvaluations = 1000;
    auto totalStartTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numEvaluations; ++i) {
        auto expression = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::IntegerLiteral);
        auto result = constexprEvaluator.evaluateExpression(expression.get());

        ASSERT_EQ(result.result, EvaluationResult::Success);
    }

    auto totalEndTime = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::microseconds>(totalEndTime - totalStartTime);

    double averageLatency = static_cast<double>(totalDuration.count()) / numEvaluations;

    auto stats = constexprEvaluator.getStats();
    EXPECT_EQ(stats.expressionsEvaluated, numEvaluations);

    std::cout << "Average latency: " << averageLatency << " microseconds per expression" << std::endl;
    std::cout << "Total time: " << totalDuration.count() << " microseconds for " << numEvaluations << " expressions" << std::endl;
}

// Test para medir estabilidad de performance
TEST(ConstexprPerformanceTest, PerformanceStability) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    constexprEvaluator.setLimits(1000000, 100, 1024 * 1024);

    const int numBatches = 10;
    const int expressionsPerBatch = 100;
    std::vector<long long> batchTimes;

    for (int batch = 0; batch < numBatches; ++batch) {
        auto batchStartTime = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < expressionsPerBatch; ++i) {
            auto expression = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::IntegerLiteral);
            auto result = constexprEvaluator.evaluateExpression(expression.get());

            ASSERT_EQ(result.result, EvaluationResult::Success);
        }

        auto batchEndTime = std::chrono::high_resolution_clock::now();
        auto batchDuration = std::chrono::duration_cast<std::chrono::milliseconds>(batchEndTime - batchStartTime);

        batchTimes.push_back(batchDuration.count());
    }

    // Calcular estadísticas de estabilidad
    double sum = 0.0;
    double minTime = *std::min_element(batchTimes.begin(), batchTimes.end());
    double maxTime = *std::max_element(batchTimes.begin(), batchTimes.end());

    for (auto time : batchTimes) {
        sum += time;
    }
    double averageTime = sum / numBatches;

    double variance = 0.0;
    for (auto time : batchTimes) {
        variance += (time - averageTime) * (time - averageTime);
    }
    variance /= numBatches;
    double stdDev = std::sqrt(variance);

    auto stats = constexprEvaluator.getStats();
    EXPECT_EQ(stats.expressionsEvaluated, numBatches * expressionsPerBatch);

    std::cout << "Performance stability statistics:" << std::endl;
    std::cout << "  Average time per batch: " << averageTime << "ms" << std::endl;
    std::cout << "  Min time: " << minTime << "ms" << std::endl;
    std::cout << "  Max time: " << maxTime << "ms" << std::endl;
    std::cout << "  Standard deviation: " << stdDev << "ms" << std::endl;
    std::cout << "  Coefficient of variation: " << (stdDev / averageTime * 100) << "%" << std::endl;

    // La desviación estándar debería ser razonablemente baja
    EXPECT_LT(stdDev / averageTime, 0.5); // Menos del 50% de variación
}

// Test para medir uso de CPU
TEST(ConstexprPerformanceTest, CPUUsageEstimation) {
    DiagnosticEngine diagEngine;
    ConstexprEvaluator constexprEvaluator(diagEngine);

    constexprEvaluator.setLimits(1000000, 100, 1024 * 1024);

    auto startTime = std::chrono::high_resolution_clock::now();
    auto startClock = std::clock();

    // Evaluar expresiones intensivas en CPU
    for (int i = 0; i < 10000; ++i) {
        auto expression = std::make_unique<ast::ASTNode>(ast::ASTNodeKind::IntegerLiteral);
        auto result = constexprEvaluator.evaluateExpression(expression.get());

        ASSERT_EQ(result.result, EvaluationResult::Success);
    }

    auto endClock = std::clock();
    auto endTime = std::chrono::high_resolution_clock::now();

    auto wallTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    double cpuTime = static_cast<double>(endClock - startClock) / CLOCKS_PER_SEC * 1000.0;

    double cpuUsage = (cpuTime / wallTime.count()) * 100.0;

    auto stats = constexprEvaluator.getStats();
    EXPECT_EQ(stats.expressionsEvaluated, 10000);

    std::cout << "CPU usage estimation:" << std::endl;
    std::cout << "  Wall time: " << wallTime.count() << "ms" << std::endl;
    std::cout << "  CPU time: " << cpuTime << "ms" << std::endl;
    std::cout << "  Estimated CPU usage: " << cpuUsage << "%" << std::endl;

    // El uso de CPU debería ser razonable (no excesivamente alto para operaciones simples)
    EXPECT_LT(cpuUsage, 150.0); // Menos del 150% (considerando hyperthreading)
}
