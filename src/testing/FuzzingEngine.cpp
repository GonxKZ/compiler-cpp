/**
 * @file FuzzingEngine.cpp
 * @brief Implementación del motor de fuzzing para testing del compilador
 */

#include <compiler/testing/FuzzingEngine.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <future>
#include <regex>
#include <cstring>

namespace cpp20::compiler::testing {

// ============================================================================
// FuzzInputGenerator - Implementación
// ============================================================================

FuzzInputGenerator::FuzzInputGenerator(size_t seed)
    : rng_(seed), lengthDist_(1, 1024), mutationDist_(0.0, 1.0) {
    // Inicializar corpus con algunos seeds básicos
    corpus_ = {
        "#include <iostream>\nint main() { return 0; }",
        "template<typename T> T add(T a, T b) { return a + b; }",
        "class MyClass { public: int value; };",
        "constexpr int factorial(int n) { return n <= 1 ? 1 : n * factorial(n - 1); }",
        "void func() { throw std::runtime_error(\"error\"); }",
        "export module math; export int square(int x) { return x * x; }",
        "int main() { std::cout << \"Hello\" << std::endl; return 0; }",
        "namespace ns { class C { virtual void f() {} }; }",
        "auto lambda = []() { return 42; };",
        "concept Integral = std::is_integral_v<T>;",
        "std::vector<int> v = {1, 2, 3};",
        "if constexpr (sizeof(T) > 4) { /* code */ }",
        "co_await std::suspend_always{};",
        "std::unique_ptr<int> p = std::make_unique<int>(42);"
    };
}

FuzzInputGenerator::~FuzzInputGenerator() = default;

std::string FuzzInputGenerator::generateRandomInput(size_t maxLength) {
    std::string result;
    size_t length = std::min(lengthDist_(rng_), maxLength);

    // Generar caracteres aleatorios válidos
    std::uniform_int_distribution<int> charDist(32, 126); // Caracteres imprimibles

    for (size_t i = 0; i < length; ++i) {
        char c = static_cast<char>(charDist(rng_));

        // Evitar algunos caracteres problemáticos
        if (c == '`' || c == '\x7F') {
            c = ' ';
        }

        result += c;
    }

    return result;
}

std::string FuzzInputGenerator::generateGrammarBasedInput(FuzzTarget target, size_t complexity) {
    std::string result;

    switch (target) {
        case FuzzTarget::Lexer:
            result = generateRandomTokens(complexity * 10);
            break;
        case FuzzTarget::Parser:
            result = generateRandomDeclaration();
            for (size_t i = 1; i < complexity; ++i) {
                result += "\n" + generateRandomDeclaration();
            }
            break;
        case FuzzTarget::Preprocessor:
            result = generatePreprocessorInput(complexity);
            break;
        case FuzzTarget::Semantic:
            result = generateSemanticInput(complexity);
            break;
        case FuzzTarget::CodeGen:
            result = generateCodeGenInput(complexity);
            break;
        case FuzzTarget::FullPipeline:
            result = generateFullPipelineInput(complexity);
            break;
    }

    return result;
}

std::string FuzzInputGenerator::mutateInput(const std::string& input, double mutationRate) {
    if (input.empty()) {
        return generateRandomInput(100);
    }

    return applyMutations(input, mutationRate);
}

std::string FuzzInputGenerator::generateTargetedInput(FuzzTarget target) {
    return generateGrammarBasedInput(target, 2);
}

bool FuzzInputGenerator::loadCorpus(const std::filesystem::path& corpusDir) {
    if (!std::filesystem::exists(corpusDir) || !std::filesystem::is_directory(corpusDir)) {
        return false;
    }

    corpus_.clear();

    for (const auto& entry : std::filesystem::directory_iterator(corpusDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".cpp") {
            std::ifstream file(entry.path());
            if (file.is_open()) {
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                corpus_.push_back(content);
                file.close();
            }
        }
    }

    return !corpus_.empty();
}

void FuzzInputGenerator::addSeed(const std::string& seed) {
    if (!seed.empty()) {
        corpus_.push_back(seed);
    }
}

std::string FuzzInputGenerator::getRandomSeed() {
    if (corpus_.empty()) {
        return "#include <iostream>\nint main() { return 0; }";
    }

    std::uniform_int_distribution<size_t> dist(0, corpus_.size() - 1);
    return corpus_[dist(rng_)];
}

std::string FuzzInputGenerator::generateRandomTokens(size_t count) {
    std::vector<std::string> tokens = {
        "int", "void", "char", "float", "double", "class", "struct", "union",
        "if", "else", "for", "while", "do", "switch", "case", "default",
        "public", "private", "protected", "virtual", "override", "final",
        "const", "volatile", "static", "inline", "extern", "mutable",
        "+", "-", "*", "/", "%", "=", "==", "!=", "<", ">", "<=", ">=",
        "&&", "||", "!", "&", "|", "^", "~", "<<", ">>",
        "(", ")", "[", "]", "{", "}", ";", ",", ".", "->", "::",
        "template", "typename", "using", "namespace", "auto", "decltype",
        "constexpr", "consteval", "concept", "requires", "co_await",
        "std", "vector", "string", "unique_ptr", "shared_ptr"
    };

    std::string result;
    std::uniform_int_distribution<size_t> tokenDist(0, tokens.size() - 1);
    std::uniform_int_distribution<size_t> identDist(1, 10);

    for (size_t i = 0; i < count; ++i) {
        if (i > 0) result += " ";

        // 30% chance de generar identificador aleatorio
        if (std::uniform_real_distribution<double>(0.0, 1.0)(rng_) < 0.3) {
            result += generateRandomIdentifier();
        } else {
            result += tokens[tokenDist(rng_)];
        }
    }

    return result;
}

std::string FuzzInputGenerator::generateRandomExpression(size_t depth) {
    if (depth == 0) {
        // Generar literal o identificador
        if (std::uniform_real_distribution<double>(0.0, 1.0)(rng_) < 0.5) {
            return generateRandomLiteral();
        } else {
            return generateRandomIdentifier();
        }
    }

    std::vector<std::string> operators = {"+", "-", "*", "/", "%", "==", "!=", "<", ">"};
    std::uniform_int_distribution<size_t> opDist(0, operators.size() - 1);

    std::string left = generateRandomExpression(depth - 1);
    std::string op = operators[opDist(rng_)];
    std::string right = generateRandomExpression(depth - 1);

    return "(" + left + " " + op + " " + right + ")";
}

std::string FuzzInputGenerator::generateRandomDeclaration() {
    std::vector<std::string> types = {"int", "void", "char", "float", "double", "auto"};
    std::uniform_int_distribution<size_t> typeDist(0, types.size() - 1);

    std::string type = types[typeDist(rng_)];
    std::string name = generateRandomIdentifier();

    // 50% chance de función, 50% de variable
    if (std::uniform_real_distribution<double>(0.0, 1.0)(rng_) < 0.5) {
        // Función
        return type + " " + name + "() { return " + generateRandomLiteral() + "; }";
    } else {
        // Variable
        return type + " " + name + " = " + generateRandomLiteral() + ";";
    }
}

std::string FuzzInputGenerator::generateRandomIdentifier() {
    std::string result;
    std::uniform_int_distribution<int> firstCharDist('a', 'z');
    std::uniform_int_distribution<int> charDist('a', 'z');
    std::uniform_int_distribution<int> digitDist('0', '9');
    std::uniform_int_distribution<size_t> lengthDist(1, 10);

    size_t length = lengthDist(rng_);
    result += static_cast<char>(firstCharDist(rng_));

    for (size_t i = 1; i < length; ++i) {
        if (std::uniform_real_distribution<double>(0.0, 1.0)(rng_) < 0.8) {
            result += static_cast<char>(charDist(rng_));
        } else {
            result += static_cast<char>(digitDist(rng_));
        }
    }

    return result;
}

std::string FuzzInputGenerator::generateRandomLiteral() {
    std::vector<std::string> literals = {
        "0", "1", "42", "-1", "3.14", "-2.71", "\"hello\"", "'a'",
        "true", "false", "nullptr"
    };

    std::uniform_int_distribution<size_t> dist(0, literals.size() - 1);
    return literals[dist(rng_)];
}

std::string FuzzInputGenerator::applyMutations(const std::string& input, double mutationRate) {
    std::string result = input;

    // Aplicar diferentes tipos de mutaciones
    if (mutationDist_(rng_) < mutationRate) {
        result = insertRandomBytes(result);
    }
    if (mutationDist_(rng_) < mutationRate) {
        result = deleteRandomBytes(result);
    }
    if (mutationDist_(rng_) < mutationRate) {
        result = modifyRandomBytes(result);
    }
    if (mutationDist_(rng_) < mutationRate) {
        result = duplicateRandomSection(result);
    }

    return result;
}

std::string FuzzInputGenerator::insertRandomBytes(const std::string& input) {
    if (input.empty()) return input;

    std::string result = input;
    std::uniform_int_distribution<size_t> posDist(0, result.size());
    std::uniform_int_distribution<int> charDist(32, 126);

    size_t pos = posDist(rng_);
    char newChar = static_cast<char>(charDist(rng_));
    result.insert(pos, 1, newChar);

    return result;
}

std::string FuzzInputGenerator::deleteRandomBytes(const std::string& input) {
    if (input.size() <= 1) return input;

    std::string result = input;
    std::uniform_int_distribution<size_t> posDist(0, result.size() - 1);

    size_t pos = posDist(rng_);
    result.erase(pos, 1);

    return result;
}

std::string FuzzInputGenerator::modifyRandomBytes(const std::string& input) {
    if (input.empty()) return input;

    std::string result = input;
    std::uniform_int_distribution<size_t> posDist(0, result.size() - 1);
    std::uniform_int_distribution<int> charDist(32, 126);

    size_t pos = posDist(rng_);
    char newChar = static_cast<char>(charDist(rng_));
    result[pos] = newChar;

    return result;
}

std::string FuzzInputGenerator::duplicateRandomSection(const std::string& input) {
    if (input.size() <= 1) return input;

    std::string result = input;
    std::uniform_int_distribution<size_t> startDist(0, result.size() - 1);
    std::uniform_int_distribution<size_t> lengthDist(1, std::min(size_t(10), result.size()));

    size_t start = startDist(rng_);
    size_t length = std::min(lengthDist(rng_), result.size() - start);
    std::string section = result.substr(start, length);

    std::uniform_int_distribution<size_t> insertPosDist(0, result.size());
    size_t insertPos = insertPosDist(rng_);
    result.insert(insertPos, section);

    return result;
}

std::string FuzzInputGenerator::generatePreprocessorInput(size_t complexity) {
    std::string result = "#include <iostream>\n";

    for (size_t i = 0; i < complexity; ++i) {
        std::vector<std::string> directives = {
            "#define " + generateRandomIdentifier() + " " + generateRandomLiteral(),
            "#ifdef " + generateRandomIdentifier(),
            "#ifndef " + generateRandomIdentifier(),
            "#if " + generateRandomExpression(1),
            "#elif " + generateRandomExpression(1),
            "#else",
            "#endif",
            "#pragma once"
        };

        std::uniform_int_distribution<size_t> dirDist(0, directives.size() - 1);
        result += directives[dirDist(rng_)] + "\n";
    }

    result += "\nint main() { return 0; }\n";
    return result;
}

std::string FuzzInputGenerator::generateSemanticInput(size_t complexity) {
    std::string result = "#include <iostream>\n#include <vector>\n\n";

    for (size_t i = 0; i < complexity; ++i) {
        result += "template<typename T>\n";
        result += "T " + generateRandomIdentifier() + "(T x) {\n";
        result += "    return x " + generateRandomExpression(1) + ";\n";
        result += "}\n\n";
    }

    result += "int main() {\n";
    result += "    auto result = " + generateRandomIdentifier() + "<int>(42);\n";
    result += "    std::cout << result << std::endl;\n";
    result += "    return 0;\n";
    result += "}\n";

    return result;
}

std::string FuzzInputGenerator::generateCodeGenInput(size_t complexity) {
    std::string result = "#include <iostream>\n\n";

    for (size_t i = 0; i < complexity; ++i) {
        result += "struct S" + std::to_string(i) + " {\n";
        result += "    int x, y;\n";
        result += "    virtual void f" + std::to_string(i) + "() {}\n";
        result += "};\n\n";
    }

    result += "class Complex : ";
    for (size_t i = 0; i < complexity && i < 3; ++i) {
        if (i > 0) result += ", ";
        result += "public S" + std::to_string(i);
    }
    result += " {\npublic:\n";

    for (size_t i = 0; i < complexity; ++i) {
        result += "    void f" + std::to_string(i) + "() override {}\n";
    }

    result += "};\n\n";
    result += "int main() { Complex c; return 0; }\n";

    return result;
}

std::string FuzzInputGenerator::generateFullPipelineInput(size_t complexity) {
    std::string result = "#include <iostream>\n#include <vector>\n#include <memory>\n\n";

    // Añadir algunos templates complejos
    result += "template<typename T, typename U = int>\n";
    result += "class Container {\n";
    result += "    std::vector<T> data;\n";
    result += "    U metadata;\n";
    result += "public:\n";
    result += "    void add(T item) { data.push_back(item); }\n";
    result += "    T get(size_t index) const { return data[index]; }\n";
    result += "};\n\n";

    // Concept
    result += "template<typename T>\n";
    result += "concept Numeric = std::is_arithmetic_v<T>;\n\n";

    // Función con constexpr
    result += "template<Numeric T>\n";
    result += "constexpr T square(T x) { return x * x; }\n\n";

    // Clase con herencia múltiple
    result += "class Base1 { public: virtual ~Base1() = default; int x = 1; };\n";
    result += "class Base2 { public: virtual ~Base2() = default; int y = 2; };\n";
    result += "class Derived : public Base1, public Base2 {\n";
    result += "public:\n";
    result += "    int z = 3;\n";
    result += "    void print() const {\n";
    result += "        std::cout << x << \" \" << y << \" \" << z << std::endl;\n";
    result += "    }\n";
    result += "};\n\n";

    // Función con excepciones
    result += "void risky_function() {\n";
    result += "    throw std::runtime_error(\"Something went wrong\");\n";
    result += "}\n\n";

    // Función con corrutinas
    result += "std::generator<int> fibonacci() {\n";
    result += "    int a = 0, b = 1;\n";
    result += "    while (true) {\n";
    result += "        co_yield a;\n";
    result += "        int temp = a;\n";
    result += "        a = b;\n";
    result += "        b = temp + b;\n";
    result += "    }\n";
    result += "}\n\n";

    // Función main
    result += "int main() {\n";
    result += "    try {\n";
    result += "        Container<int> c;\n";
    result += "        c.add(42);\n";
    result += "        std::cout << \"Square: \" << square(5) << std::endl;\n";
    result += "        std::cout << \"Value: \" << c.get(0) << std::endl;\n";
    result += "        \n";
    result += "        Derived d;\n";
    result += "        d.print();\n";
    result += "        \n";
    result += "        auto gen = fibonacci();\n";
    result += "        for (int i = 0; i < 5; ++i) {\n";
    result += "            std::cout << gen.next() << \" \";\n";
    result += "        }\n";
    result += "        std::cout << std::endl;\n";
    result += "        \n";
    result += "        risky_function();\n";
    result += "    } catch (const std::exception& e) {\n";
    result += "        std::cout << \"Error: \" << e.what() << std::endl;\n";
    result += "    }\n";
    result += "    return 0;\n";
    result += "}\n";

    return result;
}

// ============================================================================
// FuzzExecutor - Implementación
// ============================================================================

FuzzExecutor::FuzzExecutor() = default;

FuzzExecutor::~FuzzExecutor() = default;

FuzzResult FuzzExecutor::executeFuzzInput(const std::string& input,
                                         FuzzTarget target,
                                         std::chrono::milliseconds timeout) {
    FuzzResult result(input, target);

    auto startTime = std::chrono::steady_clock::now();

    try {
        switch (target) {
            case FuzzTarget::Lexer:
                result = fuzzLexer(input);
                break;
            case FuzzTarget::Parser:
                result = fuzzParser(input);
                break;
            case FuzzTarget::Preprocessor:
                result = fuzzPreprocessor(input);
                break;
            case FuzzTarget::Semantic:
                result = fuzzSemantic(input);
                break;
            case FuzzTarget::CodeGen:
                result = fuzzCodeGen(input);
                break;
            case FuzzTarget::FullPipeline:
                result = fuzzFullPipeline(input);
                break;
        }
    } catch (const std::exception& e) {
        result.isCrash = true;
        result.errorType = "Exception";
        result.errorMessage = std::string("Exception caught: ") + e.what();
        result.stackTrace = getStackTrace();
    }

    auto endTime = std::chrono::steady_clock::now();
    result.executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    // Verificar timeout
    if (detectHang(result.executionTime, timeout)) {
        result.isHang = true;
        result.errorType = "Hang";
        result.errorMessage = "Execution timed out";
    }

    return result;
}

bool FuzzExecutor::validateCompilerBehavior(const std::string& input, FuzzTarget target) {
    FuzzResult result = executeFuzzInput(input, target);

    // El compilador debe terminar sin crashes o hangs
    return !result.isCrash && !result.isHang;
}

bool FuzzExecutor::detectCrash(const std::string& output, int exitCode) {
    // Detectar crashes basado en código de salida y contenido
    if (exitCode != 0 && exitCode != 1) { // 0 = éxito, 1 = error de compilación normal
        return true;
    }

    // Buscar patrones de crash en la salida
    std::vector<std::string> crashPatterns = {
        "segmentation fault",
        "access violation",
        "stack overflow",
        "assertion failed",
        "fatal error",
        "internal compiler error"
    };

    std::string lowerOutput = output;
    std::transform(lowerOutput.begin(), lowerOutput.end(), lowerOutput.begin(), ::tolower);

    for (const auto& pattern : crashPatterns) {
        if (lowerOutput.find(pattern) != std::string::npos) {
            return true;
        }
    }

    return false;
}

bool FuzzExecutor::detectHang(std::chrono::milliseconds executionTime,
                             std::chrono::milliseconds timeout) {
    return executionTime >= timeout;
}

std::string FuzzExecutor::getStackTrace() {
    // En una implementación real, aquí se capturaría el stack trace
    // usando bibliotecas como backward-cpp o boost.stacktrace
    return "Stack trace not available in this implementation";
}

std::string FuzzExecutor::minimizeInput(const std::string& crashingInput, FuzzTarget target) {
    // Implementación simple de minimización de input
    // En una implementación real, se usaría algoritmos más sofisticados
    // como delta debugging

    std::string minimized = crashingInput;

    // Intentar eliminar partes del input mientras se preserve el crash
    for (size_t i = 0; i < minimized.size(); ++i) {
        std::string testInput = minimized;
        testInput.erase(i, 1);

        if (!validateCompilerBehavior(testInput, target)) {
            // El crash se preserva, continuar con input más pequeño
            minimized = testInput;
            i = 0; // Reiniciar búsqueda
        }
    }

    return minimized;
}

FuzzResult FuzzExecutor::fuzzLexer(const std::string& input) {
    FuzzResult result(input, FuzzTarget::Lexer);

    // En una implementación real, aquí se probaría solo el lexer
    // Simular ejecución
    auto [output, exitCode, execTime] = executeCommand("echo Testing lexer", std::chrono::seconds(1));

    result.executionTime = execTime;
    result.isCrash = detectCrash(output, exitCode);

    if (result.isCrash) {
        result.errorType = "Lexer Crash";
        result.errorMessage = "Lexer crashed with input: " + input.substr(0, 50) + "...";
    }

    return result;
}

FuzzResult FuzzExecutor::fuzzParser(const std::string& input) {
    FuzzResult result(input, FuzzTarget::Parser);

    // Simular ejecución del parser
    auto [output, exitCode, execTime] = executeCommand("echo Testing parser", std::chrono::seconds(1));

    result.executionTime = execTime;
    result.isCrash = detectCrash(output, exitCode);

    if (result.isCrash) {
        result.errorType = "Parser Crash";
        result.errorMessage = "Parser crashed with input: " + input.substr(0, 50) + "...";
    }

    return result;
}

FuzzResult FuzzExecutor::fuzzPreprocessor(const std::string& input) {
    FuzzResult result(input, FuzzTarget::Preprocessor);

    // Simular ejecución del preprocesador
    auto [output, exitCode, execTime] = executeCommand("echo Testing preprocessor", std::chrono::seconds(1));

    result.executionTime = execTime;
    result.isCrash = detectCrash(output, exitCode);

    if (result.isCrash) {
        result.errorType = "Preprocessor Crash";
        result.errorMessage = "Preprocessor crashed with input: " + input.substr(0, 50) + "...";
    }

    return result;
}

FuzzResult FuzzExecutor::fuzzSemantic(const std::string& input) {
    FuzzResult result(input, FuzzTarget::Semantic);

    // Simular ejecución del análisis semántico
    auto [output, exitCode, execTime] = executeCommand("echo Testing semantic analysis", std::chrono::seconds(1));

    result.executionTime = execTime;
    result.isCrash = detectCrash(output, exitCode);

    if (result.isCrash) {
        result.errorType = "Semantic Crash";
        result.errorMessage = "Semantic analysis crashed with input: " + input.substr(0, 50) + "...";
    }

    return result;
}

FuzzResult FuzzExecutor::fuzzCodeGen(const std::string& input) {
    FuzzResult result(input, FuzzTarget::CodeGen);

    // Simular ejecución del generador de código
    auto [output, exitCode, execTime] = executeCommand("echo Testing code generation", std::chrono::seconds(1));

    result.executionTime = execTime;
    result.isCrash = detectCrash(output, exitCode);

    if (result.isCrash) {
        result.errorType = "CodeGen Crash";
        result.errorMessage = "Code generation crashed with input: " + input.substr(0, 50) + "...";
    }

    return result;
}

FuzzResult FuzzExecutor::fuzzFullPipeline(const std::string& input) {
    FuzzResult result(input, FuzzTarget::FullPipeline);

    // Simular ejecución de toda la pipeline
    auto [output, exitCode, execTime] = executeCommand("echo Testing full pipeline", std::chrono::seconds(1));

    result.executionTime = execTime;
    result.isCrash = detectCrash(output, exitCode);

    if (result.isCrash) {
        result.errorType = "Full Pipeline Crash";
        result.errorMessage = "Full pipeline crashed with input: " + input.substr(0, 50) + "...";
    }

    return result;
}

std::tuple<std::string, int, std::chrono::milliseconds> FuzzExecutor::executeCommand(
    const std::string& command, std::chrono::milliseconds timeout) {

    auto startTime = std::chrono::steady_clock::now();

    // En una implementación real, aquí se ejecutaría el comando real
    // con captura de salida y timeout
    std::string output = "Simulated compiler output";
    int exitCode = 0;

    auto endTime = std::chrono::steady_clock::now();
    auto execTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    return {output, exitCode, execTime};
}

bool FuzzExecutor::validateOutput(const std::string& output, int exitCode) {
    // Validar que la salida es razonable
    return !output.empty() && exitCode >= 0;
}

std::string FuzzExecutor::detectErrorPattern(const std::string& output, int exitCode) {
    // Detectar patrones de error específicos
    if (exitCode == 139) return "Segmentation Fault";
    if (exitCode == 134) return "Assertion Failed";
    if (exitCode == 11) return "Segmentation Fault (SIGSEGV)";

    if (output.find("segmentation") != std::string::npos) return "Segmentation Fault";
    if (output.find("assertion") != std::string::npos) return "Assertion Failed";
    if (output.find("stack overflow") != std::string::npos) return "Stack Overflow";

    return "Unknown Error";
}

// ============================================================================
// FuzzingEngine - Implementación
// ============================================================================

FuzzingEngine::FuzzingEngine(FuzzTarget target, FuzzStrategy strategy)
    : target_(target), strategy_(strategy), maxInputSize_(4096),
      timeout_(std::chrono::seconds(5)), mutationRate_(0.1),
      verbose_(false), seed_(std::random_device{}()) {

    initializeGenerators();
}

FuzzingEngine::~FuzzingEngine() = default;

FuzzStatistics FuzzingEngine::runFuzzing(size_t numIterations,
                                        std::chrono::minutes duration) {
    auto startTime = std::chrono::steady_clock::now();
    size_t iteration = 0;

    std::cout << "Starting fuzzing session with " << numIterations << " iterations" << std::endl;

    while (iteration < numIterations && checkTimeLimits(startTime, duration)) {
        FuzzResult result = runIteration(iteration);
        processResult(result);

        if (verbose_ && iteration % 100 == 0) {
            reportProgress(iteration, result);
        }

        iteration++;
    }

    // Minimizar crashes encontrados
    minimizeCrashes();

    std::cout << "Fuzzing session completed. Found " << statistics_.crashesFound << " crashes." << std::endl;

    return statistics_;
}

void FuzzingEngine::configure(size_t maxInputSize,
                             std::chrono::milliseconds timeout,
                             double mutationRate) {
    maxInputSize_ = maxInputSize;
    timeout_ = timeout;
    mutationRate_ = mutationRate;
}

bool FuzzingEngine::loadCorpus(const std::filesystem::path& corpusDir) {
    return inputGenerator_->loadCorpus(corpusDir);
}

void FuzzingEngine::saveCrashes(const std::filesystem::path& outputDir) const {
    FuzzUtils::ensureDirectory(outputDir);

    for (size_t i = 0; i < crashes_.size(); ++i) {
        const auto& crash = crashes_[i];
        std::string filename = FuzzUtils::generateCrashFileName(crash);
        std::filesystem::path crashFile = outputDir / filename;

        std::ofstream file(crashFile);
        if (file.is_open()) {
            file << "// Crash input\n";
            file << crash.input << "\n\n";
            file << "// Error details\n";
            file << "Type: " << crash.errorType << "\n";
            file << "Message: " << crash.errorMessage << "\n";
            file << "Stack trace:\n" << crash.stackTrace << "\n";
            file.close();
        }
    }
}

void FuzzingEngine::setSeed(size_t seed) {
    seed_ = seed;
    // Reinicializar generadores con nueva semilla
    initializeGenerators();
}

void FuzzingEngine::initializeGenerators() {
    inputGenerator_ = std::make_unique<FuzzInputGenerator>(seed_);
    executor_ = std::make_unique<FuzzExecutor>();
}

FuzzResult FuzzingEngine::runIteration(size_t iteration) {
    std::string input;

    // Generar input según estrategia
    switch (strategy_) {
        case FuzzStrategy::Random:
            input = inputGenerator_->generateRandomInput(maxInputSize_);
            break;
        case FuzzStrategy::Mutational: {
            std::string seed = inputGenerator_->getRandomSeed();
            input = inputGenerator_->mutateInput(seed, mutationRate_);
            break;
        }
        case FuzzStrategy::GrammarBased:
            input = inputGenerator_->generateGrammarBasedInput(target_, 2);
            break;
        case FuzzStrategy::CoverageGuided:
            // En una implementación real, aquí se usaría información de cobertura
            input = inputGenerator_->generateTargetedInput(target_);
            break;
    }

    // Ejecutar fuzzing
    return executor_->executeFuzzInput(input, target_, timeout_);
}

void FuzzingEngine::processResult(const FuzzResult& result) {
    updateStatistics(result);

    if (result.isCrash || result.isHang) {
        if (isNewCrash(result)) {
            crashes_.push_back(result);

            if (verbose_) {
                std::cout << "Found new crash: " << result.errorType << std::endl;
            }
        }
    }
}

bool FuzzingEngine::isNewCrash(const FuzzResult& result) {
    // Verificar si es un crash único
    return FuzzUtils::isUniqueCrash(result, crashes_);
}

void FuzzingEngine::updateStatistics(const FuzzResult& result) {
    statistics_.totalInputs++;
    statistics_.totalTime += result.executionTime;

    if (result.isCrash) {
        statistics_.crashesFound++;
        statistics_.errorCounts[result.errorType]++;
    }

    if (result.isHang) {
        statistics_.hangsFound++;
    }
}

void FuzzingEngine::reportProgress(size_t iteration, const FuzzResult& result) {
    std::cout << "Iteration " << iteration << ": ";
    if (result.isCrash) {
        std::cout << "CRASH (" << result.errorType << ")";
    } else if (result.isHang) {
        std::cout << "HANG";
    } else {
        std::cout << "OK";
    }
    std::cout << " [" << result.executionTime.count() << "ms]" << std::endl;
}

bool FuzzingEngine::checkTimeLimits(std::chrono::steady_clock::time_point startTime,
                                   std::chrono::minutes maxDuration) {
    auto currentTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(currentTime - startTime);
    return elapsed < maxDuration;
}

void FuzzingEngine::minimizeCrashes() {
    for (auto& crash : crashes_) {
        if (crash.isCrash) {
            std::string minimized = executor_->minimizeInput(crash.input, crash.target);
            if (minimized.size() < crash.input.size()) {
                crash.input = minimized;
                crash.inputSize = minimized.size();
            }
        }
    }
}

void FuzzingEngine::cleanup() {
    crashes_.clear();
    statistics_ = FuzzStatistics();
}

// ============================================================================
// CorpusManager - Implementación
// ============================================================================

CorpusManager::CorpusManager(const std::filesystem::path& corpusDir)
    : corpusDir_(corpusDir) {
    std::filesystem::create_directories(corpusDir_);
}

CorpusManager::~CorpusManager() = default;

bool CorpusManager::loadCorpus() {
    if (!std::filesystem::exists(corpusDir_)) {
        return false;
    }

    entries_.clear();
    metadata_.clear();

    for (const auto& entry : std::filesystem::directory_iterator(corpusDir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            std::ifstream file(entry.path());
            if (file.is_open()) {
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                entries_.push_back(content);
                metadata_.push_back(entry.path().filename().string());
                file.close();
            }
        }
    }

    return !entries_.empty();
}

bool CorpusManager::saveCorpus() const {
    for (size_t i = 0; i < entries_.size(); ++i) {
        std::string filename = "entry_" + std::to_string(i) + ".txt";
        std::filesystem::path filePath = corpusDir_ / filename;

        std::ofstream file(filePath);
        if (file.is_open()) {
            file << entries_[i];
            file.close();
        } else {
            return false;
        }
    }

    return true;
}

void CorpusManager::addEntry(const std::string& entry, const std::string& metadata) {
    entries_.push_back(entry);
    metadata_.push_back(metadata);
}

std::string CorpusManager::getRandomEntry() {
    if (entries_.empty()) {
        return "";
    }

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, entries_.size() - 1);

