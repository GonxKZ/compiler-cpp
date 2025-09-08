#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace cpp20::compiler::backend::coff {

// ========================================================================
// Constantes COFF según especificación Microsoft PE/COFF
// ========================================================================

// Machine types
constexpr uint16_t IMAGE_FILE_MACHINE_AMD64 = 0x8664;

// Characteristics
constexpr uint16_t IMAGE_FILE_RELOCS_STRIPPED         = 0x0001;
constexpr uint16_t IMAGE_FILE_EXECUTABLE_IMAGE        = 0x0002;
constexpr uint16_t IMAGE_FILE_LINE_NUMS_STRIPPED      = 0x0004;
constexpr uint16_t IMAGE_FILE_LOCAL_SYMS_STRIPPED     = 0x0008;
constexpr uint16_t IMAGE_FILE_AGGRESSIVE_WS_TRIM     = 0x0010;
constexpr uint16_t IMAGE_FILE_LARGE_ADDRESS_AWARE    = 0x0020;
constexpr uint16_t IMAGE_FILE_BYTES_REVERSED_LO      = 0x0080;
constexpr uint16_t IMAGE_FILE_32BIT_MACHINE          = 0x0100;
constexpr uint16_t IMAGE_FILE_DEBUG_STRIPPED         = 0x0200;
constexpr uint16_t IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP = 0x0400;
constexpr uint16_t IMAGE_FILE_NET_RUN_FROM_SWAP     = 0x0800;
constexpr uint16_t IMAGE_FILE_SYSTEM                 = 0x1000;
constexpr uint16_t IMAGE_FILE_DLL                    = 0x2000;
constexpr uint16_t IMAGE_FILE_UP_SYSTEM_ONLY         = 0x4000;
constexpr uint16_t IMAGE_FILE_BYTES_REVERSED_HI      = 0x8000;

// Section characteristics
constexpr uint32_t IMAGE_SCN_TYPE_NO_PAD             = 0x00000008;
constexpr uint32_t IMAGE_SCN_CNT_CODE                = 0x00000020;
constexpr uint32_t IMAGE_SCN_CNT_INITIALIZED_DATA    = 0x00000040;
constexpr uint32_t IMAGE_SCN_CNT_UNINITIALIZED_DATA  = 0x00000080;
constexpr uint32_t IMAGE_SCN_LNK_OTHER               = 0x00000100;
constexpr uint32_t IMAGE_SCN_LNK_INFO                = 0x00000200;
constexpr uint32_t IMAGE_SCN_LNK_REMOVE              = 0x00000800;
constexpr uint32_t IMAGE_SCN_LNK_COMDAT              = 0x00001000;
constexpr uint32_t IMAGE_SCN_NO_DEFER_SPEC_EXC       = 0x00004000;
constexpr uint32_t IMAGE_SCN_GPREL                   = 0x00008000;
constexpr uint32_t IMAGE_SCN_MEM_PURGEABLE           = 0x00020000;
constexpr uint32_t IMAGE_SCN_MEM_LOCKED              = 0x00040000;
constexpr uint32_t IMAGE_SCN_MEM_PRELOAD             = 0x00080000;
constexpr uint32_t IMAGE_SCN_ALIGN_1BYTES            = 0x00100000;
constexpr uint32_t IMAGE_SCN_ALIGN_2BYTES            = 0x00200000;
constexpr uint32_t IMAGE_SCN_ALIGN_4BYTES            = 0x00300000;
constexpr uint32_t IMAGE_SCN_ALIGN_8BYTES            = 0x00400000;
constexpr uint32_t IMAGE_SCN_ALIGN_16BYTES           = 0x00500000;
constexpr uint32_t IMAGE_SCN_ALIGN_32BYTES           = 0x00600000;
constexpr uint32_t IMAGE_SCN_ALIGN_64BYTES           = 0x00700000;
constexpr uint32_t IMAGE_SCN_ALIGN_128BYTES          = 0x00800000;
constexpr uint32_t IMAGE_SCN_ALIGN_256BYTES          = 0x00900000;
constexpr uint32_t IMAGE_SCN_ALIGN_512BYTES          = 0x00A00000;
constexpr uint32_t IMAGE_SCN_ALIGN_1024BYTES         = 0x00B00000;
constexpr uint32_t IMAGE_SCN_ALIGN_2048BYTES         = 0x00C00000;
constexpr uint32_t IMAGE_SCN_ALIGN_4096BYTES         = 0x00D00000;
constexpr uint32_t IMAGE_SCN_ALIGN_8192BYTES         = 0x00E00000;
constexpr uint32_t IMAGE_SCN_LNK_NRELOC_OVFL         = 0x01000000;
constexpr uint32_t IMAGE_SCN_MEM_DISCARDABLE         = 0x02000000;
constexpr uint32_t IMAGE_SCN_MEM_NOT_CACHED          = 0x04000000;
constexpr uint32_t IMAGE_SCN_MEM_NOT_PAGED           = 0x08000000;
constexpr uint32_t IMAGE_SCN_MEM_SHARED              = 0x10000000;
constexpr uint32_t IMAGE_SCN_MEM_EXECUTE             = 0x20000000;
constexpr uint32_t IMAGE_SCN_MEM_READ                = 0x40000000;
constexpr uint32_t IMAGE_SCN_MEM_WRITE               = 0x80000000;

