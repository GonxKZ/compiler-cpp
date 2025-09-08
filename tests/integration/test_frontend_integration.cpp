/**
 * @file test_frontend_integration.cpp
 * @brief Tests de integración para el front-end completo (Lexer + Preprocessor + Parser)
 */

#include <gtest/gtest.h>
#include <compiler/frontend/lexer/Lexer.h>
#include <compiler/frontend/Preprocessor.h>
#include <compiler/frontend/Parser.h>
#include <compiler/common/diagnostics/DiagnosticEngine.h>

using namespace cpp20::compiler::frontend;
using namespace cpp20::compiler::diagnostics;

// Test de integración completa: Lexer -> Preprocessor -> Parser
TEST(FrontendIntegrationTest, CompletePipelineTest) {
    std::string sourceCode = R"cpp(
        #define PI 3.14159
        #define SQUARE(x) ((x) * (x))

        int main() {
            double radius = 5.0;
            double area = PI * SQUARE(radius);
            return 0;
        }
    )cpp";

    DiagnosticEngine diagEngine;

    // Fase 1: Lexer
    lexer::Lexer lexer(sourceCode, diagEngine);
    auto tokens = lexer.tokenize();

    EXPECT_TRUE(tokens.size() > 1);
    EXPECT_EQ(tokens.back().getType(), lexer::TokenType::END_OF_FILE);

    // Verificar estadísticas del lexer
    auto lexerStats = lexer.getStats();
    EXPECT_GT(lexerStats.totalTokens, 10);
    EXPECT_EQ(lexerStats.errorCount, 0);

    // Fase 2: Preprocessor
    Preprocessor preprocessor(diagEngine);
    auto processedTokens = preprocessor.process(tokens);

    // Verificar que las macros fueron procesadas
    EXPECT_TRUE(processedTokens.size() > 1);

    // Verificar estadísticas del preprocesador
    auto prepStats = preprocessor.getStats();
    EXPECT_GT(prepStats.macrosDefined, 0);

    // Fase 3: Parser
    Parser parser(processedTokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
    EXPECT_TRUE(ast != nullptr);

    // Verificar estadísticas del parser
    auto parserStats = parser.getStats();
    EXPECT_GT(parserStats.nodesCreated, 1);
    EXPECT_EQ(parserStats.errorsReported, 0);
}

// Test de integración con expresiones complejas
TEST(FrontendIntegrationTest, ComplexExpressionTest) {
    std::string sourceCode = R"cpp(
        #define MAX(a,b) ((a) > (b) ? (a) : (b))

        int result = MAX(2 * 3 + 4, 5 * 6 - 7) * 2;
    )cpp";

    DiagnosticEngine diagEngine;

    // Pipeline completo
    lexer::Lexer lexer(sourceCode, diagEngine);
    auto tokens = lexer.tokenize();

    Preprocessor preprocessor(diagEngine);
    auto processedTokens = preprocessor.process(tokens);

    Parser parser(processedTokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
    EXPECT_TRUE(ast != nullptr);
}

// Test de integración con estructuras de control
TEST(FrontendIntegrationTest, ControlStructuresTest) {
    std::string sourceCode = R"cpp(
        #define CONDITION(x) ((x) > 0)

        int main() {
            int value = 10;

            if (CONDITION(value)) {
                return 1;
            } else if (value == 0) {
                return 0;
            } else {
                return -1;
            }

            while (value > 0) {
                value--;
            }

            return value;
        }
    )cpp";

    DiagnosticEngine diagEngine;

    lexer::Lexer lexer(sourceCode, diagEngine);
    auto tokens = lexer.tokenize();

    Preprocessor preprocessor(diagEngine);
    auto processedTokens = preprocessor.process(tokens);

    Parser parser(processedTokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
    EXPECT_TRUE(ast != nullptr);
}

// Test de integración con funciones
TEST(FrontendIntegrationTest, FunctionDeclarationTest) {
    std::string sourceCode = R"cpp(
        #define EXPORT extern "C"

        EXPORT int calculate(int a, int b);
        EXPORT double process(double value);

        int calculate(int a, int b) {
            return a + b * 2;
        }

        double process(double value) {
            if (value > 0) {
                return value * 2;
            } else {
                return 0;
            }
        }
    )cpp";

    DiagnosticEngine diagEngine;

    lexer::Lexer lexer(sourceCode, diagEngine);
    auto tokens = lexer.tokenize();

    Preprocessor preprocessor(diagEngine);
    auto processedTokens = preprocessor.process(tokens);

    Parser parser(processedTokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
    EXPECT_TRUE(ast != nullptr);
}

