/**
 * @file TimingProfiler.h
 * @brief Sistema de profiling y medición de tiempos de compilación
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <memory>
#include <iostream>
#include <mutex>

namespace cpp20::compiler {

/**
 * @brief Fases de compilación para profiling
 */
enum class CompilationPhase {
    // Fases del front-end
    Lexing,
    Parsing,
    Preprocessing,
    SemanticAnalysis,

    // Fases de templates y constexpr
    TemplateInstantiation,
    ConstexprEvaluation,
    ConceptChecking,

    // Fases del back-end
    IRGeneration,
    InstructionSelection,
    RegisterAllocation,
    CodeOptimization,
    PeepholeOptimization,

    // Fases de código objeto
    ObjectEmission,
    UnwindGeneration,
    ExceptionHandling,

    // Fases de linking
    SymbolResolution,
    RelocationProcessing,
    FinalLinking,

    // Fases de módulos
    ModuleInterfaceGeneration,
    ModuleImportProcessing,
    HeaderUnitProcessing,

    // Otras fases
    DiagnosticEmission,
    FileIO,
    MemoryManagement,
    TotalCompilation,

    // Placeholder para extensiones futuras
    CustomPhase
};

/**
 * @brief Información de tiempo para una fase
 */
struct PhaseTiming {
    CompilationPhase phase;
    std::string phaseName;
    std::chrono::microseconds duration;
    size_t memoryUsed;        // Bytes de memoria usados
    size_t peakMemory;        // Pico de memoria usada
    size_t operationsCount;   // Número de operaciones realizadas
    std::string details;      // Detalles adicionales

    PhaseTiming(CompilationPhase p, const std::string& name)
        : phase(p), phaseName(name), duration(0), memoryUsed(0),
          peakMemory(0), operationsCount(0) {}
};

/**
 * @brief Estadísticas de una fase
 */
struct PhaseStats {
    size_t callCount;                    // Número de veces que se ejecutó la fase
    std::chrono::microseconds totalTime; // Tiempo total acumulado
    std::chrono::microseconds minTime;   // Tiempo mínimo
    std::chrono::microseconds maxTime;   // Tiempo máximo
    std::chrono::microseconds avgTime;   // Tiempo promedio
    size_t totalMemory;                  // Memoria total usada
    size_t totalOperations;              // Operaciones totales

    PhaseStats() : callCount(0), totalTime(0), minTime(std::chrono::microseconds::max()),
                  maxTime(0), avgTime(0), totalMemory(0), totalOperations(0) {}

    void update(const PhaseTiming& timing) {
        callCount++;
        totalTime += timing.duration;
        minTime = std::min(minTime, timing.duration);
        maxTime = std::max(maxTime, timing.duration);
        totalMemory += timing.memoryUsed;
        totalOperations += timing.operationsCount;

        if (callCount > 0) {
            avgTime = totalTime / callCount;
        }
    }
};

/**
 * @brief Temporizador automático (RAII)
 */
class AutoTimer {
public:
    /**
     * @brief Constructor - inicia el temporizador
     */
    AutoTimer(TimingProfiler& profiler, CompilationPhase phase,
              const std::string& details = "");

    /**
     * @brief Destructor - detiene el temporizador y registra el tiempo
     */
    ~AutoTimer();

    /**
     * @brief Añade detalles adicionales
     */
    void addDetails(const std::string& details);

    /**
     * @brief Incrementa el contador de operaciones
     */
    void incrementOperations(size_t count = 1);

    /**
     * @brief Registra uso de memoria
     */
    void recordMemoryUsage(size_t bytes);

private:
    TimingProfiler& profiler_;
    CompilationPhase phase_;
    std::chrono::steady_clock::time_point startTime_;
    std::string details_;
    size_t operations_;
    size_t memoryUsed_;
};

/**
 * @brief Sistema de profiling de tiempos
 */
class TimingProfiler {
public:
    /**
     * @brief Constructor
     */
    TimingProfiler();

    /**
     * @brief Destructor
     */
    ~TimingProfiler();

    /**
     * @brief Inicia medición de una fase
     */
    void startPhase(CompilationPhase phase, const std::string& details = "");

    /**
     * @brief Finaliza medición de una fase
     */
    void endPhase(CompilationPhase phase);

    /**
     * @brief Registra tiempo de una fase completada
     */
    void recordPhaseTiming(CompilationPhase phase, std::chrono::microseconds duration,
                          size_t memoryUsed = 0, size_t operations = 0,
                          const std::string& details = "");

    /**
     * @brief Obtiene información de timing de una fase
     */
    const PhaseTiming* getLastPhaseTiming(CompilationPhase phase) const;

    /**
     * @brief Obtiene estadísticas de una fase
     */
    const PhaseStats* getPhaseStats(CompilationPhase phase) const;