    return entries_[dist(gen)];
}

void CorpusManager::deduplicate() {
    std::unordered_map<std::string, size_t> seen;

    for (size_t i = 0; i < entries_.size(); ++i) {
        std::string hash = calculateEntryHash(entries_[i]);
        if (seen.find(hash) == seen.end()) {
            seen[hash] = i;
        }
    }

    // Mantener solo entradas únicas
    std::vector<std::string> uniqueEntries;
    std::vector<std::string> uniqueMetadata;

    for (const auto& [hash, index] : seen) {
        uniqueEntries.push_back(entries_[index]);
        uniqueMetadata.push_back(metadata_[index]);
    }

    entries_ = std::move(uniqueEntries);
    metadata_ = std::move(uniqueMetadata);
}

std::unordered_map<std::string, size_t> CorpusManager::getStatistics() const {
    return {
        {"total_entries", entries_.size()},
        {"total_size_bytes", calculateCacheSize()}
    };
}

std::string CorpusManager::calculateEntryHash(const std::string& entry) const {
    std::hash<std::string> hasher;
    return std::to_string(hasher(entry));
}

bool CorpusManager::validateCorpus() const {
    for (const auto& entry : entries_) {
        if (!FuzzUtils::isValidASCII(entry)) {
            return false;
        }
    }
    return true;
}