// Test de integración con clases C++
TEST(FrontendIntegrationTest, ClassDeclarationTest) {
    std::string sourceCode = R"cpp(
        #define PUBLIC public:
        #define PRIVATE private:

        class Calculator {
        PUBLIC
            Calculator(int initial) : value(initial) {}

            int add(int x) {
                return value += x;
            }

            int getValue() const {
                return value;
            }

        PRIVATE
            int value;
        };

        int main() {
            Calculator calc(10);
            calc.add(5);
            return calc.getValue();
        }
    )cpp";

    DiagnosticEngine diagEngine;

    lexer::Lexer lexer(sourceCode, diagEngine);
    auto tokens = lexer.tokenize();

    Preprocessor preprocessor(diagEngine);
    auto processedTokens = preprocessor.process(tokens);

    Parser parser(processedTokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
    EXPECT_TRUE(ast != nullptr);
}

// Test de integración con templates C++
TEST(FrontendIntegrationTest, TemplateTest) {
    std::string sourceCode = R"cpp(
        #define TEMPLATE template<typename T>

        TEMPLATE
        T max(T a, T b) {
            return a > b ? a : b;
        }

        TEMPLATE
        class Container {
        public:
            Container(T value) : data(value) {}

            T get() const {
                return data;
            }

        private:
            T data;
        };

        int main() {
            Container<int> container(42);
            int result = max(10, 20);
            return result + container.get();
        }
    )cpp";

    DiagnosticEngine diagEngine;

    lexer::Lexer lexer(sourceCode, diagEngine);
    auto tokens = lexer.tokenize();

    Preprocessor preprocessor(diagEngine);
    auto processedTokens = preprocessor.process(tokens);

    Parser parser(processedTokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
    EXPECT_TRUE(ast != nullptr);
}

// Test de integración con excepciones
TEST(FrontendIntegrationTest, ExceptionHandlingTest) {
    std::string sourceCode = R"cpp(
        #include <stdexcept>

        int divide(int a, int b) {
            if (b == 0) {
                throw std::runtime_error("Division by zero");
            }
            return a / b;
        }

        int main() {
            try {
                int result = divide(10, 0);
                return result;
            } catch (const std::exception& e) {
                return -1;
            }
        }
    )cpp";

    DiagnosticEngine diagEngine;

    lexer::Lexer lexer(sourceCode, diagEngine);
    auto tokens = lexer.tokenize();

    Preprocessor preprocessor(diagEngine);
    auto processedTokens = preprocessor.process(tokens);

    Parser parser(processedTokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
    EXPECT_TRUE(ast != nullptr);
}

// Test de integración con C++20 features
TEST(FrontendIntegrationTest, Cpp20FeaturesTest) {
    std::string sourceCode = R"cpp(
        #define CONSTEVAL constexpr
        #define SPACESHIP <=>

        CONSTEVAL int square(int x) {
            return x * x;
        }

        struct Point {
            int x, y;

            auto operator<=>(const Point&) const = default;
        };

        int main() {
            constexpr int value = square(5);
            Point p1{1, 2}, p2{1, 2};

            if (p1 SPACESHIP p2) {
                return 1;
            }

            return value;
        }
    )cpp";

    DiagnosticEngine diagEngine;

    lexer::Lexer lexer(sourceCode, diagEngine);
    auto tokens = lexer.tokenize();

    Preprocessor preprocessor(diagEngine);
    auto processedTokens = preprocessor.process(tokens);

    Parser parser(processedTokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
    EXPECT_TRUE(ast != nullptr);
}

