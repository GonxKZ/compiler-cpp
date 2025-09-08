/**
 * @file TestFramework.h
 * @brief Framework completo de testing para el compilador C++20
 */

#pragma once

#include <compiler/driver/CompilerDriver.h>
#include <compiler/backend/codegen/LinkerIntegration.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <chrono>
#include <functional>

namespace cpp20::compiler::testing {

/**
 * @brief Resultado de un test individual
 */
struct TestResult {
    std::string testName;
    std::string testSuite;
    bool passed;
    std::chrono::milliseconds duration;
    std::string errorMessage;
    std::vector<std::string> details;
    std::unordered_map<std::string, std::string> metadata;

    TestResult(const std::string& name = "", const std::string& suite = "")
        : testName(name), testSuite(suite), passed(false), duration(0) {}
};

/**
 * @brief Categorías de tests
 */
enum class TestCategory {
    UnitTest,           // Tests unitarios
    IntegrationTest,    // Tests de integración
    RegressionTest,     // Tests de regresión
    PerformanceTest,    // Tests de rendimiento
    GoldenTest,         // Tests golden (comparación con referencia)
    FuzzTest,           // Tests de fuzzing
    AcceptanceTest      // Tests de aceptación
};

/**
 * @brief Estado de ejecución de tests
 */
enum class TestStatus {
    NotRun,
    Running,
    Passed,
    Failed,
    Skipped,
    Timeout
};

/**
 * @brief Información de un test
 */
struct TestInfo {
    std::string name;
    std::string description;
    TestCategory category;
    std::function<TestResult()> testFunction;
    std::vector<std::string> dependencies;
    std::chrono::milliseconds timeout;
    bool enabled;

    TestInfo(const std::string& n, TestCategory cat, std::function<TestResult()> func)
        : name(n), category(cat), testFunction(func), timeout(std::chrono::seconds(30)), enabled(true) {}
};

/**
 * @brief Suite de tests
 */
class TestSuite {
public:
    /**
     * @brief Constructor
     */
    TestSuite(const std::string& name);

    /**
     * @brief Destructor
     */
    ~TestSuite();

    /**
     * @brief Añade un test a la suite
     */
    void addTest(std::unique_ptr<TestInfo> test);

    /**
     * @brief Ejecuta todos los tests de la suite
     */
    std::vector<TestResult> runAll();

    /**
     * @brief Ejecuta un test específico
     */
    TestResult runTest(const std::string& testName);

    /**
     * @brief Obtiene lista de tests disponibles
     */
    std::vector<std::string> getTestNames() const;

    /**
     * @brief Habilita/deshabilita un test
     */
    void setTestEnabled(const std::string& testName, bool enabled);

    /**
     * @brief Establece timeout para un test
     */
    void setTestTimeout(const std::string& testName, std::chrono::milliseconds timeout);

    /**
     * @brief Obtiene estadísticas de la suite
     */
    std::unordered_map<std::string, size_t> getSuiteStatistics() const;

private:
    std::string suiteName_;
    std::vector<std::unique_ptr<TestInfo>> tests_;
    std::unordered_map<std::string, size_t> testIndex_;

    /**
     * @brief Ejecuta un test con timeout
     */
    TestResult runTestWithTimeout(TestInfo& test);

    /**
     * @brief Actualiza índice de tests
     */
    void updateIndex();
};

/**
 * @brief Framework principal de testing
 */
class TestFramework {
public:
    /**
     * @brief Constructor
     */
    TestFramework();

    /**
     * @brief Destructor
     */
    ~TestFramework();

    /**
     * @brief Registra una suite de tests
     */
    void registerSuite(std::unique_ptr<TestSuite> suite);

    /**
     * @brief Ejecuta todas las suites de tests
     */
    std::vector<TestResult> runAllSuites();

    /**
     * @brief Ejecuta una suite específica
     */
    std::vector<TestResult> runSuite(const std::string& suiteName);

    /**
     * @brief Ejecuta tests por categoría
     */
    std::vector<TestResult> runTestsByCategory(TestCategory category);

    /**
     * @brief Ejecuta un test específico
     */
    TestResult runSpecificTest(const std::string& suiteName, const std::string& testName);

    /**
     * @brief Lista suites disponibles
     */
    std::vector<std::string> getAvailableSuites() const;

    /**
     * @brief Lista tests disponibles en una suite
     */
    std::vector<std::string> getTestsInSuite(const std::string& suiteName) const;

    /**
     * @brief Genera reporte de resultados
     */
    std::string generateReport(const std::vector<TestResult>& results,
                              const std::string& format = "text");

    /**
     * @brief Establece directorio de salida para reportes
     */
    void setOutputDirectory(const std::filesystem::path& outputDir);

    /**
     * @brief Obtiene directorio de salida
     */
    const std::filesystem::path& getOutputDirectory() const { return outputDirectory_; }

    /**
     * @brief Configura compilador para tests
     */
    void setCompilerDriver(std::unique_ptr<driver::CompilerDriver> driver);

    /**
     * @brief Configura linker para tests
     */
    void setLinkerIntegration(std::unique_ptr<backend::LinkerIntegration> linker);

private:
    std::vector<std::unique_ptr<TestSuite>> suites_;
    std::unordered_map<std::string, size_t> suiteIndex_;
    std::filesystem::path outputDirectory_;
    std::unique_ptr<driver::CompilerDriver> compilerDriver_;
    std::unique_ptr<backend::LinkerIntegration> linker_;