void CorpusManager::minimizeCorpus() {
    // Ordenar por tamaño (mantener entradas más pequeñas primero)
    std::vector<std::pair<size_t, size_t>> indices;
    for (size_t i = 0; i < entries_.size(); ++i) {
        indices.emplace_back(entries_[i].size(), i);
    }

    std::sort(indices.begin(), indices.end());

    // Mantener solo las primeras N entradas más pequeñas
    const size_t maxEntries = 1000;
    if (indices.size() > maxEntries) {
        indices.resize(maxEntries);
    }

    std::vector<std::string> minimizedEntries;
    std::vector<std::string> minimizedMetadata;

    for (const auto& [size, index] : indices) {
        minimizedEntries.push_back(entries_[index]);
        minimizedMetadata.push_back(metadata_[index]);
    }

    entries_ = std::move(minimizedEntries);
    metadata_ = std::move(minimizedMetadata);
}

size_t CorpusManager::calculateCacheSize() const {
    size_t totalSize = 0;
    for (const auto& entry : entries_) {
        totalSize += entry.size();
    }
    return totalSize;
}

// ============================================================================
// FuzzUtils - Implementación
// ============================================================================

bool FuzzUtils::isValidASCII(const std::string& str) {
    for (char c : str) {
        if (static_cast<unsigned char>(c) > 127) {
            return false;
        }
    }
    return true;
}

