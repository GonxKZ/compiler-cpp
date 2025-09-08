#pragma once

#include <compiler/backend/coff/COFFTypes.h>
#include <string>
#include <vector>

namespace cpp20::compiler::backend::coff {

/**
 * @brief Escritor de archivos objeto COFF
 */
class COFFWriter {
public:
    COFFWriter();

    /**
     * @brief Escribe un objeto COFF a un archivo
     * @param object El objeto COFF a escribir
     * @param filename Nombre del archivo de salida
     * @return true si la escritura fue exitosa
     */
    bool writeObject(const COFFObject& object, const std::string& filename);

private:
    void writeHeader(const COFFObject& object, std::vector<uint8_t>& buffer);
    void writeSectionHeader(const COFFSection& section, IMAGE_SECTION_HEADER& header);
    void writeSections(const COFFObject& object, std::vector<uint8_t>& buffer);
    void writeSymbols(const COFFObject& object, std::vector<uint8_t>& buffer);
    void writeSymbol(const COFFSymbol& symbol, IMAGE_SYMBOL& entry);
    void writeStringTable(const COFFObject& object, std::vector<uint8_t>& buffer);
    void updateHeaderOffsets(std::vector<uint8_t>& buffer, const COFFObject& object);
    void appendBytes(std::vector<uint8_t>& buffer, const void* data, size_t size);
};

// ========================================================================
// Funciones helper
// ========================================================================

/**
 * @brief Crea un objeto COFF básico con secciones estándar
 * @return Objeto COFF con .text, .data, y .rdata
 */
COFFObject createBasicCOFFObject();

/**
 * @brief Escribe un objeto COFF a un archivo
 * @param object El objeto COFF a escribir
 * @param filename Nombre del archivo de salida
 * @return true si la escritura fue exitosa
 */
bool writeCOFFObject(const COFFObject& object, const std::string& filename);

} // namespace cpp20::compiler::backend::coff