    /**
     * @brief Actualiza índice de suites
     */
    void updateSuiteIndex();
};

/**
 * @brief Ejecutor de golden tests
 */
class GoldenTestRunner {
public:
    /**
     * @brief Constructor
     */
    GoldenTestRunner(const std::filesystem::path& testDataDir,
                    const std::filesystem::path& referenceCompilerPath);

    /**
     * @brief Ejecuta golden test
     */
    TestResult runGoldenTest(const std::filesystem::path& testFile);

    /**
     * @brief Actualiza golden files
     */
    bool updateGoldenFiles(const std::vector<std::filesystem::path>& testFiles);

    /**
     * @brief Lista tests disponibles
     */
    std::vector<std::filesystem::path> getAvailableTests() const;

    /**
     * @brief Compara salida con referencia
     */
    bool compareWithReference(const std::string& output,
                             const std::filesystem::path& referenceFile);

private:
    std::filesystem::path testDataDir_;
    std::filesystem::path referenceCompilerPath_;

    /**
     * @brief Compila con compilador de referencia
     */
    std::string compileWithReference(const std::filesystem::path& testFile);

    /**
     * @brief Compila con nuestro compilador
     */
    std::string compileWithOurCompiler(const std::filesystem::path& testFile);

    /**
     * @brief Normaliza salida para comparación
     */
    std::string normalizeOutput(const std::string& output);
};

/**
 * @brief Generador de tests unitarios
 */
class UnitTestGenerator {
public:
    /**
     * @brief Genera tests para el lexer
     */
    static std::vector<std::unique_ptr<TestInfo>> generateLexerTests();

    /**
     * @brief Genera tests para el parser
     */
    static std::vector<std::unique_ptr<TestInfo>> generateParserTests();

    /**
     * @brief Genera tests para el analizador semántico
     */
    static std::vector<std::unique_ptr<TestInfo>> generateSemanticTests();

    /**
     * @brief Genera tests para el generador de código
     */
    static std::vector<std::unique_ptr<TestInfo>> generateCodeGenTests();

    /**
     * @brief Genera tests para optimizaciones
     */
    static std::vector<std::unique_ptr<TestInfo>> generateOptimizationTests();

    /**
     * @brief Genera tests para el linker
     */
    static std::vector<std::unique_ptr<TestInfo>> generateLinkerTests();

    /**
     * @brief Genera tests para módulos
     */
    static std::vector<std::unique_ptr<TestInfo>> generateModuleTests();
};

/**
 * @brief Utilidades para testing
 */
class TestUtils {
public:
    /**
     * @brief Crea archivo temporal
     */
    static std::filesystem::path createTempFile(const std::string& content,
                                               const std::string& extension = ".cpp");

    /**
     * @brief Limpia archivos temporales
     */
    static void cleanupTempFiles(const std::vector<std::filesystem::path>& files);

    /**
     * @brief Ejecuta comando con timeout
     */
    static std::pair<std::string, int> executeCommand(const std::string& command,
                                                     std::chrono::milliseconds timeout);

    /**
     * @brief Compara archivos
     */
    static bool compareFiles(const std::filesystem::path& file1,
                           const std::filesystem::path& file2);

    /**
     * @brief Calcula hash de archivo
     */
    static std::string calculateFileHash(const std::filesystem::path& file);

    /**
     * @brief Verifica que un ejecutable existe y es válido
     */
    static bool validateExecutable(const std::filesystem::path& executable);

    /**
     * @brief Genera reporte HTML
     */
    static std::string generateHTMLReport(const std::vector<TestResult>& results);

    /**
     * @brief Genera reporte JUnit XML
     */
    static std::string generateJUnitReport(const std::vector<TestResult>& results);

    /**
     * @brief Calcula estadísticas de tests
     */
    static std::unordered_map<std::string, double> calculateTestStatistics(
        const std::vector<TestResult>& results);

    /**
     * @brief Formatea duración
     */
    static std::string formatDuration(std::chrono::milliseconds duration);

    /**
     * @brief Escapa caracteres especiales en XML
     */
    static std::string escapeXML(const std::string& text);
};

/**
 * @brief Test runner principal
 */
class TestRunner {
public:
    /**
     * @brief Constructor
     */
    TestRunner(int argc, char** argv);

    /**
     * @brief Ejecuta tests según argumentos de línea de comandos
     */
    int run();

    /**
     * @brief Muestra ayuda
     */
    void showHelp();

private:
    int argc_;
    char** argv_;
    std::unique_ptr<TestFramework> framework_;

    /**
     * @brief Parsea argumentos de línea de comandos
     */
    void parseArguments();

    /**
     * @brief Ejecuta tests unitarios
     */
    void runUnitTests();

    /**
     * @brief Ejecuta tests de integración
     */
    void runIntegrationTests();

    /**
     * @brief Ejecuta golden tests
     */
    void runGoldenTests();

    /**
     * @brief Ejecuta tests de rendimiento
     */
    void runPerformanceTests();
};

} // namespace cpp20::compiler::testing