bool FuzzUtils::containsDangerousChars(const std::string& str) {
    for (char c : str) {
        if (c == '\0' || c == '\r' || c == '\n' || c == '\t') {
            return true;
        }
    }
    return false;
}

std::string FuzzUtils::sanitizeString(const std::string& str) {
    std::string sanitized;
    for (char c : str) {
        if (c == '\0') {
            sanitized += "\\0";
        } else if (c == '\r') {
            sanitized += "\\r";
        } else if (c == '\n') {
            sanitized += "\\n";
        } else if (c == '\t') {
            sanitized += "\\t";
        } else if (static_cast<unsigned char>(c) < 32 || c == 127) {
            sanitized += "\\x" + std::to_string(static_cast<unsigned char>(c));
        } else {
            sanitized += c;
        }
    }
    return sanitized;
}

size_t FuzzUtils::editDistance(const std::string& s1, const std::string& s2) {
    const size_t m = s1.size();
    const size_t n = s2.size();

    std::vector<std::vector<size_t>> dp(m + 1, std::vector<size_t>(n + 1));

    for (size_t i = 0; i <= m; ++i) {
        for (size_t j = 0; j <= n; ++j) {
            if (i == 0) {
                dp[i][j] = j;
            } else if (j == 0) {
                dp[i][j] = i;
            } else if (s1[i - 1] == s2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
            }
        }
    }

    return dp[m][n];
}