    /**
     * @brief Obtiene todas las fases medidas
     */
    std::vector<CompilationPhase> getMeasuredPhases() const;

    /**
     * @brief Genera reporte de tiempos
     */
    std::string generateTimingReport(bool detailed = false) const;

    /**
     * @brief Genera reporte en formato JSON
     */
    std::string generateJSONReport() const;

    /**
     * @brief Reinicia todas las mediciones
     */
    void reset();

    /**
     * @brief Habilita/deshabilita profiling
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }

    /**
     * @brief Verifica si el profiling está habilitado
     */
    bool isEnabled() const { return enabled_; }

    /**
     * @brief Obtiene el tiempo total de compilación
     */
    std::chrono::microseconds getTotalCompilationTime() const;

    /**
     * @brief Obtiene la fase más lenta
     */
    CompilationPhase getSlowestPhase() const;

    /**
     * @brief Obtiene la fase que más memoria usa
     */
    CompilationPhase getMostMemoryIntensivePhase() const;

private:
    bool enabled_;
    mutable std::mutex mutex_;

    // Timing actual por fase
    std::unordered_map<CompilationPhase, std::chrono::steady_clock::time_point> activeTimers_;
    std::unordered_map<CompilationPhase, std::string> activeDetails_;
    std::unordered_map<CompilationPhase, size_t> activeOperations_;
    std::unordered_map<CompilationPhase, size_t> activeMemory_;

    // Historial de timings
    std::unordered_map<CompilationPhase, std::vector<PhaseTiming>> phaseHistory_;

    // Estadísticas acumuladas
    std::unordered_map<CompilationPhase, PhaseStats> phaseStats_;

    // Información de fases
    std::unordered_map<CompilationPhase, std::string> phaseNames_;

    /**
     * @brief Inicializa nombres de fases
     */
    void initializePhaseNames();

    /**
     * @brief Obtiene nombre de una fase
     */
    std::string getPhaseName(CompilationPhase phase) const;

    /**
     * @brief Formatea duración como string
     */
    std::string formatDuration(std::chrono::microseconds duration) const;

    /**
     * @brief Formatea tamaño de memoria como string
     */
    std::string formatMemorySize(size_t bytes) const;

    /**
     * @brief Calcula porcentaje del tiempo total
     */
    double calculatePercentage(std::chrono::microseconds duration) const;

    /**
     * @brief Ordena fases por tiempo
     */
    std::vector<std::pair<CompilationPhase, std::chrono::microseconds>>
        getPhasesSortedByTime() const;

    /**
     * @brief Ordena fases por uso de memoria
     */
    std::vector<std::pair<CompilationPhase, size_t>>
        getPhasesSortedByMemory() const;

    /**
     * @brief Genera sección de resumen del reporte
     */
    std::string generateSummarySection() const;

    /**
     * @brief Genera sección detallada del reporte
     */
    std::string generateDetailedSection() const;

    /**
     * @brief Genera sección de estadísticas del reporte
     */
    std::string generateStatsSection() const;
};

/**
 * @brief Utilidades para medición de memoria
 */
class MemoryProfiler {
public:
    /**
     * @brief Obtiene uso de memoria actual del proceso
     */
    static size_t getCurrentMemoryUsage();

    /**
     * @brief Obtiene pico de memoria del proceso
     */
    static size_t getPeakMemoryUsage();

    /**
     * @brief Registra punto de medición de memoria
     */
    static void recordMemoryCheckpoint(const std::string& label);

    /**
     * @brief Obtiene diferencia de memoria desde el último checkpoint
     */
    static size_t getMemoryDelta();

private:
    static size_t lastCheckpoint_;
};

/**
 * @brief Sistema de telemetría para compilaciones
 */
class CompilationTelemetry {
public:
    /**
     * @brief Constructor
     */
    CompilationTelemetry(TimingProfiler& profiler);

    /**
     * @brief Registra evento de compilación
     */
    void recordCompilationEvent(const std::string& eventType,
                               const std::string& details = "");

    /**
     * @brief Registra métrica personalizada
     */
    void recordMetric(const std::string& name, double value);

    /**
     * @brief Registra error de compilación
     */
    void recordCompilationError(const std::string& errorType,
                               const std::string& file,
                               size_t line);

    /**
     * @brief Genera reporte de telemetría
     */
    std::string generateTelemetryReport() const;

    /**
     * @brief Exporta datos a formato JSON
     */
    std::string exportToJSON() const;

private:
    TimingProfiler& profiler_;
    std::vector<std::pair<std::string, std::string>> events_;
    std::unordered_map<std::string, std::vector<double>> metrics_;
    std::vector<std::tuple<std::string, std::string, size_t>> errors_;

    mutable std::mutex mutex_;
};

} // namespace cpp20::compiler
