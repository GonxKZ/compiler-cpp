/**
 * @file DiagnosticRenderer.h
 * @brief Sistema avanzado de renderizado de diagnósticos con formato caret
 */

#pragma once

#include <compiler/diagnostics/DiagnosticEngine.h>
#include <string>
#include <vector>
#include <memory>
#include <iostream>

namespace cpp20::compiler::diagnostics {

/**
 * @brief Estilos de formato para diagnósticos
 */
enum class DiagnosticStyle {
    Clang,      // Estilo Clang/GCC con caret
    MSVC,       // Estilo Microsoft
    JSON,       // Formato JSON estructurado
    SARIF       // Formato SARIF para herramientas
};

/**
 * @brief Colores para salida en terminal
 */
enum class Color {
    Reset = 0,
    Red = 31,
    Green = 32,
    Yellow = 33,
    Blue = 34,
    Magenta = 35,
    Cyan = 36,
    White = 37,
    BrightRed = 91,
    BrightGreen = 92,
    BrightYellow = 93,
    BrightBlue = 94,
    BrightMagenta = 95,
    BrightCyan = 96,
    BrightWhite = 97
};

/**
 * @brief Información de una línea de código fuente
 */
struct SourceLine {
    std::string content;      // Contenido de la línea
    size_t lineNumber;        // Número de línea (1-based)
    size_t startColumn;       // Columna de inicio en el archivo
    std::string fileName;     // Nombre del archivo

    SourceLine(const std::string& content, size_t lineNum, size_t startCol,
              const std::string& fileName)
        : content(content), lineNumber(lineNum), startColumn(startCol),
          fileName(fileName) {}
};

/**
 * @brief Renderizador de diagnósticos
 */
class DiagnosticRenderer {
public:
    /**
     * @brief Constructor
     */
    DiagnosticRenderer(std::ostream& output = std::cerr,
                      DiagnosticStyle style = DiagnosticStyle::Clang,
                      bool useColors = true);

    /**
     * @brief Destructor
     */
    ~DiagnosticRenderer();

    /**
     * @brief Renderiza un diagnóstico
     */
    void render(const Diagnostic& diagnostic);

    /**
     * @brief Renderiza múltiples diagnósticos
     */
    void render(const std::vector<Diagnostic>& diagnostics);

    /**
     * @brief Establece el estilo de formato
     */
    void setStyle(DiagnosticStyle style) { style_ = style; }

    /**
     * @brief Habilita/deshabilita colores
     */
    void setUseColors(bool useColors) { useColors_ = useColors; }

    /**
     * @brief Establece el ancho máximo de línea
     */
    void setMaxLineWidth(size_t width) { maxLineWidth_ = width; }

    /**
     * @brief Habilita/deshabilita la impresión de números de línea
     */
    void setShowLineNumbers(bool show) { showLineNumbers_ = show; }

    /**
     * @brief Habilita/deshabilita el contexto de líneas adicionales
     */
    void setShowContext(bool show) { showContext_ = show; }

    /**
     * @brief Obtiene el conteo de diagnósticos renderizados por severidad
     */
    std::unordered_map<DiagnosticLevel, size_t> getDiagnosticCounts() const {
        return diagnosticCounts_;
    }

private:
    std::ostream& output_;
    DiagnosticStyle style_;
    bool useColors_;
    size_t maxLineWidth_;
    bool showLineNumbers_;
    bool showContext_;
    std::unordered_map<DiagnosticLevel, size_t> diagnosticCounts_;

    /**
     * @brief Renderiza en estilo Clang
     */
    void renderClangStyle(const Diagnostic& diagnostic);

    /**
     * @brief Renderiza en estilo MSVC
     */
    void renderMSVCStyle(const Diagnostic& diagnostic);

    /**
     * @brief Renderiza en formato JSON
     */
    void renderJSONStyle(const Diagnostic& diagnostic);

    /**
     * @brief Renderiza en formato SARIF
     */
    void renderSARIFStyle(const Diagnostic& diagnostic);

    /**
     * @brief Obtiene líneas de contexto alrededor de una ubicación
     */
    std::vector<SourceLine> getContextLines(const SourceLocation& location,
                                          size_t contextLines = 2);

