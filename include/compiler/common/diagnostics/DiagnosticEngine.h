#pragma once

#include "Diagnostic.h"
#include "SourceManager.h"
#include <memory>
#include <vector>
#include <functional>
#include <mutex>

namespace cpp20::compiler::diagnostics {

/**
 * @brief Motor principal del sistema de diagnósticos
 *
 * El DiagnosticEngine es responsable de:
 * - Recibir y procesar diagnósticos de todas las fases del compilador
 * - Formatear y emitir diagnósticos al usuario
 * - Mantener estadísticas de diagnósticos
 * - Gestionar el flujo de diagnóstico (errores fatales detienen compilación)
 * - Proporcionar interfaces para diagnostic consumers
 */
class DiagnosticEngine {
public:
    /**
     * @brief Consumer de diagnósticos
     *
     * Interface para componentes que quieren procesar diagnósticos
     * (por ejemplo, IDEs, herramientas de análisis, etc.)
     */
    class Consumer {
    public:
        virtual ~Consumer() = default;

        /**
         * @brief Procesa un diagnóstico
         * @param diagnostic El diagnóstico a procesar
         * @return true si el procesamiento fue exitoso
         */
        virtual bool handleDiagnostic(const Diagnostic& diagnostic) = 0;

        /**
         * @brief Finaliza el procesamiento de diagnósticos
         */
        virtual void finish() {}
    };

    /**
     * @brief Opciones de configuración del motor de diagnósticos
     */
    struct Options {
        bool showWarnings = true;           // Mostrar advertencias
        bool showNotes = true;              // Mostrar notas
        bool showColors = true;             // Usar colores en salida
        bool showSourceLines = true;        // Mostrar líneas de código fuente
        bool showFixIts = true;             // Mostrar sugerencias de corrección
        int maxErrors = 100;                // Máximo número de errores antes de detener
        bool fatalErrors = false;           // Tratar errores como fatales
        std::string outputFile;             // Archivo de salida (vacío = stderr)
    };

    // Constructor y destructor
    explicit DiagnosticEngine(std::shared_ptr<SourceManager> sourceManager);
    ~DiagnosticEngine();

    // Configuración
    void setOptions(const Options& options) { options_ = options; }
    const Options& options() const { return options_; }

    // Gestión de consumers
    void addConsumer(std::unique_ptr<Consumer> consumer);
    void clearConsumers();

    // Emisión de diagnósticos
    void emit(const Diagnostic& diagnostic);
    void emit(DiagnosticLevel level, DiagnosticCode code,
              SourceLocation location, std::string message);

    // Métodos convenientes para tipos comunes de diagnóstico
    void reportError(DiagnosticCode code, SourceLocation loc, std::string msg);
    void reportWarning(DiagnosticCode code, SourceLocation loc, std::string msg);
    void reportNote(DiagnosticCode code, SourceLocation loc, std::string msg);
    void reportFatal(DiagnosticCode code, SourceLocation loc, std::string msg);

    // Estadísticas
    size_t errorCount() const { return errorCount_; }
    size_t warningCount() const { return warningCount_; }
    size_t noteCount() const { return noteCount_; }
    size_t totalCount() const { return errorCount_ + warningCount_ + noteCount_; }

    bool hasErrors() const { return errorCount_ > 0; }
    bool hasFatalErrors() const { return fatalCount_ > 0; }

    // Control de flujo
    void setErrorLimit(size_t limit) { options_.maxErrors = static_cast<int>(limit); }
    bool shouldContinue() const { return errorCount_ < static_cast<size_t>(options_.maxErrors); }

    // Historial de diagnósticos
    const std::vector<Diagnostic>& diagnostics() const { return diagnostics_; }
    void clearDiagnostics();

    // Utilidades de formateo
    std::string formatDiagnostic(const Diagnostic& diagnostic) const;
    std::string formatSourceLine(SourceLocation location, int contextLines = 1) const;

    // Funciones públicas para formateo
    std::string formatDiagnosticPublic(const Diagnostic& diagnostic) const {
        return formatDiagnostic(diagnostic);
    }

private:
    // Miembros privados
    std::shared_ptr<SourceManager> sourceManager_;
    Options options_;
    std::vector<std::unique_ptr<Consumer>> consumers_;
    std::vector<Diagnostic> diagnostics_;
    std::mutex mutex_;  // Para thread-safety

    // Contadores
    size_t errorCount_ = 0;
    size_t warningCount_ = 0;
    size_t noteCount_ = 0;
    size_t fatalCount_ = 0;

    // Métodos internos
    void processDiagnostic(const Diagnostic& diagnostic);
    void emitToConsumers(const Diagnostic& diagnostic);
    void updateStatistics(const Diagnostic& diagnostic);
    bool shouldEmit(const Diagnostic& diagnostic) const;
};

/**
 * @brief Consumer que emite diagnósticos a un stream
 */
class StreamConsumer : public DiagnosticEngine::Consumer {
public:
    explicit StreamConsumer(std::ostream& stream, bool useColors = true);

    bool handleDiagnostic(const Diagnostic& diagnostic) override;
    void finish() override;

private:
    std::ostream& stream_;
    bool useColors_;
    std::string formatWithColor(const std::string& text, const std::string& color) const;
};

/**
 * @brief Consumer que acumula diagnósticos en memoria
 */
class MemoryConsumer : public DiagnosticEngine::Consumer {
public:
    MemoryConsumer() = default;

    bool handleDiagnostic(const Diagnostic& diagnostic) override;

    const std::vector<Diagnostic>& diagnostics() const { return diagnostics_; }
    void clear() { diagnostics_.clear(); }

private:
    std::vector<Diagnostic> diagnostics_;
};

} // namespace cpp20::compiler::diagnostics
