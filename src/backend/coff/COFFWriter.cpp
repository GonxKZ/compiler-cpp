/**
 * @file COFFWriter.cpp
 * @brief Escritor de archivos COFF para x86_64
 */

#include <compiler/backend/coff/COFFWriter.h>
#include <compiler/backend/coff/COFFTypes.h>
#include <compiler/common/utils/FileUtils.h>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <iostream>

namespace cpp20::compiler::backend::coff {

// ========================================================================
// COFFWriter implementation
// ========================================================================

COFFWriter::COFFWriter() = default;

bool COFFWriter::writeObject(const COFFObject& object, const std::string& filename) {
    try {
        std::vector<uint8_t> buffer;
        writeHeader(object, buffer);
        writeSections(object, buffer);
        writeSymbols(object, buffer);
        writeStringTable(object, buffer);

        // Update header with final positions
        updateHeaderOffsets(buffer, object);

        // Write to file
        cpp20::compiler::common::utils::writeFile(filename, std::string(buffer.begin(), buffer.end()));
        return true;

    } catch (const std::exception& e) {
        // Report error - TODO: Use diagnostic engine when available
        std::cerr << "Error writing COFF object to file '" << filename
                  << "': " << e.what() << std::endl;
        return false;
    }
}

void COFFWriter::writeHeader(const COFFObject& object, std::vector<uint8_t>& buffer) {
    // File header
    appendBytes(buffer, &object.header, sizeof(IMAGE_FILE_HEADER));

    // Section headers
    for (const auto& section : object.sections) {
        IMAGE_SECTION_HEADER sectionHeader = {};
        writeSectionHeader(section, sectionHeader);
        appendBytes(buffer, &sectionHeader, sizeof(IMAGE_SECTION_HEADER));
    }
}

void COFFWriter::writeSectionHeader(const COFFSection& section, IMAGE_SECTION_HEADER& header) {
    // Copy name (max 8 characters)
    std::string name = section.name;
    if (name.length() > 8) {
        name = name.substr(0, 8);
    }
    std::memcpy(header.Name, name.c_str(), name.length());

    // Section data info
    header.Misc.VirtualSize = static_cast<uint32_t>(section.size());
    header.VirtualAddress = section.virtualAddress;
    header.SizeOfRawData = static_cast<uint32_t>(section.size());
    header.PointerToRawData = 0;        // Will be updated later
    header.PointerToRelocations = 0;    // Will be updated later
    header.PointerToLinenumbers = 0;
    header.NumberOfRelocations = static_cast<uint16_t>(section.relocations.size());
    header.NumberOfLinenumbers = 0;
    header.Characteristics = section.characteristics;
}

void COFFWriter::writeSections(const COFFObject& object, std::vector<uint8_t>& buffer) {
    for (const auto& section : object.sections) {
        // Section data
        buffer.insert(buffer.end(), section.data.begin(), section.data.end());

        // Relocations for this section
        for (const auto& reloc : section.relocations) {
            appendBytes(buffer, &reloc, sizeof(IMAGE_RELOCATION));
        }
    }
}

void COFFWriter::writeSymbols(const COFFObject& object, std::vector<uint8_t>& buffer) {
    for (const auto& symbol : object.symbols) {
        IMAGE_SYMBOL symbolEntry = {};
        writeSymbol(symbol, symbolEntry);
        appendBytes(buffer, &symbolEntry, sizeof(IMAGE_SYMBOL));
    }
}

void COFFWriter::writeSymbol(const COFFSymbol& symbol, IMAGE_SYMBOL& entry) {
    // Symbol name
    if (symbol.name.length() <= 8) {
        // Short name
        std::memcpy(entry.N.ShortName, symbol.name.c_str(), symbol.name.length());
        entry.N.Name.Zeroes = 0;
    } else {
        // Long name - will be in string table
        entry.N.Name.Zeroes = 0;
        entry.N.Name.Offset = 0; // Will be updated with string table offset
    }

    // Symbol properties
    entry.Value = symbol.value;
    entry.SectionNumber = symbol.sectionNumber;
    entry.Type = symbol.type;
    entry.StorageClass = symbol.storageClass;
    entry.NumberOfAuxSymbols = symbol.auxSymbols;
}

void COFFWriter::writeStringTable(const COFFObject& object, std::vector<uint8_t>& buffer) {
    if (object.stringTable.empty()) {
        // Empty string table (just size)
        uint32_t size = sizeof(uint32_t);
        appendBytes(buffer, &size, sizeof(uint32_t));
        return;
    }

    // Calculate total size
    uint32_t totalSize = sizeof(uint32_t); // Size of string table
    for (const auto& str : object.stringTable) {
        totalSize += static_cast<uint32_t>(str.length() + 1); // +1 for null terminator
    }

    // Write size
    appendBytes(buffer, &totalSize, sizeof(uint32_t));

    // Write strings
    for (const auto& str : object.stringTable) {
        buffer.insert(buffer.end(), str.begin(), str.end());
        buffer.push_back(0); // Null terminator
    }
}

void COFFWriter::updateHeaderOffsets(std::vector<uint8_t>& buffer, const COFFObject& object) {
    size_t offset = sizeof(IMAGE_FILE_HEADER) + object.sections.size() * sizeof(IMAGE_SECTION_HEADER);

    // Update section data pointers
    for (size_t i = 0; i < object.sections.size(); ++i) {
        const auto& section = object.sections[i];
        size_t sectionHeaderOffset = sizeof(IMAGE_FILE_HEADER) + i * sizeof(IMAGE_SECTION_HEADER);

        // Update PointerToRawData
        size_t dataOffset = offset;
        std::memcpy(&buffer[sectionHeaderOffset + offsetof(IMAGE_SECTION_HEADER, PointerToRawData)],
                   &dataOffset, sizeof(uint32_t));

        // Update PointerToRelocations
        if (!section.relocations.empty()) {
            size_t relocOffset = offset + section.size();
            std::memcpy(&buffer[sectionHeaderOffset + offsetof(IMAGE_SECTION_HEADER, PointerToRelocations)],
                       &relocOffset, sizeof(uint32_t));
        }

        offset += section.size() + section.relocations.size() * sizeof(IMAGE_RELOCATION);
    }

    // Update symbol table pointer
    if (!object.symbols.empty()) {
        std::memcpy(&buffer[offsetof(IMAGE_FILE_HEADER, PointerToSymbolTable)],
                   &offset, sizeof(uint32_t));
    }
}

void COFFWriter::appendBytes(std::vector<uint8_t>& buffer, const void* data, size_t size) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    buffer.insert(buffer.end(), bytes, bytes + size);
}

// ========================================================================
// Helper functions
// ========================================================================

COFFObject createBasicCOFFObject() {
    COFFObject object;

    // Add .text section
    COFFSection textSection(".text", IMAGE_SCN_CNT_CODE |
                                      IMAGE_SCN_MEM_EXECUTE |
                                      IMAGE_SCN_MEM_READ);
    object.addSection(std::move(textSection));

    // Add .data section
    COFFSection dataSection(".data", IMAGE_SCN_CNT_INITIALIZED_DATA |
                                      IMAGE_SCN_MEM_READ |
                                      IMAGE_SCN_MEM_WRITE);
    object.addSection(std::move(dataSection));

    // Add .rdata section
    COFFSection rdataSection(".rdata", IMAGE_SCN_CNT_INITIALIZED_DATA |
                                        IMAGE_SCN_MEM_READ);
    object.addSection(std::move(rdataSection));

    return object;
}

bool writeCOFFObject(const COFFObject& object, const std::string& filename) {
    COFFWriter writer;
    return writer.writeObject(object, filename);
}

} // namespace cpp20::compiler::backend::coff
