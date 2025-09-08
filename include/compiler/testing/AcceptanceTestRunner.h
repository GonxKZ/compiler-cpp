/**
 * @file AcceptanceTestRunner.h
 * @brief Sistema de pruebas de aceptación para validar el compilador C++20
 */

#pragma once

#include <compiler/driver/CompilerDriver.h>
#include <compiler/backend/codegen/LinkerIntegration.h>
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <memory>

namespace cpp20::compiler::testing {

/**
 * @brief Resultado de una prueba individual
 */
struct TestResult {
    std::string testName;
    bool passed;
    std::string errorMessage;
    std::chrono::milliseconds executionTime;
    std::vector<std::string> compilerOutput;
    std::vector<std::string> programOutput;
    int exitCode;

    TestResult(const std::string& name)
        : testName(name), passed(false), executionTime(0), exitCode(0) {}
};

/**
 * @brief Categorías de pruebas de aceptación
 */
enum class TestCategory {
    BasicCompilation,    // Compilación básica
    ABITests,           // Pruebas de ABI
    ExceptionHandling,  // Manejo de excepciones
    NameMangling,       // Name mangling
    Templates,          // Plantillas y metaprogramming
    Modules,            // Módulos C++20
    Coroutines,         // Corroutines
    Constexpr,          // Evaluación constexpr
    Optimization,       // Optimizaciones
    Performance,        // Rendimiento
    Integration         // Pruebas de integración
};

/**
 * @brief Información de una prueba de aceptación
 */
struct AcceptanceTest {
    std::string name;
    std::string description;
    TestCategory category;
    std::vector<std::filesystem::path> sourceFiles;
    std::vector<std::string> compilerArgs;
    std::vector<std::string> linkerArgs;
    std::string expectedOutput;
    int expectedExitCode;
    bool shouldCompile;
    bool shouldLink;
    bool shouldRun;
    std::chrono::milliseconds timeout;

    AcceptanceTest(const std::string& testName, TestCategory cat)
        : name(testName), category(cat), expectedExitCode(0),
          shouldCompile(true), shouldLink(true), shouldRun(true),
          timeout(std::chrono::seconds(30)) {}
};

/**
 * @brief Ejecutor de pruebas de aceptación
 */
class AcceptanceTestRunner {
public:
    /**
     * @brief Constructor
     */
    AcceptanceTestRunner(const std::filesystem::path& testDirectory,
                        const std::filesystem::path& tempDirectory);

    /**
     * @brief Destructor
     */
    ~AcceptanceTestRunner();

    /**
     * @brief Ejecuta todas las pruebas
     */
    std::vector<TestResult> runAllTests();

    /**
     * @brief Ejecuta pruebas de una categoría específica
     */
    std::vector<TestResult> runTestsByCategory(TestCategory category);

    /**
     * @brief Ejecuta una prueba específica
     */
    TestResult runTest(const std::string& testName);

    /**
     * @brief Lista todas las pruebas disponibles
     */
    std::vector<std::string> listAvailableTests();

    /**
     * @brief Genera reporte de resultados
     */
    std::string generateReport(const std::vector<TestResult>& results);

    /**
     * @brief Configura el compilador a usar
     */
    void setCompilerDriver(std::unique_ptr<driver::CompilerDriver> driver);

    /**
     * @brief Configura la integración con linker
     */
    void setLinkerIntegration(std::unique_ptr<backend::LinkerIntegration> linker);

    /**
     * @brief Establece timeout global
     */
    void setGlobalTimeout(std::chrono::milliseconds timeout);

    /**
     * @brief Habilita/desabilita ejecución de programas
     */
    void setRunPrograms(bool run);

    /**
     * @brief Obtiene estadísticas de las pruebas
     */
    std::unordered_map<TestCategory, size_t> getTestCounts() const;

private:
    std::filesystem::path testDirectory_;
    std::filesystem::path tempDirectory_;
    std::unique_ptr<driver::CompilerDriver> compilerDriver_;
    std::unique_ptr<backend::LinkerIntegration> linker_;
    std::chrono::milliseconds globalTimeout_;
    bool runPrograms_;

    std::vector<std::unique_ptr<AcceptanceTest>> tests_;
    std::unordered_map<std::string, size_t> testIndex_;

    /**
     * @brief Inicializa las pruebas de aceptación
     */
    void initializeTests();

    /**
     * @brief Crea pruebas básicas de compilación
     */
    void createBasicCompilationTests();

    /**
     * @brief Crea pruebas de ABI
     */
    void createABITests();

