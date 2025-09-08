/**
 * @file FuzzingEngine.h
 * @brief Motor de fuzzing para testing del compilador C++20
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <random>
#include <filesystem>
#include <chrono>

namespace cpp20::compiler::testing {

/**
 * @brief Tipos de componentes que se pueden fuzzear
 */
enum class FuzzTarget {
    Lexer,          // Fuzzing del lexer
    Parser,         // Fuzzing del parser
    Preprocessor,   // Fuzzing del preprocesador
    Semantic,       // Fuzzing del análisis semántico
    CodeGen,        // Fuzzing del generador de código
    FullPipeline    // Fuzzing de toda la pipeline
};

/**
 * @brief Estrategias de generación de fuzzing
 */
enum class FuzzStrategy {
    Random,         // Generación completamente aleatoria
    Mutational,     // Mutación de inputs válidos
    GrammarBased,   // Basado en gramática C++
    CoverageGuided  // Guiado por cobertura de código
};

/**
 * @brief Resultado de una sesión de fuzzing
 */
struct FuzzResult {
    std::string input;                    // Input que causó el problema
    std::string errorType;               // Tipo de error encontrado
    std::string errorMessage;             // Mensaje de error detallado
    std::string stackTrace;              // Stack trace si hubo crash
    FuzzTarget target;                   // Componente que falló
    std::chrono::milliseconds executionTime; // Tiempo de ejecución
    bool isCrash;                        // Si fue un crash
    bool isHang;                         // Si fue un hang
    size_t inputSize;                    // Tamaño del input

    FuzzResult(const std::string& inp = "", FuzzTarget tgt = FuzzTarget::Lexer)
        : input(inp), target(tgt), executionTime(0), isCrash(false),
          isHang(false), inputSize(inp.size()) {}
};

/**
 * @brief Estadísticas de una sesión de fuzzing
 */
struct FuzzStatistics {
    size_t totalInputs;                  // Total de inputs generados
    size_t crashesFound;                 // Número de crashes encontrados
    size_t hangsFound;                   // Número de hangs encontrados
    size_t uniqueCrashes;                // Crashes únicos
    size_t coverageIncrease;             // Aumento de cobertura
    std::chrono::milliseconds totalTime; // Tiempo total
    std::unordered_map<std::string, size_t> errorCounts; // Conteo por tipo de error

    FuzzStatistics()
        : totalInputs(0), crashesFound(0), hangsFound(0), uniqueCrashes(0),
          coverageIncrease(0), totalTime(0) {}
};

/**
 * @brief Generador de inputs para fuzzing
 */
class FuzzInputGenerator {
public:
    /**
     * @brief Constructor
     */
    FuzzInputGenerator(size_t seed = std::random_device{}());

    /**
     * @brief Destructor
     */
    ~FuzzInputGenerator();

    /**
     * @brief Genera input aleatorio
     */
    std::string generateRandomInput(size_t maxLength = 1024);

    /**
     * @brief Genera input basado en gramática C++
     */
    std::string generateGrammarBasedInput(FuzzTarget target, size_t complexity = 1);

    /**
     * @brief Muta un input existente
     */
    std::string mutateInput(const std::string& input, double mutationRate = 0.1);

    /**
     * @brief Genera input específico para un target
     */
    std::string generateTargetedInput(FuzzTarget target);

    /**
     * @brief Carga corpus de seeds
     */
    bool loadCorpus(const std::filesystem::path& corpusDir);

    /**
     * @brief Añade seed al corpus
     */
    void addSeed(const std::string& seed);

    /**
     * @brief Obtiene seed aleatorio del corpus
     */
    std::string getRandomSeed();

private:
    std::mt19937 rng_;
    std::vector<std::string> corpus_;
    std::uniform_int_distribution<size_t> lengthDist_;
    std::uniform_real_distribution<double> mutationDist_;

    /**
     * @brief Genera tokens C++ aleatorios
     */
    std::string generateRandomTokens(size_t count);

    /**
     * @brief Genera expresiones aleatorias
     */
    std::string generateRandomExpression(size_t depth = 1);

    /**
     * @brief Genera declaraciones aleatorias
     */
    std::string generateRandomDeclaration();

    /**
     * @brief Genera identificadores válidos
     */
    std::string generateRandomIdentifier();

    /**
     * @brief Genera literales aleatorios
     */
    std::string generateRandomLiteral();

    /**
     * @brief Aplica mutaciones al input
     */
    std::string applyMutations(const std::string& input, double mutationRate);

    /**
     * @brief Mutación: inserción de bytes
     */
    std::string insertRandomBytes(const std::string& input);

