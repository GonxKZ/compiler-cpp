/**
 * @file COFFDumper.cpp
 * @brief Lector y dumper de archivos COFF para validación
 */

#include <compiler/backend/coff/COFFDumper.h>
#include <compiler/backend/coff/COFFTypes.h>
#include <compiler/common/utils/FileUtils.h>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace cpp20::compiler::backend::coff {

// ========================================================================
// COFFDumper implementation
// ========================================================================

COFFDumper::COFFDumper() = default;

bool COFFDumper::dumpFile(const std::string& filename, std::ostream& output) {
    try {
        std::string content = utils::readFile(filename);
        if (content.empty()) {
            output << "Error: Empty or invalid file" << std::endl;
            return false;
        }

        const uint8_t* data = reinterpret_cast<const uint8_t*>(content.data());
        size_t size = content.size();

        return dumpObject(data, size, output);

    } catch (const std::exception& e) {
        output << "Error reading file: " << e.what() << std::endl;
        return false;
    }
}

bool COFFDumper::dumpObject(const uint8_t* data, size_t size, std::ostream& output) {
    if (size < sizeof(IMAGE_FILE_HEADER)) {
        output << "Error: File too small for COFF header" << std::endl;
        return false;
    }

    // Read file header
    const IMAGE_FILE_HEADER* header = reinterpret_cast<const IMAGE_FILE_HEADER*>(data);
    dumpFileHeader(*header, output);

    // Validate header
    if (!validateFileHeader(*header)) {
        output << "Warning: Invalid COFF file header" << std::endl;
    }

    size_t offset = sizeof(IMAGE_FILE_HEADER);

    // Read section headers
    for (uint16_t i = 0; i < header->NumberOfSections; ++i) {
        if (offset + sizeof(IMAGE_SECTION_HEADER) > size) {
            output << "Error: Truncated section header" << std::endl;
            return false;
        }

        const IMAGE_SECTION_HEADER* sectionHeader =
            reinterpret_cast<const IMAGE_SECTION_HEADER*>(data + offset);

        dumpSectionHeader(*sectionHeader, output);
        offset += sizeof(IMAGE_SECTION_HEADER);
    }

    // Read sections data
    for (uint16_t i = 0; i < header->NumberOfSections; ++i) {
        const IMAGE_SECTION_HEADER* sectionHeader =
            reinterpret_cast<const IMAGE_SECTION_HEADER*>(
                data + sizeof(IMAGE_FILE_HEADER) + i * sizeof(IMAGE_SECTION_HEADER));

        dumpSectionData(*sectionHeader, data, size, output);
    }

    // Read symbol table if present
    if (header->PointerToSymbolTable > 0 && header->NumberOfSymbols > 0) {
        if (header->PointerToSymbolTable + header->NumberOfSymbols * sizeof(IMAGE_SYMBOL) > size) {
            output << "Error: Symbol table extends beyond file" << std::endl;
            return false;
        }

        dumpSymbolTable(data + header->PointerToSymbolTable,
                       header->NumberOfSymbols, output);
    }

    return true;
}

void COFFDumper::dumpFileHeader(const IMAGE_FILE_HEADER& header, std::ostream& output) {
    output << "COFF File Header:" << std::endl;
    output << "  Machine:              0x" << std::hex << header.Machine << std::dec;

    switch (header.Machine) {
        case IMAGE_FILE_MACHINE_AMD64:
            output << " (AMD64)";
            break;
        default:
            output << " (Unknown)";
            break;
    }
    output << std::endl;

    output << "  Number of Sections:   " << header.NumberOfSections << std::endl;
    output << "  TimeDateStamp:        " << header.TimeDateStamp << std::endl;
    output << "  PointerToSymbolTable: 0x" << std::hex << header.PointerToSymbolTable << std::dec << std::endl;
    output << "  NumberOfSymbols:      " << header.NumberOfSymbols << std::endl;
    output << "  SizeOfOptionalHeader: " << header.SizeOfOptionalHeader << std::endl;
    output << "  Characteristics:      0x" << std::hex << header.Characteristics << std::dec << std::endl;

    // Decode characteristics
    dumpCharacteristics(header.Characteristics, output);
}

void COFFDumper::dumpCharacteristics(uint16_t characteristics, std::ostream& output) {
    output << "    Characteristics: ";
    if (characteristics & IMAGE_FILE_RELOCS_STRIPPED) output << "RELOCS_STRIPPED ";
    if (characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) output << "EXECUTABLE_IMAGE ";
    if (characteristics & IMAGE_FILE_LINE_NUMS_STRIPPED) output << "LINE_NUMS_STRIPPED ";
    if (characteristics & IMAGE_FILE_LOCAL_SYMS_STRIPPED) output << "LOCAL_SYMS_STRIPPED ";
    if (characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) output << "LARGE_ADDRESS_AWARE ";
    output << std::endl;
}