bool FuzzUtils::isUniqueCrash(const FuzzResult& crash,
                              const std::vector<FuzzResult>& knownCrashes) {
    std::string crashHash = calculateStackTraceHash(crash.stackTrace);

    for (const auto& known : knownCrashes) {
        std::string knownHash = calculateStackTraceHash(known.stackTrace);
        if (crashHash == knownHash) {
            return false;
        }
    }

    return true;
}

std::string FuzzUtils::generateCrashFileName(const FuzzResult& crash) {
    std::string timestamp = getFormattedTimestamp();
    std::string type = crash.errorType;
    std::replace(type.begin(), type.end(), ' ', '_');

    return "crash_" + type + "_" + timestamp + ".txt";
}

std::string FuzzUtils::formatFuzzResult(const FuzzResult& result) {
    std::stringstream ss;

    ss << "=== Fuzz Result ===\n";
    ss << "Target: " << static_cast<int>(result.target) << "\n";
    ss << "Crash: " << (result.isCrash ? "Yes" : "No") << "\n";
    ss << "Hang: " << (result.isHang ? "Yes" : "No") << "\n";
    ss << "Error Type: " << result.errorType << "\n";
    ss << "Error Message: " << result.errorMessage << "\n";
    ss << "Input Size: " << result.inputSize << "\n";
    ss << "Execution Time: " << result.executionTime.count() << "ms\n";
    ss << "Input (first 100 chars): " << result.input.substr(0, 100);
    if (result.input.size() > 100) {
        ss << "...";
    }
    ss << "\n";

    if (!result.stackTrace.empty()) {
        ss << "Stack Trace:\n" << result.stackTrace << "\n";
    }

    return ss.str();
}