    /**
     * @brief Mutación: eliminación de bytes
     */
    std::string deleteRandomBytes(const std::string& input);

    /**
     * @brief Mutación: modificación de bytes
     */
    std::string modifyRandomBytes(const std::string& input);

    /**
     * @brief Mutación: duplicación de secciones
     */
    std::string duplicateRandomSection(const std::string& input);
};

/**
 * @brief Ejecutor de fuzzing
 */
class FuzzExecutor {
public:
    /**
     * @brief Constructor
     */
    FuzzExecutor();

    /**
     * @brief Destructor
     */
    ~FuzzExecutor();

    /**
     * @brief Ejecuta fuzzing en un target específico
     */
    FuzzResult executeFuzzInput(const std::string& input,
                               FuzzTarget target,
                               std::chrono::milliseconds timeout = std::chrono::seconds(5));

    /**
     * @brief Verifica si el compilador maneja correctamente el input
     */
    bool validateCompilerBehavior(const std::string& input, FuzzTarget target);

    /**
     * @brief Detecta crashes en el proceso
     */
    bool detectCrash(const std::string& output, int exitCode);

    /**
     * @brief Detecta hangs en el proceso
     */
    bool detectHang(std::chrono::milliseconds executionTime,
                   std::chrono::milliseconds timeout);

    /**
     * @brief Obtiene stack trace de un crash
     */
    std::string getStackTrace();

    /**
     * @brief Minimiza input que causa crash
     */
    std::string minimizeInput(const std::string& crashingInput, FuzzTarget target);

private:
    /**
     * @brief Ejecuta fuzzing del lexer
     */
    FuzzResult fuzzLexer(const std::string& input);

    /**
     * @brief Ejecuta fuzzing del parser
     */
    FuzzResult fuzzParser(const std::string& input);

    /**
     * @brief Ejecuta fuzzing del preprocesador
     */
    FuzzResult fuzzPreprocessor(const std::string& input);

    /**
     * @brief Ejecuta fuzzing del análisis semántico
     */
    FuzzResult fuzzSemantic(const std::string& input);

    /**
     * @brief Ejecuta fuzzing del generador de código
     */
    FuzzResult fuzzCodeGen(const std::string& input);

    /**
     * @brief Ejecuta fuzzing de toda la pipeline
     */
    FuzzResult fuzzFullPipeline(const std::string& input);

    /**
     * @brief Ejecuta comando con captura de salida
     */
    std::tuple<std::string, int, std::chrono::milliseconds> executeCommand(
        const std::string& command, std::chrono::milliseconds timeout);

    /**
     * @brief Verifica que la salida es razonable
     */
    bool validateOutput(const std::string& output, int exitCode);

    /**
     * @brief Detecta patrones de error comunes
     */
    std::string detectErrorPattern(const std::string& output, int exitCode);
};

/**
 * @brief Motor principal de fuzzing
 */
class FuzzingEngine {
public:
    /**
     * @brief Constructor
     */
    FuzzingEngine(FuzzTarget target = FuzzTarget::FullPipeline,
                  FuzzStrategy strategy = FuzzStrategy::Random);

    /**
     * @brief Destructor
     */
    ~FuzzingEngine();

    /**
     * @brief Ejecuta sesión de fuzzing
     */
    FuzzStatistics runFuzzing(size_t numIterations = 1000,
                             std::chrono::minutes duration = std::chrono::minutes(5));

    /**
     * @brief Configura el motor de fuzzing
     */
    void configure(size_t maxInputSize = 4096,
                  std::chrono::milliseconds timeout = std::chrono::seconds(5),
                  double mutationRate = 0.1);

    /**
     * @brief Carga corpus de seeds
     */
    bool loadCorpus(const std::filesystem::path& corpusDir);

    /**
     * @brief Guarda crashes encontrados
     */
    void saveCrashes(const std::filesystem::path& outputDir) const;

    /**
     * @brief Obtiene estadísticas actuales
     */
    const FuzzStatistics& getStatistics() const { return statistics_; }

    /**
     * @brief Obtiene crashes encontrados
     */
    const std::vector<FuzzResult>& getCrashes() const { return crashes_; }

    /**
     * @brief Habilita/deshabilita modo verbose
     */
    void setVerbose(bool verbose) { verbose_ = verbose; }

    /**
     * @brief Establece semilla para generación aleatoria
     */
    void setSeed(size_t seed);

private:
    FuzzTarget target_;
    FuzzStrategy strategy_;
    size_t maxInputSize_;
    std::chrono::milliseconds timeout_;
    double mutationRate_;
    bool verbose_;
    size_t seed_;