// Test de integración con corutinas C++20
TEST(FrontendIntegrationTest, CoroutinesTest) {
    std::string sourceCode = R"cpp(
        #include <coroutine>

        struct Generator {
            struct promise_type {
                int current_value;

                Generator get_return_object() {
                    return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
                }

                std::suspend_always initial_suspend() { return {}; }
                std::suspend_always final_suspend() noexcept { return {}; }

                std::suspend_always yield_value(int value) {
                    current_value = value;
                    return {};
                }

                void return_void() {}
                void unhandled_exception() { std::terminate(); }
            };

            std::coroutine_handle<promise_type> handle;

            Generator(auto h) : handle(h) {}
            ~Generator() { if (handle) handle.destroy(); }

            int operator()() {
                handle.resume();
                return handle.promise().current_value;
            }
        };

        Generator fibonacci() {
            int a = 0, b = 1;
            while (true) {
                co_yield a;
                int temp = a;
                a = b;
                b = temp + b;
            }
        }

        int main() {
            auto gen = fibonacci();
            int sum = 0;
            for (int i = 0; i < 10; ++i) {
                sum += gen();
            }
            return sum;
        }
    )cpp";

    DiagnosticEngine diagEngine;

    lexer::Lexer lexer(sourceCode, diagEngine);
    auto tokens = lexer.tokenize();

    Preprocessor preprocessor(diagEngine);
    auto processedTokens = preprocessor.process(tokens);

    Parser parser(processedTokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
    EXPECT_TRUE(ast != nullptr);
}

// Test de integración con módulos C++20
TEST(FrontendIntegrationTest, ModulesTest) {
    std::string sourceCode = R"cpp(
        export module math;

        #define EXPORT export

        EXPORT int add(int a, int b) {
            return a + b;
        }

        EXPORT int multiply(int a, int b) {
            return a * b;
        }

        export class Calculator {
        public:
            int calculate(int a, int b, char op) {
                switch (op) {
                    case '+': return add(a, b);
                    case '*': return multiply(a, b);
                    default: return 0;
                }
            }
        };
    )cpp";

    DiagnosticEngine diagEngine;

    lexer::Lexer lexer(sourceCode, diagEngine);
    auto tokens = lexer.tokenize();

    Preprocessor preprocessor(diagEngine);
    auto processedTokens = preprocessor.process(tokens);

    Parser parser(processedTokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
    EXPECT_TRUE(ast != nullptr);
}

// Test de integración con conceptos C++20
TEST(FrontendIntegrationTest, ConceptsTest) {
    std::string sourceCode = R"cpp(
        #include <type_traits>

        template<typename T>
        concept Integral = std::is_integral_v<T>;

        template<typename T>
        concept SignedIntegral = Integral<T> && std::is_signed_v<T>;

        template<typename T>
        concept UnsignedIntegral = Integral<T> && !std::is_signed_v<T>;

        template<Integral T>
        T abs(T value) {
            if constexpr (std::is_signed_v<T>) {
                return value < 0 ? -value : value;
            } else {
                return value;
            }
        }

        template<typename T>
        requires Integral<T>
        T increment(T value) {
            return value + 1;
        }

        int main() {
            int x = 5;
            unsigned int y = 10;

            int result1 = abs(x);         // Usa SignedIntegral
            unsigned int result2 = abs(y); // Usa UnsignedIntegral

            int result3 = increment(x);
            unsigned int result4 = increment(y);

            return result1 + result2 + result3 + result4;
        }
    )cpp";

    DiagnosticEngine diagEngine;

    lexer::Lexer lexer(sourceCode, diagEngine);
    auto tokens = lexer.tokenize();

    Preprocessor preprocessor(diagEngine);
    auto processedTokens = preprocessor.process(tokens);

    Parser parser(processedTokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
    EXPECT_TRUE(ast != nullptr);
}

// Test de integración completo con error recovery
TEST(FrontendIntegrationTest, ErrorRecoveryTest) {
    std::string sourceCode = R"cpp(
        #define BAD_MACRO(x) x +

        int main() {
            int x = BAD_MACRO(5;  // Error: macro sin cerrar
            int y = 10;
            return x + y;  // Esto debería parsearse correctamente
        }
    )cpp";

    DiagnosticEngine diagEngine;

    lexer::Lexer lexer(sourceCode, diagEngine);
    auto tokens = lexer.tokenize();

    Preprocessor preprocessor(diagEngine);
    auto processedTokens = preprocessor.process(tokens);

    Parser parser(processedTokens, diagEngine);
    auto ast = parser.parse();

    // El parser debería manejar errores y continuar
    EXPECT_FALSE(parser.isSuccessful()); // Debería haber errores

    auto stats = parser.getStats();
    EXPECT_GT(stats.errorsReported, 0);
    EXPECT_TRUE(ast != nullptr); // Pero debería crear un AST
}