// Symbol types
constexpr uint8_t IMAGE_SYM_TYPE_NULL                = 0;
constexpr uint8_t IMAGE_SYM_TYPE_VOID                = 1;
constexpr uint8_t IMAGE_SYM_TYPE_CHAR                = 2;
constexpr uint8_t IMAGE_SYM_TYPE_SHORT               = 3;
constexpr uint8_t IMAGE_SYM_TYPE_INT                 = 4;
constexpr uint8_t IMAGE_SYM_TYPE_LONG                = 5;
constexpr uint8_t IMAGE_SYM_TYPE_FLOAT               = 6;
constexpr uint8_t IMAGE_SYM_TYPE_DOUBLE              = 7;
constexpr uint8_t IMAGE_SYM_TYPE_STRUCT              = 8;
constexpr uint8_t IMAGE_SYM_TYPE_UNION               = 9;
constexpr uint8_t IMAGE_SYM_TYPE_ENUM                = 10;
constexpr uint8_t IMAGE_SYM_TYPE_MOE                 = 11;
constexpr uint8_t IMAGE_SYM_TYPE_BYTE                = 12;
constexpr uint8_t IMAGE_SYM_TYPE_WORD                = 13;
constexpr uint8_t IMAGE_SYM_TYPE_UINT                = 14;
constexpr uint8_t IMAGE_SYM_TYPE_DWORD               = 15;

// Symbol classes
constexpr uint8_t IMAGE_SYM_CLASS_END_OF_FUNCTION    = 0xFF;
constexpr uint8_t IMAGE_SYM_CLASS_NULL               = 0;
constexpr uint8_t IMAGE_SYM_CLASS_AUTOMATIC          = 1;
constexpr uint8_t IMAGE_SYM_CLASS_EXTERNAL           = 2;
constexpr uint8_t IMAGE_SYM_CLASS_STATIC             = 3;
constexpr uint8_t IMAGE_SYM_CLASS_REGISTER           = 4;
constexpr uint8_t IMAGE_SYM_CLASS_EXTERNAL_DEF       = 5;
constexpr uint8_t IMAGE_SYM_CLASS_LABEL              = 6;
constexpr uint8_t IMAGE_SYM_CLASS_UNDEFINED_LABEL    = 7;
constexpr uint8_t IMAGE_SYM_CLASS_MEMBER_OF_STRUCT   = 8;
constexpr uint8_t IMAGE_SYM_CLASS_ARGUMENT           = 9;
constexpr uint8_t IMAGE_SYM_CLASS_STRUCT_TAG         = 10;
constexpr uint8_t IMAGE_SYM_CLASS_MEMBER_OF_UNION    = 11;
constexpr uint8_t IMAGE_SYM_CLASS_UNION_TAG          = 12;
constexpr uint8_t IMAGE_SYM_CLASS_TYPE_DEFINITION    = 13;
constexpr uint8_t IMAGE_SYM_CLASS_UNDEFINED_STATIC   = 14;
constexpr uint8_t IMAGE_SYM_CLASS_ENUM_TAG           = 15;
constexpr uint8_t IMAGE_SYM_CLASS_MEMBER_OF_ENUM     = 16;
constexpr uint8_t IMAGE_SYM_CLASS_REGISTER_PARAM     = 17;
constexpr uint8_t IMAGE_SYM_CLASS_BIT_FIELD          = 18;
constexpr uint8_t IMAGE_SYM_CLASS_BLOCK              = 100;
constexpr uint8_t IMAGE_SYM_CLASS_FUNCTION           = 101;
constexpr uint8_t IMAGE_SYM_CLASS_END_OF_STRUCT      = 102;
constexpr uint8_t IMAGE_SYM_CLASS_FILE               = 103;
constexpr uint8_t IMAGE_SYM_CLASS_SECTION            = 104;
constexpr uint8_t IMAGE_SYM_CLASS_WEAK_EXTERNAL      = 105;
constexpr uint8_t IMAGE_SYM_CLASS_CLR_TOKEN          = 106;

