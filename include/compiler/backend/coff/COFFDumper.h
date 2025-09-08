#pragma once

#include <string>
#include <iostream>

namespace cpp20::compiler::backend::coff {

struct IMAGE_FILE_HEADER;
struct IMAGE_SECTION_HEADER;
struct IMAGE_SYMBOL;

/**
 * @brief Lector y dumper de archivos COFF para validación
 */
class COFFDumper {
public:
    COFFDumper();

    /**
     * @brief Lee y muestra el contenido de un archivo COFF
     * @param filename Nombre del archivo COFF
     * @param output Stream de salida
     * @return true si la operación fue exitosa
     */
    bool dumpFile(const std::string& filename, std::ostream& output);

    /**
     * @brief Lee y muestra el contenido de datos COFF en memoria
     * @param data Puntero a los datos COFF
     * @param size Tamaño de los datos
     * @param output Stream de salida
     * @return true si la operación fue exitosa
     */
    bool dumpObject(const uint8_t* data, size_t size, std::ostream& output);

private:
    void dumpFileHeader(const IMAGE_FILE_HEADER& header, std::ostream& output);
    void dumpCharacteristics(uint16_t characteristics, std::ostream& output);
    void dumpSectionHeader(const IMAGE_SECTION_HEADER& header, std::ostream& output);
    void dumpSectionCharacteristics(uint32_t characteristics, std::ostream& output);
    void dumpSectionData(const IMAGE_SECTION_HEADER& header,
                        const uint8_t* data, size_t size, std::ostream& output);
    void dumpSymbolTable(const uint8_t* data, uint32_t numSymbols, std::ostream& output);
    void dumpSymbol(const IMAGE_SYMBOL& symbol, std::ostream& output);

    bool validateFileHeader(const IMAGE_FILE_HEADER& header);
    std::string getSectionName(const IMAGE_SECTION_HEADER& header);
    size_t getFileSize(const std::string& filename);
};

// ========================================================================
// Funciones helper
// ========================================================================

/**
 * @brief Crea un dump de un archivo COFF a un stream
 * @param filename Archivo COFF de entrada
 * @param output Stream de salida
 * @return true si la operación fue exitosa
 */
bool dumpCOFFFile(const std::string& filename, std::ostream& output);

/**
 * @brief Crea un dump de un archivo COFF a un archivo
 * @param filename Archivo COFF de entrada
 * @param outputFilename Archivo de salida
 * @return true si la operación fue exitosa
 */
bool dumpCOFFFile(const std::string& filename, const std::string& outputFilename);

} // namespace cpp20::compiler::backend::coff