// Test de integración con archivos grandes
TEST(FrontendIntegrationTest, LargeFileTest) {
    std::string sourceCode;

    // Crear un archivo "grande" con múltiples funciones
    sourceCode += "#define FUNC(name) int name() { return 42; }\n\n";

    for (int i = 0; i < 50; ++i) {
        sourceCode += "FUNC(func" + std::to_string(i) + ")\n";
    }

    sourceCode += "\nint main() {\n";
    for (int i = 0; i < 50; ++i) {
        sourceCode += "    func" + std::to_string(i) + "();\n";
    }
    sourceCode += "    return 0;\n}\n";

    DiagnosticEngine diagEngine;

    lexer::Lexer lexer(sourceCode, diagEngine);
    auto tokens = lexer.tokenize();

    Preprocessor preprocessor(diagEngine);
    auto processedTokens = preprocessor.process(tokens);

    Parser parser(processedTokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
    EXPECT_TRUE(ast != nullptr);

    // Verificar estadísticas
    auto lexerStats = lexer.getStats();
    auto parserStats = parser.getStats();

    EXPECT_GT(lexerStats.totalTokens, 100);
    EXPECT_GT(parserStats.nodesCreated, 50);
}

// Test de integración con rendimiento
TEST(FrontendIntegrationTest, PerformanceTest) {
    // Crear código con expresiones complejas
    std::string sourceCode = "int result = ";

    for (int i = 0; i < 100; ++i) {
        if (i > 0) sourceCode += " + ";
        sourceCode += "(a" + std::to_string(i) + " * b" + std::to_string(i) + ")";
    }

    sourceCode += ";";

    DiagnosticEngine diagEngine;

    lexer::Lexer lexer(sourceCode, diagEngine);
    auto tokens = lexer.tokenize();

    Preprocessor preprocessor(diagEngine);
    auto processedTokens = preprocessor.process(tokens);

    Parser parser(processedTokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
    EXPECT_TRUE(ast != nullptr);
}

// Test de integración final: programa completo
TEST(FrontendIntegrationTest, CompleteProgramTest) {
    std::string sourceCode = R"cpp(
        #include <iostream>

        #define PI 3.141592653589793
        #define SQUARE(x) ((x) * (x))
        #define CIRCLE_AREA(r) (PI * SQUARE(r))

        class Shape {
        public:
            virtual double area() const = 0;
            virtual ~Shape() = default;
        };

        class Circle : public Shape {
        private:
            double radius;

        public:
            Circle(double r) : radius(r) {}

            double area() const override {
                return CIRCLE_AREA(radius);
            }
        };

        template<typename T>
        T max(T a, T b) {
            return a > b ? a : b;
        }

        int main() {
            Circle circle(5.0);
            double area = circle.area();

            int a = 10, b = 20;
            int maximum = max(a, b);

            if (area > 0 && maximum > 0) {
                return 0;
            } else {
                return 1;
            }
        }
    )cpp";

    DiagnosticEngine diagEngine;

    // Pipeline completo
    lexer::Lexer lexer(sourceCode, diagEngine);
    auto tokens = lexer.tokenize();

    Preprocessor preprocessor(diagEngine);
    auto processedTokens = preprocessor.process(tokens);

    Parser parser(processedTokens, diagEngine);
    auto ast = parser.parse();

    // Verificaciones finales
    EXPECT_TRUE(parser.isSuccessful());
    EXPECT_TRUE(ast != nullptr);

    // Verificar estadísticas
    auto lexerStats = lexer.getStats();
    auto parserStats = parser.getStats();

    EXPECT_GT(lexerStats.totalTokens, 50);
    EXPECT_GT(parserStats.nodesCreated, 10);
    EXPECT_EQ(lexerStats.errorCount, 0);
    EXPECT_EQ(parserStats.errorsReported, 0);

    std::cout << "✅ Pipeline completo ejecutado exitosamente:" << std::endl;
    std::cout << "  - Tokens procesados: " << lexerStats.totalTokens << std::endl;
    std::cout << "  - Nodos AST creados: " << parserStats.nodesCreated << std::endl;
    std::cout << "  - Líneas de código: " << lexerStats.totalLines << std::endl;
}