// Relocation types for AMD64
constexpr uint16_t IMAGE_REL_AMD64_ABSOLUTE          = 0x0000;
constexpr uint16_t IMAGE_REL_AMD64_ADDR64            = 0x0001;
constexpr uint16_t IMAGE_REL_AMD64_ADDR32            = 0x0002;
constexpr uint16_t IMAGE_REL_AMD64_ADDR32NB          = 0x0003;
constexpr uint16_t IMAGE_REL_AMD64_REL32             = 0x0004;
constexpr uint16_t IMAGE_REL_AMD64_REL32_1           = 0x0005;
constexpr uint16_t IMAGE_REL_AMD64_REL32_2           = 0x0006;
constexpr uint16_t IMAGE_REL_AMD64_REL32_3           = 0x0007;
constexpr uint16_t IMAGE_REL_AMD64_REL32_4           = 0x0008;
constexpr uint16_t IMAGE_REL_AMD64_REL32_5           = 0x0009;
constexpr uint16_t IMAGE_REL_AMD64_SECTION           = 0x000A;
constexpr uint16_t IMAGE_REL_AMD64_SECREL            = 0x000B;
constexpr uint16_t IMAGE_REL_AMD64_SECREL7           = 0x000C;
constexpr uint16_t IMAGE_REL_AMD64_TOKEN             = 0x000D;
constexpr uint16_t IMAGE_REL_AMD64_SREL32            = 0x000E;
constexpr uint16_t IMAGE_REL_AMD64_PAIR              = 0x000F;
constexpr uint16_t IMAGE_REL_AMD64_SSPAN32           = 0x0010;

// ========================================================================
// Estructuras COFF según especificación
// ========================================================================

#pragma pack(push, 1)

// File Header
struct IMAGE_FILE_HEADER {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
};

// Section Header
struct IMAGE_SECTION_HEADER {
    char Name[8];
    union {
        uint32_t PhysicalAddress;
        uint32_t VirtualSize;
    } Misc;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
};

// Symbol Table Entry
struct IMAGE_SYMBOL {
    union {
        char ShortName[8];
        struct {
            uint32_t Zeroes;
            uint32_t Offset;
        } Name;
    } N;
    uint32_t Value;
    int16_t SectionNumber;
    uint16_t Type;
    uint8_t StorageClass;
    uint8_t NumberOfAuxSymbols;
};

// Relocation Entry
struct IMAGE_RELOCATION {
    uint32_t VirtualAddress;
    uint32_t SymbolTableIndex;
    uint16_t Type;
};

// Import structures (simplified)
struct IMAGE_IMPORT_DESCRIPTOR {
    union {
        uint32_t Characteristics;
        uint32_t OriginalFirstThunk;
    };
    uint32_t TimeDateStamp;
    uint32_t ForwarderChain;
    uint32_t Name;
    uint32_t FirstThunk;
};

#pragma pack(pop)

// ========================================================================
// Clases helper para trabajar con COFF
// ========================================================================

/**
 * @brief Representa una sección COFF
 */
struct COFFSection {
    std::string name;
    std::vector<uint8_t> data;
    uint32_t characteristics;
    std::vector<IMAGE_RELOCATION> relocations;
    uint32_t virtualAddress = 0;

    COFFSection(std::string n, uint32_t chars = 0)
        : name(std::move(n)), characteristics(chars) {}

    size_t size() const { return data.size(); }
    bool isEmpty() const { return data.empty(); }
};

/**
 * @brief Representa un símbolo COFF
 */
struct COFFSymbol {
    std::string name;
    uint32_t value = 0;
    int16_t sectionNumber = 0;
    uint16_t type = 0;
    uint8_t storageClass = 0;
    uint8_t auxSymbols = 0;

    COFFSymbol(std::string n, uint8_t storage = IMAGE_SYM_CLASS_EXTERNAL)
        : name(std::move(n)), storageClass(storage) {}
};

/**
 * @brief Representa un objeto COFF completo
 */
struct COFFObject {
    IMAGE_FILE_HEADER header;
    std::vector<COFFSection> sections;
    std::vector<COFFSymbol> symbols;
    std::vector<std::string> stringTable;

    COFFObject() {
        // Initialize header
        header.Machine = IMAGE_FILE_MACHINE_AMD64;
        header.NumberOfSections = 0;
        header.TimeDateStamp = 0; // Will be set when writing
        header.PointerToSymbolTable = 0;
        header.NumberOfSymbols = 0;
        header.SizeOfOptionalHeader = 0;
        header.Characteristics = IMAGE_FILE_RELOCS_STRIPPED |
                                IMAGE_FILE_EXECUTABLE_IMAGE |
                                IMAGE_FILE_LINE_NUMS_STRIPPED |
                                IMAGE_FILE_LOCAL_SYMS_STRIPPED |
                                IMAGE_FILE_LARGE_ADDRESS_AWARE;
    }

    void addSection(COFFSection section) {
        sections.push_back(std::move(section));
        header.NumberOfSections = static_cast<uint16_t>(sections.size());
    }

    void addSymbol(COFFSymbol symbol) {
        symbols.push_back(std::move(symbol));
        header.NumberOfSymbols = static_cast<uint32_t>(symbols.size());
    }
};

} // namespace cpp20::compiler::backend::coff
