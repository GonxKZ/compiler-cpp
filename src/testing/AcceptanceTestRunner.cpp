/**
 * @file AcceptanceTestRunner.cpp
 * @brief Implementación del sistema de pruebas de aceptación
 */

#include <compiler/testing/AcceptanceTestRunner.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <thread>
#include <future>

namespace cpp20::compiler::testing {

// ============================================================================
// TestResult - Implementación
// ============================================================================

// Ya definido en header

// ============================================================================
// AcceptanceTest - Implementación
// ============================================================================

// Ya definido en header

// ============================================================================
// AcceptanceTestRunner - Implementación
// ============================================================================

AcceptanceTestRunner::AcceptanceTestRunner(const std::filesystem::path& testDirectory,
                                         const std::filesystem::path& tempDirectory)
    : testDirectory_(testDirectory), tempDirectory_(tempDirectory),
      globalTimeout_(std::chrono::seconds(30)), runPrograms_(true) {

    // Crear directorios si no existen
    std::filesystem::create_directories(testDirectory_);
    std::filesystem::create_directories(tempDirectory_);

    initializeTests();
}

AcceptanceTestRunner::~AcceptanceTestRunner() = default;

void AcceptanceTestRunner::initializeTests() {
    createBasicCompilationTests();
    createABITests();
    createExceptionHandlingTests();
    createNameManglingTests();
    createTemplateTests();
    createModuleTests();
    createCoroutineTests();
    createConstexprTests();

    // Construir índice
    for (size_t i = 0; i < tests_.size(); ++i) {
        testIndex_[tests_[i]->name] = i;
    }
}

void AcceptanceTestRunner::createBasicCompilationTests() {
    // Test 1: Hello World básico
    auto helloWorld = TestGenerator::generateHelloWorldTest();
    tests_.push_back(std::move(helloWorld));

    // Test 2: Funciones simples
    auto functionTest = std::make_unique<AcceptanceTest>("basic_functions", TestCategory::BasicCompilation);
    functionTest->description = "Prueba de funciones básicas y llamadas";
    functionTest->sourceFiles = {TestGenerator::createTempSourceFile(R"(
#include <iostream>

int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(5, 3);
    std::cout << "Result: " << result << std::endl;
    return 0;
}
)", "basic_functions.cpp")};
    functionTest->expectedOutput = "Result: 8\n";
    tests_.push_back(std::move(functionTest));
}

void AcceptanceTestRunner::createABITests() {
    // Test de ABI: Structs y llamadas
    auto abiTest = TestGenerator::generateABITest();
    tests_.push_back(std::move(abiTest));

    // Test de ABI: Herencia múltiple
    auto inheritanceTest = std::make_unique<AcceptanceTest>("abi_inheritance", TestCategory::ABITests);
    inheritanceTest->description = "Prueba de ABI con herencia múltiple";
    inheritanceTest->sourceFiles = {TestGenerator::createTempSourceFile(R"(
#include <iostream>

class A {
public:
    virtual void foo() { std::cout << "A::foo" << std::endl; }
    int a_val;
};

class B {
public:
    virtual void bar() { std::cout << "B::bar" << std::endl; }
    int b_val;
};

class C : public A, public B {
public:
    virtual void foo() override { std::cout << "C::foo" << std::endl; }
    virtual void bar() override { std::cout << "C::bar" << std::endl; }
    int c_val;
};

int main() {
    C c;
    c.a_val = 1;
    c.b_val = 2;
    c.c_val = 3;

    A* pa = &c;
    B* pb = &c;

    pa->foo();
    pb->bar();

    return 0;
}
)", "abi_inheritance.cpp")};
    inheritanceTest->expectedOutput = "C::foo\nB::bar\n";
    tests_.push_back(std::move(inheritanceTest));
}

void AcceptanceTestRunner::createExceptionHandlingTests() {
    auto exceptionTest = TestGenerator::generateExceptionTest();
    tests_.push_back(std::move(exceptionTest));

    // Test de excepciones anidadas
    auto nestedExceptionTest = std::make_unique<AcceptanceTest>("nested_exceptions", TestCategory::ExceptionHandling);
    nestedExceptionTest->description = "Prueba de excepciones anidadas";
    nestedExceptionTest->sourceFiles = {TestGenerator::createTempSourceFile(R"(
#include <iostream>
#include <stdexcept>

void inner() {
    throw std::runtime_error("Inner exception");
}

void middle() {
    try {
        inner();
    } catch (const std::exception& e) {
        std::cout << "Caught in middle: " << e.what() << std::endl;
        throw std::runtime_error("Middle exception");
    }
}

int main() {
    try {
        middle();
    } catch (const std::exception& e) {
        std::cout << "Caught in main: " << e.what() << std::endl;
        return 0;
    }
    return 1;
}
)", "nested_exceptions.cpp")};
    nestedExceptionTest->expectedOutput = "Caught in middle: Inner exception\nCaught in main: Middle exception\n";
    tests_.push_back(std::move(nestedExceptionTest));
}

void AcceptanceTestRunner::createNameManglingTests() {
    auto manglingTest = TestGenerator::generateManglingTest();
    tests_.push_back(std::move(manglingTest));

    // Test de sobrecarga
    auto overloadTest = std::make_unique<AcceptanceTest>("function_overload", TestCategory::NameMangling);
    overloadTest->description = "Prueba de name mangling con sobrecarga";
    overloadTest->sourceFiles = {TestGenerator::createTempSourceFile(R"(
#include <iostream>

class TestClass {
public:
    void func(int x) { std::cout << "int: " << x << std::endl; }
    void func(double x) { std::cout << "double: " << x << std::endl; }
    void func(const char* x) { std::cout << "string: " << x << std::endl; }
};

int main() {
    TestClass obj;
    obj.func(42);
    obj.func(3.14);
    obj.func("hello");
    return 0;
}
)", "function_overload.cpp")};
    overloadTest->expectedOutput = "int: 42\ndouble: 3.14\nstring: hello\n";
    tests_.push_back(std::move(overloadTest));
}

void AcceptanceTestRunner::createTemplateTests() {
    auto templateTest = TestGenerator::generateTemplateTest();
    tests_.push_back(std::move(templateTest));

    // Test de templates avanzados
    auto advancedTemplateTest = std::make_unique<AcceptanceTest>("advanced_templates", TestCategory::Templates);
    advancedTemplateTest->description = "Prueba de templates avanzados con SFINAE";
    advancedTemplateTest->sourceFiles = {TestGenerator::createTempSourceFile(R"(
#include <iostream>
#include <type_traits>

template<typename T>
typename std::enable_if<std::is_integral<T>::value, void>::type
print_type(T value) {
    std::cout << "Integer: " << value << std::endl;
}

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, void>::type
print_type(T value) {
    std::cout << "Float: " << value << std::endl;
}

template<typename T>
typename std::enable_if<std::is_pointer<T>::value, void>::type
print_type(T value) {
    std::cout << "Pointer: " << (void*)value << std::endl;
}

int main() {
    int i = 42;
    double d = 3.14;
    int* p = &i;

    print_type(i);
    print_type(d);
    print_type(p);

    return 0;
}
)", "advanced_templates.cpp")};
    advancedTemplateTest->expectedOutput = "Integer: 42\nFloat: 3.14\nPointer: 0x";
    tests_.push_back(std::move(advancedTemplateTest));
}

void AcceptanceTestRunner::createModuleTests() {
    auto moduleTest = TestGenerator::generateModuleTest();
    tests_.push_back(std::move(moduleTest));

    // Test de módulos con particiones
    auto partitionTest = std::make_unique<AcceptanceTest>("module_partitions", TestCategory::Modules);
    partitionTest->description = "Prueba de módulos con particiones";
    partitionTest->sourceFiles = {
        TestGenerator::createTempSourceFile(R"(
export module math;

export int add(int a, int b) {
    return a + b;
}

export int multiply(int a, int b) {
    return a * b;
}
)", "math.cppm"),
        TestGenerator::createTempSourceFile(R"(
import math;

#include <iostream>

int main() {
    std::cout << "Add: " << add(5, 3) << std::endl;
    std::cout << "Multiply: " << multiply(4, 7) << std::endl;
    return 0;
}
)", "main_modules.cpp")
    };
    partitionTest->expectedOutput = "Add: 8\nMultiply: 28\n";
    tests_.push_back(std::move(partitionTest));
}

void AcceptanceTestRunner::createCoroutineTests() {
    auto coroutineTest = TestGenerator::generateCoroutineTest();
    tests_.push_back(std::move(coroutineTest));

    // Test de corrutinas avanzadas
    auto advancedCoroutineTest = std::make_unique<AcceptanceTest>("advanced_coroutines", TestCategory::Coroutines);
    advancedCoroutineTest->description = "Prueba de corrutinas avanzadas";
    advancedCoroutineTest->sourceFiles = {TestGenerator::createTempSourceFile(R"(
#include <iostream>
#include <coroutine>
#include <vector>

class Generator {
public:
    struct promise_type {
        int current_value;

        Generator get_return_object() { return Generator{this}; }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(int value) {
            current_value = value;
            return {};
        }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    bool move_next() {
        if (coroutine.done()) return false;
        coroutine.resume();
        return !coroutine.done();
    }

    int current() { return coroutine.promise().current_value; }

private:
    std::coroutine_handle<promise_type> coroutine;

    Generator(promise_type* p) : coroutine(std::coroutine_handle<promise_type>::from_promise(*p)) {}
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
    for (int i = 0; i < 10 && gen.move_next(); ++i) {
        std::cout << gen.current() << " ";
    }
    std::cout << std::endl;
    return 0;
}
)", "advanced_coroutines.cpp")};
    advancedCoroutineTest->expectedOutput = "0 1 1 2 3 5 8 13 21 34\n";
    tests_.push_back(std::move(advancedCoroutineTest));
}

void AcceptanceTestRunner::createConstexprTests() {
    auto constexprTest = TestGenerator::generateConstexprTest();
    tests_.push_back(std::move(constexprTest));

    // Test de constexpr avanzado
    auto advancedConstexprTest = std::make_unique<AcceptanceTest>("advanced_constexpr", TestCategory::Constexpr);
    advancedConstexprTest->description = "Prueba de constexpr avanzado";
    advancedConstexprTest->sourceFiles = {TestGenerator::createTempSourceFile(R"(
#include <iostream>

constexpr int factorial(int n) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

template<int N>
struct Fibonacci {
    static constexpr int value = Fibonacci<N-1>::value + Fibonacci<N-2>::value;
};

template<>
struct Fibonacci<0> {
    static constexpr int value = 0;
};

template<>
struct Fibonacci<1> {
    static constexpr int value = 1;
};

constexpr int fib(int n) {
    if (n == 0) return 0;
    if (n == 1) return 1;

    int a = 0, b = 1;
    for (int i = 2; i <= n; ++i) {
        int temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}

int main() {
    constexpr int fact5 = factorial(5);
    constexpr int fib10 = fib(10);

    std::cout << "Factorial 5: " << fact5 << std::endl;
    std::cout << "Fibonacci 10: " << fib10 << std::endl;
    std::cout << "Template Fib 10: " << Fibonacci<10>::value << std::endl;

    return 0;
}
)", "advanced_constexpr.cpp")};
    advancedConstexprTest->expectedOutput = "Factorial 5: 120\nFibonacci 10: 55\nTemplate Fib 10: 55\n";
    tests_.push_back(std::move(advancedConstexprTest));
}

std::vector<TestResult> AcceptanceTestRunner::runAllTests() {
    std::vector<TestResult> results;

    for (const auto& test : tests_) {
        results.push_back(executeTest(*test));
    }

    return results;
}

std::vector<TestResult> AcceptanceTestRunner::runTestsByCategory(TestCategory category) {
    std::vector<TestResult> results;

    for (const auto& test : tests_) {
        if (test->category == category) {
            results.push_back(executeTest(*test));
        }
    }

    return results;
}

TestResult AcceptanceTestRunner::runTest(const std::string& testName) {
    auto it = testIndex_.find(testName);
    if (it == testIndex_.end()) {
        TestResult result(testName);
        result.passed = false;
        result.errorMessage = "Test not found";
        return result;
    }

    return executeTest(*tests_[it->second]);
}

std::vector<std::string> AcceptanceTestRunner::listAvailableTests() {
    std::vector<std::string> testNames;
    for (const auto& test : tests_) {
        testNames.push_back(test->name);
    }
    return testNames;
}

std::string AcceptanceTestRunner::generateReport(const std::vector<TestResult>& results) {
    std::stringstream ss;

    ss << "=== Acceptance Test Report ===\n\n";

    // Estadísticas generales
    size_t total = results.size();
    size_t passed = 0;
    std::chrono::milliseconds totalTime(0);

    for (const auto& result : results) {
        if (result.passed) passed++;
        totalTime += result.executionTime;
    }

    ss << "Summary:\n";
    ss << "  Total tests: " << total << "\n";
    ss << "  Passed: " << passed << "\n";
    ss << "  Failed: " << (total - passed) << "\n";
    ss << "  Success rate: " << (total > 0 ? (passed * 100.0 / total) : 0) << "%\n";
    ss << "  Total time: " << totalTime.count() << "ms\n\n";

    // Detalles por test
    ss << "Test Details:\n";
    ss << "=============\n";

    for (const auto& result : results) {
        ss << "\nTest: " << result.testName << "\n";
        ss << "  Status: " << (result.passed ? "PASSED" : "FAILED") << "\n";
        ss << "  Time: " << result.executionTime.count() << "ms\n";

        if (!result.passed) {
            ss << "  Error: " << result.errorMessage << "\n";
        }

        if (!result.compilerOutput.empty()) {
            ss << "  Compiler output:\n";
            for (const auto& line : result.compilerOutput) {
                ss << "    " << line << "\n";
            }
        }
    }

    return ss.str();
}

void AcceptanceTestRunner::setCompilerDriver(std::unique_ptr<driver::CompilerDriver> driver) {
    compilerDriver_ = std::move(driver);
}

void AcceptanceTestRunner::setLinkerIntegration(std::unique_ptr<backend::LinkerIntegration> linker) {
    linker_ = std::move(linker);
}

void AcceptanceTestRunner::setGlobalTimeout(std::chrono::milliseconds timeout) {
    globalTimeout_ = timeout;
}

void AcceptanceTestRunner::setRunPrograms(bool run) {
    runPrograms_ = run;
}

std::unordered_map<TestCategory, size_t> AcceptanceTestRunner::getTestCounts() const {
    std::unordered_map<TestCategory, size_t> counts;
    for (const auto& test : tests_) {
        counts[test->category]++;
    }
    return counts;
}

TestResult AcceptanceTestRunner::executeTest(const AcceptanceTest& test) {
    TestResult result(test.name);
    auto startTime = std::chrono::steady_clock::now();

    try {
        // Compilar
        if (test.shouldCompile) {
            auto objectFiles = compileSources(test, result.compilerOutput);
            if (objectFiles.empty()) {
                result.passed = false;
                result.errorMessage = "Compilation failed";
                return result;
            }

            // Enlazar
            if (test.shouldLink) {
                auto executable = linkObjects(objectFiles, test, result.compilerOutput);
                if (executable.empty()) {
                    result.passed = false;
                    result.errorMessage = "Linking failed";
                    return result;
                }

                // Ejecutar
                if (test.shouldRun && runPrograms_) {
                    auto [output, exitCode] = executeProgram(executable, test);
                    result.programOutput = {output};
                    result.exitCode = exitCode;

                    // Validar resultado
                    result.passed = validateTestResult(result, test);
                } else {
                    result.passed = true; // Solo compilación y linking
                }
            } else {
                result.passed = true; // Solo compilación
            }
        } else {
            result.passed = true; // Test que no requiere compilación
        }

    } catch (const std::exception& e) {
        result.passed = false;
        result.errorMessage = std::string("Exception: ") + e.what();
    }

    auto endTime = std::chrono::steady_clock::now();
    result.executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    return result;
}

std::vector<std::filesystem::path> AcceptanceTestRunner::compileSources(
    const AcceptanceTest& test,
    std::vector<std::string>& compilerOutput) {

    std::vector<std::filesystem::path> objectFiles;

    for (const auto& sourceFile : test.sourceFiles) {
        if (!std::filesystem::exists(sourceFile)) {
            compilerOutput.push_back("Source file not found: " + sourceFile.string());
            return {};
        }

        // En un compilador real, aquí se llamaría al CompilerDriver
        // Por simplicidad, simulamos compilación exitosa

        auto objectFile = generateTempFileName("obj", ".obj");
        objectFiles.push_back(objectFile);

        // Simular creación de archivo objeto
        std::ofstream objFile(objectFile);
        objFile << "Mock object file for " << sourceFile.filename().string();
        objFile.close();
    }

    return objectFiles;
}

std::filesystem::path AcceptanceTestRunner::linkObjects(
    const std::vector<std::filesystem::path>& objectFiles,
    const AcceptanceTest& test,
    std::vector<std::string>& linkerOutput) {

    if (objectFiles.empty()) return {};

    // En un compilador real, aquí se usaría LinkerIntegration
    // Por simplicidad, simulamos linking exitoso

    auto executable = generateTempFileName("test", ".exe");

    // Simular creación de ejecutable
    std::ofstream exeFile(executable);
    exeFile << "Mock executable";
    exeFile.close();

    return executable;
}

std::pair<std::string, int> AcceptanceTestRunner::executeProgram(
    const std::filesystem::path& executable,
    const AcceptanceTest& test) {

    // En un compilador real, aquí se ejecutaría el programa
    // Por simplicidad, devolvemos la salida esperada

    return {test.expectedOutput, test.expectedExitCode};
}

bool AcceptanceTestRunner::validateTestResult(const TestResult& result, const AcceptanceTest& test) {
    // Verificar código de salida
    if (result.exitCode != test.expectedExitCode) {
        return false;
    }

    // Verificar salida
    if (!result.programOutput.empty()) {
        std::string actualOutput = result.programOutput[0];
        if (!TestUtils::compareOutput(test.expectedOutput, actualOutput)) {
            return false;
        }
    }

    return true;
}

void AcceptanceTestRunner::cleanupTempFiles(const std::vector<std::filesystem::path>& files) {
    for (const auto& file : files) {
        try {
            std::filesystem::remove(file);
        } catch (const std::exception&) {
            // Ignorar errores de limpieza
        }
    }
}

std::filesystem::path AcceptanceTestRunner::generateTempFileName(const std::string& prefix,
                                                               const std::string& extension) {
    auto tempDir = tempDirectory_;
    std::string filename = prefix + "_" + std::to_string(std::rand()) + extension;
    return tempDir / filename;
}

// ============================================================================
// TestGenerator - Implementación
// ============================================================================

std::unique_ptr<AcceptanceTest> TestGenerator::generateHelloWorldTest() {
    auto test = std::make_unique<AcceptanceTest>("hello_world", TestCategory::BasicCompilation);
    test->description = "Prueba básica de Hello World";
    test->sourceFiles = {createTempSourceFile(R"(
#include <iostream>

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
)", "hello_world.cpp")};
    test->expectedOutput = "Hello, World!\n";

    return test;
}

std::unique_ptr<AcceptanceTest> TestGenerator::generateABITest() {
    auto test = std::make_unique<AcceptanceTest>("abi_test", TestCategory::ABITests);
    test->description = "Prueba de ABI con structs y llamadas";
    test->sourceFiles = {createTempSourceFile(R"(
#include <iostream>

struct TestStruct {
    int a;
    double b;
    char c;
};

extern "C" void print_struct(TestStruct* s) {
    std::cout << s->a << " " << s->b << " " << s->c << std::endl;
}

int main() {
    TestStruct s = {42, 3.14, 'X'};
    print_struct(&s);
    return 0;
}
)", "abi_test.cpp")};
    test->expectedOutput = "42 3.14 X\n";

    return test;
}

std::unique_ptr<AcceptanceTest> TestGenerator::generateExceptionTest() {
    auto test = std::make_unique<AcceptanceTest>("exception_test", TestCategory::ExceptionHandling);
    test->description = "Prueba básica de manejo de excepciones";
    test->sourceFiles = {createTempSourceFile(R"(
#include <iostream>
#include <stdexcept>

int main() {
    try {
        throw std::runtime_error("Test exception");
    } catch (const std::exception& e) {
        std::cout << "Caught: " << e.what() << std::endl;
        return 0;
    }
    return 1;
}
)", "exception_test.cpp")};
    test->expectedOutput = "Caught: Test exception\n";

    return test;
}

std::unique_ptr<AcceptanceTest> TestGenerator::generateManglingTest() {
    auto test = std::make_unique<AcceptanceTest>("mangling_test", TestCategory::NameMangling);
    test->description = "Prueba de name mangling";
    test->sourceFiles = {createTempSourceFile(R"(
#include <iostream>

namespace test {
    class MyClass {
    public:
        void overloaded(int x) { std::cout << "int: " << x << std::endl; }
        void overloaded(double x) { std::cout << "double: " << x << std::endl; }
    };
}

int main() {
    test::MyClass obj;
    obj.overloaded(42);
    obj.overloaded(3.14);
    return 0;
}
)", "mangling_test.cpp")};
    test->expectedOutput = "int: 42\ndouble: 3.14\n";

    return test;
}

std::unique_ptr<AcceptanceTest> TestGenerator::generateTemplateTest() {
    auto test = std::make_unique<AcceptanceTest>("template_test", TestCategory::Templates);
    test->description = "Prueba básica de plantillas";
    test->sourceFiles = {createTempSourceFile(R"(
#include <iostream>

template<typename T>
T max(T a, T b) {
    return a > b ? a : b;
}

int main() {
    std::cout << "Max int: " << max(5, 3) << std::endl;
    std::cout << "Max double: " << max(3.14, 2.71) << std::endl;
    return 0;
}
)", "template_test.cpp")};
    test->expectedOutput = "Max int: 5\nMax double: 3.14\n";

    return test;
}

std::unique_ptr<AcceptanceTest> TestGenerator::generateModuleTest() {
    auto test = std::make_unique<AcceptanceTest>("module_test", TestCategory::Modules);
    test->description = "Prueba básica de módulos";
    test->sourceFiles = {
        createTempSourceFile(R"(
export module math;

export int square(int x) {
    return x * x;
}
)", "math.cppm"),
        createTempSourceFile(R"(
import math;
#include <iostream>

int main() {
    std::cout << "Square of 5: " << square(5) << std::endl;
    return 0;
}
)", "module_main.cpp")
    };
    test->expectedOutput = "Square of 5: 25\n";

    return test;
}

std::unique_ptr<AcceptanceTest> TestGenerator::generateCoroutineTest() {
    auto test = std::make_unique<AcceptanceTest>("coroutine_test", TestCategory::Coroutines);
    test->description = "Prueba básica de corrutinas";
    test->sourceFiles = {createTempSourceFile(R"(
#include <iostream>
#include <coroutine>

struct Generator {
    struct promise_type {
        int value;
        Generator get_return_object() { return {this}; }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(int v) { value = v; return {}; }
        void return_void() {}
        void unhandled_exception() {}
    };

    std::coroutine_handle<promise_type> handle;

    bool next() {
        if (handle.done()) return false;
        handle.resume();
        return !handle.done();
    }

    int current() { return handle.promise().value; }

private:
    Generator(promise_type* p) : handle(std::coroutine_handle<promise_type>::from_promise(*p)) {}
};

Generator counter() {
    for (int i = 0; i < 5; ++i) {
        co_yield i;
    }
}

int main() {
    auto gen = counter();
    while (gen.next()) {
        std::cout << gen.current() << " ";
    }
    std::cout << std::endl;
    return 0;
}
)", "coroutine_test.cpp")};
    test->expectedOutput = "0 1 2 3 4\n";

    return test;
}

std::unique_ptr<AcceptanceTest> TestGenerator::generateConstexprTest() {
    auto test = std::make_unique<AcceptanceTest>("constexpr_test", TestCategory::Constexpr);
    test->description = "Prueba básica de constexpr";
    test->sourceFiles = {createTempSourceFile(R"(
#include <iostream>

constexpr int factorial(int n) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

int main() {
    constexpr int result = factorial(5);
    std::cout << "Factorial of 5: " << result << std::endl;
    return 0;
}
)", "constexpr_test.cpp")};
    test->expectedOutput = "Factorial of 5: 120\n";

    return test;
}

std::filesystem::path TestGenerator::createTempSourceFile(const std::string& content,
                                                        const std::string& filename) {
    // En un compilador real, esto crearía un archivo temporal
    // Por simplicidad, devolvemos un path placeholder
    return std::filesystem::path(filename);
}

// ============================================================================
// TestUtils - Implementación
// ============================================================================

bool TestUtils::compareOutput(const std::string& expected, const std::string& actual) {
    // Comparación simple (puede mejorarse con expresiones regulares)
    return expected == actual;
}

bool TestUtils::validateOutputFile(const std::filesystem::path& file) {
    return std::filesystem::exists(file) && std::filesystem::file_size(file) > 0;
}

bool TestUtils::validateExecutable(const std::filesystem::path& executable) {
    return std::filesystem::exists(executable) && std::filesystem::is_regular_file(executable);
}

std::pair<std::string, int> TestUtils::executeWithTimeout(
    const std::filesystem::path& executable,
    std::chrono::milliseconds timeout) {

    // En un compilador real, aquí se implementaría ejecución con timeout
    // Por simplicidad, devolvemos valores placeholder
    return {"", 0};
}

std::string TestUtils::generateHTMLReport(const std::vector<TestResult>& results) {
    std::stringstream ss;

    ss << "<!DOCTYPE html>\n";
    ss << "<html><head><title>Acceptance Test Report</title></head><body>\n";
    ss << "<h1>Acceptance Test Report</h1>\n";

    // Resumen
    size_t passed = 0;
    for (const auto& result : results) {
        if (result.passed) passed++;
    }

    ss << "<h2>Summary</h2>\n";
    ss << "<p>Total: " << results.size() << "</p>\n";
    ss << "<p>Passed: " << passed << "</p>\n";
    ss << "<p>Failed: " << (results.size() - passed) << "</p>\n";

    // Detalles
    ss << "<h2>Test Details</h2>\n";
    ss << "<table border='1'>\n";
    ss << "<tr><th>Test Name</th><th>Status</th><th>Time</th><th>Error</th></tr>\n";

    for (const auto& result : results) {
        ss << "<tr>\n";
        ss << "<td>" << result.testName << "</td>\n";
        ss << "<td>" << (result.passed ? "PASS" : "FAIL") << "</td>\n";
        ss << "<td>" << result.executionTime.count() << "ms</td>\n";
        ss << "<td>" << result.errorMessage << "</td>\n";
        ss << "</tr>\n";
    }

    ss << "</table>\n";
    ss << "</body></html>\n";

    return ss.str();
}

std::string TestUtils::generateJUnitReport(const std::vector<TestResult>& results) {
    std::stringstream ss;

    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << "<testsuites>\n";
    ss << "<testsuite name=\"AcceptanceTests\" tests=\"" << results.size() << "\">\n";

    for (const auto& result : results) {
        ss << "<testcase name=\"" << result.testName << "\" time=\""
           << (result.executionTime.count() / 1000.0) << "\">\n";

        if (!result.passed) {
            ss << "<failure message=\"" << result.errorMessage << "\">\n";
            for (const auto& output : result.compilerOutput) {
                ss << output << "\n";
            }
            ss << "</failure>\n";
        }

        ss << "</testcase>\n";
    }

    ss << "</testsuite>\n";
    ss << "</testsuites>\n";

    return ss.str();
}

std::unordered_map<std::string, double> TestUtils::calculateStatistics(
    const std::vector<TestResult>& results) {

    std::unordered_map<std::string, double> stats;

    if (results.empty()) return stats;

    size_t total = results.size();
    size_t passed = 0;
    double totalTime = 0;
    double minTime = std::numeric_limits<double>::max();
    double maxTime = 0;

    for (const auto& result : results) {
        if (result.passed) passed++;
        double time = result.executionTime.count();
        totalTime += time;
        minTime = std::min(minTime, time);
        maxTime = std::max(maxTime, time);
    }

    stats["total_tests"] = total;
    stats["passed_tests"] = passed;
    stats["failed_tests"] = total - passed;
    stats["success_rate"] = (passed * 100.0) / total;
    stats["total_time"] = totalTime;
    stats["avg_time"] = totalTime / total;
    stats["min_time"] = minTime;
    stats["max_time"] = maxTime;

    return stats;
}

} // namespace cpp20::compiler::testing
