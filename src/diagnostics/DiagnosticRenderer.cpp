/**
 * @file DiagnosticRenderer.cpp
 * @brief Implementación del sistema de renderizado de diagnósticos
 */

#include <compiler/diagnostics/DiagnosticRenderer.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <unordered_map>

namespace cpp20::compiler::diagnostics {

// ============================================================================
// DiagnosticRenderer - Implementación
// ============================================================================

DiagnosticRenderer::DiagnosticRenderer(std::ostream& output, DiagnosticStyle style, bool useColors)
    : output_(output), style_(style), useColors_(useColors),
      maxLineWidth_(120), showLineNumbers_(true), showContext_(true) {
}

DiagnosticRenderer::~DiagnosticRenderer() = default;

void DiagnosticRenderer::render(const Diagnostic& diagnostic) {
    incrementCount(diagnostic.level);

    switch (style_) {
        case DiagnosticStyle::Clang:
            renderClangStyle(diagnostic);
            break;
        case DiagnosticStyle::MSVC:
            renderMSVCStyle(diagnostic);
            break;
        case DiagnosticStyle::JSON:
            renderJSONStyle(diagnostic);
            break;
        case DiagnosticStyle::SARIF:
            renderSARIFStyle(diagnostic);
            break;
    }
}

void DiagnosticRenderer::render(const std::vector<Diagnostic>& diagnostics) {
    for (const auto& diagnostic : diagnostics) {
        render(diagnostic);
        output_ << std::endl;
    }
}

void DiagnosticRenderer::renderClangStyle(const Diagnostic& diagnostic) {
    // Renderizar ubicación principal
    renderLocation(diagnostic.location, diagnostic.level);

    // Renderizar mensaje principal
    renderMessage(diagnostic.message, diagnostic.level);

    // Renderizar código fuente con caret si hay ubicación
    if (diagnostic.location.isValid()) {
        auto contextLines = getContextLines(diagnostic.location);
        if (!contextLines.empty()) {
            // Encontrar la línea principal
            for (const auto& line : contextLines) {
                if (line.lineNumber == diagnostic.location.line) {
                    renderCodeWithCaret(line,
                                      diagnostic.location.column,
                                      diagnostic.location.endColumn,
                                      diagnostic.level);
                    break;
                }
            }

            // Renderizar líneas de contexto si está habilitado
            if (showContext_ && contextLines.size() > 1) {
                renderContextLines(contextLines, diagnostic.location.line);
            }
        }
    }

    // Renderizar notas adicionales
    renderNotes(diagnostic.notes);

    // Renderizar sugerencias
    renderSuggestions(diagnostic.suggestions);
}

void DiagnosticRenderer::renderMSVCStyle(const Diagnostic& diagnostic) {
    // Formato MSVC: archivo(línea): nivel C####: mensaje
    output_ << diagnostic.location.fileName
            << "(" << diagnostic.location.line << "): "
            << getTextPrefix(diagnostic.level) << " "
            << diagnostic.message << std::endl;

    // Código fuente con caret
    if (diagnostic.location.isValid()) {
        auto contextLines = getContextLines(diagnostic.location);
        if (!contextLines.empty()) {
            for (const auto& line : contextLines) {
                if (line.lineNumber == diagnostic.location.line) {
                    output_ << line.content << std::endl;
                    if (diagnostic.location.endColumn > diagnostic.location.column) {
                        // Subrayar el rango de error
                        std::string underline(diagnostic.location.endColumn - diagnostic.location.column, '~');
                        output_ << std::string(diagnostic.location.column - 1, ' ') << underline << std::endl;
                    } else {
                        // Caret simple
                        output_ << std::string(diagnostic.location.column - 1, ' ') << "^" << std::endl;
                    }
                    break;
                }
            }
        }
    }

    // Notas adicionales
    renderNotes(diagnostic.notes);
}

void DiagnosticRenderer::renderJSONStyle(const Diagnostic& diagnostic) {
    output_ << "{"
            << "\"level\": \"" << getTextPrefix(diagnostic.level) << "\","
            << "\"message\": \"" << escapeJSON(diagnostic.message) << "\","
            << "\"file\": \"" << escapeJSON(diagnostic.location.fileName) << "\","
            << "\"line\": " << diagnostic.location.line << ","
            << "\"column\": " << diagnostic.location.column;

    if (!diagnostic.notes.empty()) {
        output_ << ",\"notes\": [";
        for (size_t i = 0; i < diagnostic.notes.size(); ++i) {
            const auto& note = diagnostic.notes[i];
            if (i > 0) output_ << ",";
            output_ << "{"
                    << "\"message\": \"" << escapeJSON(note.message) << "\","
                    << "\"file\": \"" << escapeJSON(note.location.fileName) << "\","
                    << "\"line\": " << note.location.line << ","
                    << "\"column\": " << note.location.column
                    << "}";
        }
        output_ << "]";
    }

    output_ << "}" << std::endl;
}

void DiagnosticRenderer::renderSARIFStyle(const Diagnostic& diagnostic) {
    // SARIF es un formato complejo - simplificado para este ejemplo
    output_ << "{"
            << "\"$schema\": \"https://raw.githubusercontent.com/oasis-tcs/sarif-spec/master/Schemata/sarif-schema-2.1.0.json\","
            << "\"version\": \"2.1.0\","
            << "\"runs\": [{"
            << "\"tool\": {\"driver\": {\"name\": \"cpp20-compiler\"}},"
            << "\"results\": [{"
            << "\"level\": \"" << (diagnostic.level == DiagnosticLevel::Error ? "error" : "warning") << "\","
            << "\"message\": {\"text\": \"" << escapeJSON(diagnostic.message) << "\"},"
            << "\"locations\": [{"
            << "\"physicalLocation\": {"
            << "\"artifactLocation\": {\"uri\": \"" << escapeJSON(diagnostic.location.fileName) << "\"},"
            << "\"region\": {"
            << "\"startLine\": " << diagnostic.location.line << ","
            << "\"startColumn\": " << diagnostic.location.column
            << "}}}]";

    if (!diagnostic.notes.empty()) {
        output_ << ",\"relatedLocations\": [";
        for (size_t i = 0; i < diagnostic.notes.size(); ++i) {
            const auto& note = diagnostic.notes[i];
            if (i > 0) output_ << ",";
            output_ << "{"
                    << "\"message\": {\"text\": \"" << escapeJSON(note.message) << "\"},"
                    << "\"physicalLocation\": {"
                    << "\"artifactLocation\": {\"uri\": \"" << escapeJSON(note.location.fileName) << "\"},"
                    << "\"region\": {"
                    << "\"startLine\": " << note.location.line << ","
                    << "\"startColumn\": " << note.location.column
                    << "}}}]";
        }
        output_ << "]";
    }

    output_ << "}]}]}" << std::endl;
}

std::vector<SourceLine> DiagnosticRenderer::getContextLines(const SourceLocation& location, size_t contextLines) {
    std::vector<SourceLine> lines;

    // En un compilador real, aquí se leerían las líneas del archivo fuente
    // usando el SourceManager

    // Placeholder: crear líneas de ejemplo
    if (location.isValid()) {
        // Línea principal
        std::string content = "int main() { return 0; }"; // Ejemplo
        lines.emplace_back(content, location.line, 1, location.fileName);

        // Líneas de contexto adicionales
        if (contextLines > 0 && location.line > 1) {
            for (size_t i = 1; i <= contextLines && location.line > i; ++i) {
                std::string ctxContent = "// context line " + std::to_string(location.line - i);
                lines.emplace_back(ctxContent, location.line - i, 1, location.fileName);
            }
        }
    }

    return lines;
}

void DiagnosticRenderer::renderLocation(const SourceLocation& location, DiagnosticLevel level) {
    if (location.isValid()) {
        output_ << colorize(location.fileName, Color::Cyan) << ":"
                << colorize(std::to_string(location.line), Color::Yellow) << ":"
                << colorize(std::to_string(location.column), Color::Yellow) << ": ";
    }
}

void DiagnosticRenderer::renderMessage(const std::string& message, DiagnosticLevel level) {
    output_ << colorize(getSymbolPrefix(level), getColorForLevel(level)) << " "
            << message << std::endl;
}

void DiagnosticRenderer::renderNotes(const std::vector<DiagnosticNote>& notes) {
    for (const auto& note : notes) {
        if (note.location.isValid()) {
            output_ << "  " << colorize(note.location.fileName, Color::Cyan) << ":"
                    << colorize(std::to_string(note.location.line), Color::Yellow) << ":"
                    << colorize(std::to_string(note.location.column), Color::Yellow) << ": ";
        } else {
            output_ << "  ";
        }
        output_ << colorize("note: ", Color::Blue) << note.message << std::endl;
    }
}

void DiagnosticRenderer::renderSuggestions(const std::vector<std::string>& suggestions) {
    for (const auto& suggestion : suggestions) {
        output_ << colorize("  suggestion: ", Color::Green) << suggestion << std::endl;
    }
}

void DiagnosticRenderer::renderCodeWithCaret(const SourceLine& line, size_t startCol, size_t endCol, DiagnosticLevel level) {
    // Mostrar la línea de código
    if (showLineNumbers_) {
        output_ << colorize(std::to_string(line.lineNumber), Color::Cyan) << " | ";
    }
    output_ << line.content << std::endl;

    // Mostrar caret o subrayado
    if (showLineNumbers_) {
        output_ << "  | ";
    }

    if (endCol > startCol) {
        // Subrayar rango
        size_t underlineLength = endCol - startCol;
        std::string underline(underlineLength, '~');
        output_ << std::string(startCol - 1, ' ')
                << colorize(underline, getColorForLevel(level)) << std::endl;
    } else {
        // Caret simple
        output_ << std::string(startCol - 1, ' ')
                << colorize("^", getColorForLevel(level)) << std::endl;
    }
}

void DiagnosticRenderer::renderContextLines(const std::vector<SourceLine>& contextLines, size_t highlightLine) {
    for (const auto& line : contextLines) {
        if (line.lineNumber == highlightLine) continue; // Ya se mostró

        if (showLineNumbers_) {
            output_ << colorize(std::to_string(line.lineNumber), Color::Cyan) << " | ";
        }
        output_ << line.content << std::endl;
    }
}

Color DiagnosticRenderer::getColorForLevel(DiagnosticLevel level) const {
    switch (level) {
        case DiagnosticLevel::Error:
        case DiagnosticLevel::Fatal:
            return Color::Red;
        case DiagnosticLevel::Warning:
            return Color::Yellow;
        case DiagnosticLevel::Info:
            return Color::Blue;
        case DiagnosticLevel::Note:
            return Color::Cyan;
        default:
            return Color::White;
    }
}

std::string DiagnosticRenderer::getTextPrefix(DiagnosticLevel level) const {
    switch (level) {
        case DiagnosticLevel::Error:
        case DiagnosticLevel::Fatal:
            return "error";
        case DiagnosticLevel::Warning:
            return "warning";
        case DiagnosticLevel::Info:
            return "info";
        case DiagnosticLevel::Note:
            return "note";
        default:
            return "unknown";
    }
}

std::string DiagnosticRenderer::getSymbolPrefix(DiagnosticLevel level) const {
    switch (level) {
        case DiagnosticLevel::Error:
        case DiagnosticLevel::Fatal:
            return "error";
        case DiagnosticLevel::Warning:
            return "warning";
        case DiagnosticLevel::Info:
            return "info";
        case DiagnosticLevel::Note:
            return "note";
        default:
            return "?";
    }
}

std::string DiagnosticRenderer::colorize(const std::string& text, Color color) const {
    if (!useColors_) {
        return text;
    }

    std::stringstream ss;
    ss << "\033[" << static_cast<int>(color) << "m" << text << "\033[0m";
    return ss.str();
}

std::string DiagnosticRenderer::escapeJSON(const std::string& text) const {
    std::string result;
    for (char c : text) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::string DiagnosticRenderer::formatLocation(const SourceLocation& location) const {
    std::stringstream ss;
    ss << location.fileName << ":" << location.line << ":" << location.column;
    return ss.str();
}

std::string DiagnosticRenderer::getIndentation(size_t width) const {
    return std::string(width, ' ');
}

std::string DiagnosticRenderer::truncateLine(const std::string& line, size_t maxWidth) const {
    if (line.size() <= maxWidth) {
        return line;
    }

    return line.substr(0, maxWidth - 3) + "...";
}

void DiagnosticRenderer::incrementCount(DiagnosticLevel level) {
    diagnosticCounts_[level]++;
}

// ============================================================================
// DiagnosticFormatter - Implementación
// ============================================================================

std::string DiagnosticFormatter::format(const Diagnostic& diagnostic,
                                      DiagnosticStyle style, bool useColors) {
    std::stringstream ss;
    DiagnosticRenderer renderer(ss, style, useColors);
    renderer.render(diagnostic);
    return ss.str();
}

std::string DiagnosticFormatter::format(const std::vector<Diagnostic>& diagnostics,
                                      DiagnosticStyle style, bool useColors) {
    std::stringstream ss;
    DiagnosticRenderer renderer(ss, style, useColors);
    renderer.render(diagnostics);
    return ss.str();
}

std::string DiagnosticFormatter::generateSummary(const std::vector<Diagnostic>& diagnostics) {
    auto counts = DiagnosticUtils::countByLevel(diagnostics);

    std::stringstream ss;
    ss << "Summary:" << std::endl;

    for (const auto& [level, count] : counts) {
        if (count > 0) {
            std::string levelName;
            switch (level) {
                case DiagnosticLevel::Error: levelName = "errors"; break;
                case DiagnosticLevel::Fatal: levelName = "fatal errors"; break;
                case DiagnosticLevel::Warning: levelName = "warnings"; break;
                case DiagnosticLevel::Info: levelName = "info"; break;
                case DiagnosticLevel::Note: levelName = "notes"; break;
                default: levelName = "unknown"; break;
            }
            ss << "  " << count << " " << levelName << std::endl;
        }
    }

    return ss.str();
}

// ============================================================================
// DiagnosticUtils - Implementación
// ============================================================================

std::vector<Diagnostic> DiagnosticUtils::filterByLevel(const std::vector<Diagnostic>& diagnostics, DiagnosticLevel level) {
    std::vector<Diagnostic> filtered;
    for (const auto& diag : diagnostics) {
        if (diag.level == level) {
            filtered.push_back(diag);
        }
    }
    return filtered;
}

std::vector<Diagnostic> DiagnosticUtils::filterByFile(const std::vector<Diagnostic>& diagnostics, const std::string& fileName) {
    std::vector<Diagnostic> filtered;
    for (const auto& diag : diagnostics) {
        if (diag.location.fileName == fileName) {
            filtered.push_back(diag);
        }
    }
    return filtered;
}

void DiagnosticUtils::sortByLocation(std::vector<Diagnostic>& diagnostics) {
    std::sort(diagnostics.begin(), diagnostics.end(),
              [](const Diagnostic& a, const Diagnostic& b) {
                  if (a.location.fileName != b.location.fileName) {
                      return a.location.fileName < b.location.fileName;
                  }
                  if (a.location.line != b.location.line) {
                      return a.location.line < b.location.line;
                  }
                  return a.location.column < b.location.column;
              });
}

std::unordered_map<DiagnosticLevel, size_t> DiagnosticUtils::countByLevel(const std::vector<Diagnostic>& diagnostics) {
    std::unordered_map<DiagnosticLevel, size_t> counts;
    for (const auto& diag : diagnostics) {
        counts[diag.level]++;
    }
    return counts;
}

bool DiagnosticUtils::hasFatalErrors(const std::vector<Diagnostic>& diagnostics) {
    return std::any_of(diagnostics.begin(), diagnostics.end(),
                      [](const Diagnostic& d) { return d.level == DiagnosticLevel::Fatal; });
}

DiagnosticLevel DiagnosticUtils::getMostSevereLevel(const std::vector<Diagnostic>& diagnostics) {
    DiagnosticLevel mostSevere = DiagnosticLevel::Note;

    for (const auto& diag : diagnostics) {
        if (diag.level == DiagnosticLevel::Fatal) {
            return DiagnosticLevel::Fatal;
        } else if (diag.level == DiagnosticLevel::Error) {
            mostSevere = DiagnosticLevel::Error;
        } else if (diag.level == DiagnosticLevel::Warning && mostSevere == DiagnosticLevel::Note) {
            mostSevere = DiagnosticLevel::Warning;
        }
    }

    return mostSevere;
}

} // namespace cpp20::compiler::diagnostics