std::string FuzzUtils::calculateStackTraceHash(const std::string& stackTrace) {
    std::hash<std::string> hasher;
    return std::to_string(hasher(stackTrace));
}

std::string FuzzUtils::detectCrashType(const std::string& output, int exitCode) {
    if (exitCode == 139 || exitCode == 11) return "Segmentation Fault";
    if (exitCode == 134) return "Assertion Failed";
    if (exitCode == 6) return "Abort";
    if (exitCode == 8) return "Floating Point Exception";

    std::string lower = output;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower.find("segmentation") != std::string::npos) return "Segmentation Fault";
    if (lower.find("assertion") != std::string::npos) return "Assertion Failed";
    if (lower.find("stack overflow") != std::string::npos) return "Stack Overflow";
    if (lower.find("heap corruption") != std::string::npos) return "Heap Corruption";
    if (lower.find("access violation") != std::string::npos) return "Access Violation";

    return "Unknown";
}

bool FuzzUtils::validateCompilerState() {
    // En una implementación real, aquí se verificaría el estado interno del compilador
    return true;
}

std::string FuzzUtils::getFormattedTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    return ss.str();
}

bool FuzzUtils::ensureDirectory(const std::filesystem::path& dir) {
    return std::filesystem::create_directories(dir);
}

} // namespace cpp20::compiler::testing