    /**
     * @brief Crea pruebas de manejo de excepciones
     */
    void createExceptionHandlingTests();

    /**
     * @brief Crea pruebas de name mangling
     */
    void createNameManglingTests();

    /**
     * @brief Crea pruebas de plantillas
     */
    void createTemplateTests();

    /**
     * @brief Crea pruebas de módulos
     */
    void createModuleTests();

    /**
     * @brief Crea pruebas de corrutinas
     */
    void createCoroutineTests();

    /**
     * @brief Crea pruebas de constexpr
     */
    void createConstexprTests();

    /**
     * @brief Ejecuta una prueba individual
     */
    TestResult executeTest(const AcceptanceTest& test);

    /**
     * @brief Compila archivos fuente
     */
    std::vector<std::filesystem::path> compileSources(
        const AcceptanceTest& test,
        std::vector<std::string>& compilerOutput);

    /**
     * @brief Enlaza archivos objeto
     */
    std::filesystem::path linkObjects(
        const std::vector<std::filesystem::path>& objectFiles,
        const AcceptanceTest& test,
        std::vector<std::string>& linkerOutput);

    /**
     * @brief Ejecuta programa compilado
     */
    std::pair<std::string, int> executeProgram(
        const std::filesystem::path& executable,
        const AcceptanceTest& test);

    /**
     * @brief Valida resultado de la prueba
     */
    bool validateTestResult(const TestResult& result, const AcceptanceTest& test);

    /**
     * @brief Limpia archivos temporales
     */
    void cleanupTempFiles(const std::vector<std::filesystem::path>& files);

    /**
     * @brief Genera nombre de archivo temporal
     */
    std::filesystem::path generateTempFileName(const std::string& prefix,
                                             const std::string& extension);

    /**
     * @brief Parsea salida del compilador para encontrar errores
     */
    std::vector<std::string> parseCompilerErrors(const std::string& output);
};

/**
 * @brief Generador de pruebas de aceptación
 */
class TestGenerator {
public:
    /**
     * @brief Genera prueba básica de "Hello World"
     */
    static std::unique_ptr<AcceptanceTest> generateHelloWorldTest();

    /**
     * @brief Genera prueba de ABI con structs complejos
     */
    static std::unique_ptr<AcceptanceTest> generateABITest();

    /**
     * @brief Genera prueba de excepciones
     */
    static std::unique_ptr<AcceptanceTest> generateExceptionTest();

    /**
     * @brief Genera prueba de name mangling
     */
    static std::unique_ptr<AcceptanceTest> generateManglingTest();

    /**
     * @brief Genera prueba de plantillas
     */
    static std::unique_ptr<AcceptanceTest> generateTemplateTest();

    /**
     * @brief Genera prueba de módulos
     */
    static std::unique_ptr<AcceptanceTest> generateModuleTest();

    /**
     * @brief Genera prueba de corrutinas
     */
    static std::unique_ptr<AcceptanceTest> generateCoroutineTest();

    /**
     * @brief Genera prueba de constexpr
     */
    static std::unique_ptr<AcceptanceTest> generateConstexprTest();

    /**
     * @brief Crea archivo fuente temporal
     */
    static std::filesystem::path createTempSourceFile(
        const std::string& content,
        const std::string& filename);
};

/**
 * @brief Utilidades para pruebas
 */
class TestUtils {
public:
    /**
     * @brief Compara salida esperada con salida real
     */
    static bool compareOutput(const std::string& expected, const std::string& actual);

    /**
     * @brief Verifica que un archivo existe y no está vacío
     */
    static bool validateOutputFile(const std::filesystem::path& file);

    /**
     * @brief Verifica que un ejecutable se puede ejecutar
     */
    static bool validateExecutable(const std::filesystem::path& executable);

    /**
     * @brief Ejecuta programa con timeout
     */
    static std::pair<std::string, int> executeWithTimeout(
        const std::filesystem::path& executable,
        std::chrono::milliseconds timeout);

    /**
     * @brief Genera reporte HTML de resultados
     */
    static std::string generateHTMLReport(const std::vector<TestResult>& results);

    /**
     * @brief Genera reporte JUnit XML
     */
    static std::string generateJUnitReport(const std::vector<TestResult>& results);

    /**
     * @brief Calcula estadísticas de pruebas
     */
    static std::unordered_map<std::string, double> calculateStatistics(
        const std::vector<TestResult>& results);
};

} // namespace cpp20::compiler::testing