void COFFDumper::dumpSectionHeader(const IMAGE_SECTION_HEADER& header, std::ostream& output) {
    output << std::endl << "Section Header:" << std::endl;
    output << "  Name:                 " << std::string(header.Name, strnlen(header.Name, 8)) << std::endl;
    output << "  Virtual Size:         0x" << std::hex << header.Misc.VirtualSize << std::dec << std::endl;
    output << "  Virtual Address:      0x" << std::hex << header.VirtualAddress << std::dec << std::endl;
    output << "  Size of Raw Data:     " << header.SizeOfRawData << std::endl;
    output << "  Pointer to Raw Data:  0x" << std::hex << header.PointerToRawData << std::dec << std::endl;
    output << "  Pointer to Relocs:    0x" << std::hex << header.PointerToRelocations << std::dec << std::endl;
    output << "  Number of Relocs:     " << header.NumberOfRelocations << std::endl;
    output << "  Characteristics:      0x" << std::hex << header.Characteristics << std::dec << std::endl;

    // Decode section characteristics
    dumpSectionCharacteristics(header.Characteristics, output);
}

void COFFDumper::dumpSectionCharacteristics(uint32_t characteristics, std::ostream& output) {
    output << "    Section flags: ";
    if (characteristics & IMAGE_SCN_CNT_CODE) output << "CODE ";
    if (characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) output << "INITIALIZED_DATA ";
    if (characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) output << "UNINITIALIZED_DATA ";
    if (characteristics & IMAGE_SCN_MEM_READ) output << "READ ";
    if (characteristics & IMAGE_SCN_MEM_WRITE) output << "WRITE ";
    if (characteristics & IMAGE_SCN_MEM_EXECUTE) output << "EXECUTE ";
    output << std::endl;
}

void COFFDumper::dumpSectionData(const IMAGE_SECTION_HEADER& header,
                                const uint8_t* data, size_t size, std::ostream& output) {
    if (header.PointerToRawData == 0 || header.SizeOfRawData == 0) {
        return;
    }

    if (header.PointerToRawData + header.SizeOfRawData > size) {
        output << "Error: Section data extends beyond file" << std::endl;
        return;
    }

    output << std::endl << "Section Data (" << std::string(header.Name, strnlen(header.Name, 8)) << "):" << std::endl;

    // Dump first 16 bytes as hex
    const uint8_t* sectionData = data + header.PointerToRawData;
    size_t dumpSize = std::min<size_t>(16, header.SizeOfRawData);

    output << "  ";
    for (size_t i = 0; i < dumpSize; ++i) {
        output << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<int>(sectionData[i]) << " ";
    }

    if (header.SizeOfRawData > 16) {
        output << "... (" << header.SizeOfRawData << " bytes total)";
    }
    output << std::dec << std::endl;
}

void COFFDumper::dumpSymbolTable(const uint8_t* data, uint32_t numSymbols, std::ostream& output) {
    output << std::endl << "Symbol Table:" << std::endl;

    for (uint32_t i = 0; i < numSymbols; ++i) {
        const IMAGE_SYMBOL* symbol = reinterpret_cast<const IMAGE_SYMBOL*>(
            data + i * sizeof(IMAGE_SYMBOL));

        dumpSymbol(*symbol, output);
    }
}

void COFFDumper::dumpSymbol(const IMAGE_SYMBOL& symbol, std::ostream& output) {
    output << "  Symbol:" << std::endl;
    output << "    Value:         0x" << std::hex << symbol.Value << std::dec << std::endl;
    output << "    Section:       " << symbol.SectionNumber << std::endl;
    output << "    Type:          " << symbol.Type << std::endl;
    output << "    Storage Class: " << static_cast<int>(symbol.StorageClass) << std::endl;
    output << "    Aux Symbols:   " << static_cast<int>(symbol.NumberOfAuxSymbols) << std::endl;
}

bool COFFDumper::validateFileHeader(const IMAGE_FILE_HEADER& header) {
    // Check machine type
    if (header.Machine != IMAGE_FILE_MACHINE_AMD64) {
        return false;
    }

    // Check for reasonable number of sections
    if (header.NumberOfSections == 0 || header.NumberOfSections > 96) {
        return false;
    }

    // Check characteristics
    uint16_t required = IMAGE_FILE_RELOCS_STRIPPED | IMAGE_FILE_EXECUTABLE_IMAGE;
    if ((header.Characteristics & required) != required) {
        return false;
    }

    return true;
}

std::string COFFDumper::getSectionName(const IMAGE_SECTION_HEADER& header) {
    return std::string(header.Name, strnlen(header.Name, 8));
}

size_t COFFDumper::getFileSize(const std::string& filename) {
    try {
        std::string content = utils::readFile(filename);
        return content.size();
    } catch (...) {
        return 0;
    }
}

// ========================================================================
// Funciones helper
// ========================================================================

bool dumpCOFFFile(const std::string& filename, std::ostream& output) {
    COFFDumper dumper;
    return dumper.dumpFile(filename, output);
}

bool dumpCOFFFile(const std::string& filename, const std::string& outputFilename) {
    std::ofstream output(outputFilename);
    if (!output) {
        return false;
    }
    return dumpCOFFFile(filename, output);
}

} // namespace cpp20::compiler::backend::coff