    FuzzStatistics statistics_;
    std::vector<FuzzResult> crashes_;
    std::unique_ptr<FuzzInputGenerator> inputGenerator_;
    std::unique_ptr<FuzzExecutor> executor_;

    /**
     * @brief Inicializa generadores
     */
    void initializeGenerators();

    /**
     * @brief Ejecuta una iteración de fuzzing
     */
    FuzzResult runIteration(size_t iteration);

    /**
     * @brief Procesa resultado de fuzzing
     */
    void processResult(const FuzzResult& result);

    /**
     * @brief Verifica si el resultado es un nuevo crash
     */
    bool isNewCrash(const FuzzResult& result);

    /**
     * @brief Actualiza estadísticas
     */
    void updateStatistics(const FuzzResult& result);

    /**
     * @brief Genera reporte de progreso
     */
    void reportProgress(size_t iteration, const FuzzResult& result);

    /**
     * @brief Verifica límites de tiempo
     */
    bool checkTimeLimits(std::chrono::steady_clock::time_point startTime,
                        std::chrono::minutes maxDuration);

    /**
     * @brief Minimiza crashes encontrados
     */
    void minimizeCrashes();

    /**
     * @brief Limpia recursos
     */
    void cleanup();
};

/**
 * @brief Gestor de corpus de fuzzing
 */
class CorpusManager {
public:
    /**
     * @brief Constructor
     */
    CorpusManager(const std::filesystem::path& corpusDir);

    /**
     * @brief Destructor
     */
    ~CorpusManager();

    /**
     * @brief Carga corpus desde directorio
     */
    bool loadCorpus();

    /**
     * @brief Guarda corpus a directorio
     */
    bool saveCorpus() const;

    /**
     * @brief Añade entrada al corpus
     */
    void addEntry(const std::string& entry, const std::string& metadata = "");

    /**
     * @brief Obtiene entrada aleatoria
     */
    std::string getRandomEntry();

    /**
     * @brief Obtiene todas las entradas
     */
    const std::vector<std::string>& getAllEntries() const { return entries_; }

    /**
     * @brief Elimina entradas duplicadas
     */
    void deduplicate();

    /**
     * @brief Obtiene estadísticas del corpus
     */
    std::unordered_map<std::string, size_t> getStatistics() const;

private:
    std::filesystem::path corpusDir_;
    std::vector<std::string> entries_;
    std::vector<std::string> metadata_;

    /**
     * @brief Calcula hash de entrada
     */
    std::string calculateEntryHash(const std::string& entry) const;

    /**
     * @brief Verifica integridad del corpus
     */
    bool validateCorpus() const;

    /**
     * @brief Minimiza corpus eliminando entradas redundantes
     */
    void minimizeCorpus();
};

/**
 * @brief Utilidades para fuzzing
 */
class FuzzUtils {
public:
    /**
     * @brief Verifica si un string es ASCII válido
     */
    static bool isValidASCII(const std::string& str);

    /**
     * @brief Verifica si un string contiene caracteres de control peligrosos
     */
    static bool containsDangerousChars(const std::string& str);

    /**
     * @brief Sanitiza string para uso seguro
     */
    static std::string sanitizeString(const std::string& str);

    /**
     * @brief Calcula distancia de edición entre dos strings
     */
    static size_t editDistance(const std::string& s1, const std::string& s2);

    /**
     * @brief Verifica si un crash es único comparado con crashes conocidos
     */
    static bool isUniqueCrash(const FuzzResult& crash,
                             const std::vector<FuzzResult>& knownCrashes);

    /**
     * @brief Genera nombre de archivo único para crash
     */
    static std::string generateCrashFileName(const FuzzResult& crash);

    /**
     * @brief Formatea resultado de fuzzing para reporte
     */
    static std::string formatFuzzResult(const FuzzResult& result);

    /**
     * @brief Calcula hash de stack trace
     */
    static std::string calculateStackTraceHash(const std::string& stackTrace);

    /**
     * @brief Detecta tipo de crash basado en salida
     */
    static std::string detectCrashType(const std::string& output, int exitCode);

    /**
     * @brief Verifica si el compilador está en un estado consistente
     */
    static bool validateCompilerState();

    /**
     * @brief Obtiene timestamp formateado
     */
    static std::string getFormattedTimestamp();

    /**
     * @brief Crea directorio si no existe
     */
    static bool ensureDirectory(const std::filesystem::path& dir);
};

} // namespace cpp20::compiler::testing
