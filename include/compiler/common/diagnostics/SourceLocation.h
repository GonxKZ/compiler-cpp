#pragma once

#include <cstdint>
#include <string>
#include <compare>

namespace cpp20::compiler::diagnostics {

/**
 * @brief Representa una posición específica en el código fuente
 *
 * Esta clase maneja ubicaciones precisas en archivos fuente, incluyendo
 * línea, columna, y offset absoluto. Es fundamental para el sistema
 * de diagnósticos del compilador.
 */
class SourceLocation {
public:
    // Constructores
    constexpr SourceLocation() noexcept = default;

    constexpr SourceLocation(
        uint32_t line,
        uint32_t column,
        uint32_t offset = 0,
        uint32_t fileId = 0
    ) noexcept
        : line_(line), column_(column), offset_(offset), fileId_(fileId) {}

    // Getters
    constexpr uint32_t line() const noexcept { return line_; }
    constexpr uint32_t column() const noexcept { return column_; }
    constexpr uint32_t offset() const noexcept { return offset_; }
    constexpr uint32_t fileId() const noexcept { return fileId_; }

    // Setters
    void setLine(uint32_t line) noexcept { line_ = line; }
    void setColumn(uint32_t column) noexcept { column_ = column; }
    void setOffset(uint32_t offset) noexcept { offset_ = offset; }
    void setFileId(uint32_t fileId) noexcept { fileId_ = fileId; }

    // Operadores de comparación
    constexpr auto operator<=>(const SourceLocation&) const noexcept = default;

    // Operadores aritméticos para manipulación de posiciones
    constexpr SourceLocation operator+(uint32_t offset) const noexcept {
        return SourceLocation(line_, column_ + offset, offset_ + offset, fileId_);
    }

    constexpr SourceLocation& operator+=(uint32_t offset) noexcept {
        column_ += offset;
        offset_ += offset;
        return *this;
    }

    // Utilidades
    constexpr bool isValid() const noexcept {
        return line_ > 0 && column_ > 0;
    }

    constexpr bool isInvalid() const noexcept {
        return !isValid();
    }

    // Conversión a string para debugging
    std::string toString() const;

    // Constantes útiles
    static constexpr SourceLocation invalid() noexcept {
        return SourceLocation(0, 0, 0, 0);
    }

private:
    uint32_t line_ = 0;     // Número de línea (1-based)
    uint32_t column_ = 0;   // Número de columna (1-based)
    uint32_t offset_ = 0;   // Offset absoluto en el archivo
    uint32_t fileId_ = 0;   // ID del archivo en el SourceManager
};

/**
 * @brief Representa un rango de código fuente
 *
 * Un SourceRange define el inicio y fin de un fragmento de código,
 * útil para destacar secciones específicas en diagnósticos.
 */
class SourceRange {
public:
    constexpr SourceRange() noexcept = default;

    constexpr SourceRange(SourceLocation start, SourceLocation end) noexcept
        : start_(start), end_(end) {}

    // Getters
    constexpr const SourceLocation& start() const noexcept { return start_; }
    constexpr const SourceLocation& end() const noexcept { return end_; }

    // Setters
    void setStart(SourceLocation start) noexcept { start_ = start; }
    void setEnd(SourceLocation end) noexcept { end_ = end; }

    // Utilidades
    constexpr bool isValid() const noexcept {
        return start_.isValid() && end_.isValid();
    }

    constexpr bool isEmpty() const noexcept {
        return start_ == end_;
    }

    // Longitud del rango
    constexpr uint32_t length() const noexcept {
        if (!isValid()) return 0;
        return end_.offset() - start_.offset();
    }

    // Conversión a string
    std::string toString() const;

    // Constantes
    static constexpr SourceRange invalid() noexcept {
        return SourceRange(SourceLocation::invalid(), SourceLocation::invalid());
    }

private:
    SourceLocation start_;
    SourceLocation end_;
};

} // namespace cpp20::compiler::diagnostics