    /**
     * @brief Renderiza la ubicación principal del diagnóstico
     */
    void renderLocation(const SourceLocation& location, DiagnosticLevel level);

    /**
     * @brief Renderiza el mensaje principal
     */
    void renderMessage(const std::string& message, DiagnosticLevel level);

    /**
     * @brief Renderiza notas adicionales
     */
    void renderNotes(const std::vector<DiagnosticNote>& notes);

    /**
     * @brief Renderiza sugerencias de corrección
     */
    void renderSuggestions(const std::vector<std::string>& suggestions);

    /**
     * @brief Renderiza líneas de código con caret
     */
    void renderCodeWithCaret(const SourceLine& line,
                           size_t startCol, size_t endCol,
                           DiagnosticLevel level);

    /**
     * @brief Renderiza líneas de contexto
     */
    void renderContextLines(const std::vector<SourceLine>& contextLines,
                          size_t highlightLine);

    /**
     * @brief Obtiene el color para un nivel de diagnóstico
     */
    Color getColorForLevel(DiagnosticLevel level) const;

    /**
     * @brief Obtiene el prefijo de texto para un nivel
     */
    std::string getTextPrefix(DiagnosticLevel level) const;

    /**
     * @brief Obtiene el prefijo de símbolo para un nivel
     */
    std::string getSymbolPrefix(DiagnosticLevel level) const;

    /**
     * @brief Aplica color a un texto
     */
    std::string colorize(const std::string& text, Color color) const;

    /**
     * @brief Escapa caracteres especiales en JSON
     */
    std::string escapeJSON(const std::string& text) const;

    /**
     * @brief Formatea una ubicación como string
     */
    std::string formatLocation(const SourceLocation& location) const;

    /**
     * @brief Calcula la indentación para alineación
     */
    std::string getIndentation(size_t width) const;

    /**
     * @brief Trunca una línea si es demasiado larga
     */
    std::string truncateLine(const std::string& line, size_t maxWidth) const;

    /**
     * @brief Incrementa el conteo de diagnósticos
     */
    void incrementCount(DiagnosticLevel level);
};

/**
 * @brief Formateador de diagnósticos en cadena
 */
class DiagnosticFormatter {
public:
    /**
     * @brief Formatea un diagnóstico como string
     */
    static std::string format(const Diagnostic& diagnostic,
                            DiagnosticStyle style = DiagnosticStyle::Clang,
                            bool useColors = false);

    /**
     * @brief Formatea múltiples diagnósticos
     */
    static std::string format(const std::vector<Diagnostic>& diagnostics,
                            DiagnosticStyle style = DiagnosticStyle::Clang,
                            bool useColors = false);

    /**
     * @brief Genera un resumen de diagnósticos
     */
    static std::string generateSummary(const std::vector<Diagnostic>& diagnostics);
};

/**
 * @brief Utilidades para trabajar con diagnósticos
 */
class DiagnosticUtils {
public:
    /**
     * @brief Filtra diagnósticos por nivel
     */
    static std::vector<Diagnostic> filterByLevel(
        const std::vector<Diagnostic>& diagnostics,
        DiagnosticLevel level);

    /**
     * @brief Filtra diagnósticos por archivo
     */
    static std::vector<Diagnostic> filterByFile(
        const std::vector<Diagnostic>& diagnostics,
        const std::string& fileName);

    /**
     * @brief Ordena diagnósticos por ubicación
     */
    static void sortByLocation(std::vector<Diagnostic>& diagnostics);

    /**
     * @brief Cuenta diagnósticos por nivel
     */
    static std::unordered_map<DiagnosticLevel, size_t> countByLevel(
        const std::vector<Diagnostic>& diagnostics);

    /**
     * @brief Verifica si hay errores fatales
     */
    static bool hasFatalErrors(const std::vector<Diagnostic>& diagnostics);

    /**
     * @brief Obtiene el diagnóstico más severo
     */
    static DiagnosticLevel getMostSevereLevel(const std::vector<Diagnostic>& diagnostics);
};

} // namespace cpp20::compiler::diagnostics
