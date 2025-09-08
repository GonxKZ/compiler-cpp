/**
 * @file test_coff_writer.cpp
 * @brief Tests para validar el COFF writer
 */

#include <compiler/backend/coff/COFFWriter.h>
#include <compiler/backend/coff/COFFDumper.h>
#include <compiler/backend/coff/COFFTypes.h>
#include <gtest/gtest.h>
#include <filesystem>

using namespace cpp20::compiler::backend::coff;
namespace fs = std::filesystem;

class COFFWriterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Crear directorio temporal para tests
        tempDir = fs::temp_directory_path() / "cpp20_compiler_test";
        fs::create_directories(tempDir);
    }

    void TearDown() override {
        // Limpiar archivos temporales
        fs::remove_all(tempDir);
    }

    fs::path tempDir;
    fs::path getTempFile(const std::string& name) {
        return tempDir / name;
    }
};

// ========================================================================
// Tests básicos de creación de objetos COFF
// ========================================================================

TEST_F(COFFWriterTest, CreateBasicCOFFObject) {
    COFFObject object = createBasicCOFFObject();

    // Verificar header
    EXPECT_EQ(object.header.Machine, IMAGE_FILE_MACHINE_AMD64);
    EXPECT_EQ(object.header.NumberOfSections, 3u); // .text, .data, .rdata

    // Verificar secciones
    ASSERT_EQ(object.sections.size(), 3u);

    EXPECT_EQ(object.sections[0].name, ".text");
    EXPECT_EQ(object.sections[1].name, ".data");
    EXPECT_EQ(object.sections[2].name, ".rdata");

    // Verificar características de secciones
    EXPECT_TRUE(object.sections[0].characteristics & IMAGE_SCN_CNT_CODE);
    EXPECT_TRUE(object.sections[0].characteristics & IMAGE_SCN_MEM_EXECUTE);

    EXPECT_TRUE(object.sections[1].characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA);
    EXPECT_TRUE(object.sections[1].characteristics & IMAGE_SCN_MEM_WRITE);

    EXPECT_TRUE(object.sections[2].characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA);
    EXPECT_TRUE(object.sections[2].characteristics & IMAGE_SCN_MEM_READ);
    EXPECT_FALSE(object.sections[2].characteristics & IMAGE_SCN_MEM_WRITE);
}

TEST_F(COFFWriterTest, WriteEmptyCOFFObject) {
    COFFObject object = createBasicCOFFObject();
    fs::path testFile = getTempFile("empty.obj");

    // Escribir objeto
    EXPECT_TRUE(writeCOFFObject(object, testFile.string()));

    // Verificar que el archivo existe
    EXPECT_TRUE(fs::exists(testFile));

    // Verificar tamaño mínimo
    auto fileSize = fs::file_size(testFile);
    EXPECT_GT(fileSize, sizeof(IMAGE_FILE_HEADER));
    EXPECT_GT(fileSize, sizeof(IMAGE_FILE_HEADER) + 3 * sizeof(IMAGE_SECTION_HEADER));
}

TEST_F(COFFWriterTest, WriteCOFFObjectWithData) {
    COFFObject object = createBasicCOFFObject();

    // Agregar datos a las secciones
    std::vector<uint8_t> textData = {0x90, 0x90, 0xC3}; // nop, nop, ret
    object.sections[0].data = textData;

    std::vector<uint8_t> dataData = {0x41, 0x42, 0x43, 0x44}; // "ABCD"
    object.sections[1].data = dataData;

    std::vector<uint8_t> rdataData = {0x48, 0x65, 0x6C, 0x6C, 0x6F}; // "Hello"
    object.sections[2].data = rdataData;

    fs::path testFile = getTempFile("with_data.obj");

    // Escribir objeto
    EXPECT_TRUE(writeCOFFObject(object, testFile.string()));

    // Verificar que el archivo existe y tiene el tamaño correcto
    EXPECT_TRUE(fs::exists(testFile));
    auto fileSize = fs::file_size(testFile);
    EXPECT_EQ(fileSize, sizeof(IMAGE_FILE_HEADER) +
                       3 * sizeof(IMAGE_SECTION_HEADER) +
                       textData.size() + dataData.size() + rdataData.size());
}

TEST_F(COFFWriterTest, AddSymbolsToCOFFObject) {
    COFFObject object = createBasicCOFFObject();

    // Agregar símbolos
    COFFSymbol mainSymbol("_main", IMAGE_SYM_CLASS_EXTERNAL);
    mainSymbol.value = 0;
    mainSymbol.sectionNumber = 1; // .text section
    mainSymbol.type = 0x20; // Function

    COFFSymbol dataSymbol("_global_var", IMAGE_SYM_CLASS_EXTERNAL);
    dataSymbol.value = 0;
    dataSymbol.sectionNumber = 2; // .data section
    dataSymbol.type = 0x00; // Data

    object.addSymbol(std::move(mainSymbol));
    object.addSymbol(std::move(dataSymbol));

    // Verificar símbolos
    ASSERT_EQ(object.symbols.size(), 2u);
    EXPECT_EQ(object.symbols[0].name, "_main");
    EXPECT_EQ(object.symbols[0].storageClass, IMAGE_SYM_CLASS_EXTERNAL);
    EXPECT_EQ(object.symbols[1].name, "_global_var");
    EXPECT_EQ(object.symbols[1].sectionNumber, 2);

    // Verificar header
    EXPECT_EQ(object.header.NumberOfSymbols, 2u);
}

TEST_F(COFFWriterTest, COFFDumperBasicTest) {
    COFFObject object = createBasicCOFFObject();

    // Agregar algunos datos para hacer el dump más interesante
    object.sections[0].data = {0x48, 0x89, 0xC8, 0xC3}; // mov rax, rcx; ret

    fs::path testFile = getTempFile("dump_test.obj");

    // Escribir y luego hacer dump
    ASSERT_TRUE(writeCOFFObject(object, testFile.string()));

    // Verificar que podemos hacer dump sin errores
    std::stringstream output;
    COFFDumper dumper;
    EXPECT_TRUE(dumper.dumpFile(testFile.string(), output));

    // Verificar que el output contiene información básica
    std::string dumpOutput = output.str();
    EXPECT_NE(dumpOutput.find("COFF File Header"), std::string::npos);
    EXPECT_NE(dumpOutput.find("AMD64"), std::string::npos);
    EXPECT_NE(dumpOutput.find(".text"), std::string::npos);
    EXPECT_NE(dumpOutput.find(".data"), std::string::npos);
    EXPECT_NE(dumpOutput.find(".rdata"), std::string::npos);
}

TEST_F(COFFWriterTest, ValidateCOFFStructure) {
    COFFObject object = createBasicCOFFObject();

    // Verificar estructura básica
    EXPECT_EQ(object.header.Machine, IMAGE_FILE_MACHINE_AMD64);
    EXPECT_EQ(object.header.NumberOfSections, 3u);
    EXPECT_EQ(object.header.SizeOfOptionalHeader, 0u);

    // Verificar que las características son válidas
    EXPECT_TRUE(object.header.Characteristics & IMAGE_FILE_RELOCS_STRIPPED);
    EXPECT_TRUE(object.header.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE);
    EXPECT_TRUE(object.header.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE);

    // Verificar secciones
    for (const auto& section : object.sections) {
        EXPECT_FALSE(section.name.empty());
        EXPECT_TRUE(section.characteristics != 0);
    }
}

TEST_F(COFFWriterTest, COFFSectionProperties) {
    COFFSection section(".test", IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_READ);

    // Verificar propiedades iniciales
    EXPECT_EQ(section.name, ".test");
    EXPECT_TRUE(section.data.empty());
    EXPECT_TRUE(section.relocations.empty());
    EXPECT_EQ(section.characteristics, IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_READ);

    // Agregar datos
    section.data = {0x01, 0x02, 0x03, 0x04};
    EXPECT_EQ(section.size(), 4u);
    EXPECT_FALSE(section.isEmpty());

    // Agregar relocation
    IMAGE_RELOCATION reloc = {};
    reloc.VirtualAddress = 0x1000;
    reloc.SymbolTableIndex = 1;
    reloc.Type = IMAGE_REL_AMD64_REL32;

    section.relocations.push_back(reloc);
    EXPECT_EQ(section.relocations.size(), 1u);
}

TEST_F(COFFWriterTest, COFFSymbolProperties) {
    COFFSymbol symbol("test_symbol", IMAGE_SYM_CLASS_STATIC);

    // Verificar propiedades iniciales
    EXPECT_EQ(symbol.name, "test_symbol");
    EXPECT_EQ(symbol.value, 0u);
    EXPECT_EQ(symbol.sectionNumber, 0);
    EXPECT_EQ(symbol.storageClass, IMAGE_SYM_CLASS_STATIC);

    // Modificar propiedades
    symbol.value = 0x1000;
    symbol.sectionNumber = 1;
    symbol.type = 0x20; // Function

    EXPECT_EQ(symbol.value, 0x1000u);
    EXPECT_EQ(symbol.sectionNumber, 1);
    EXPECT_EQ(symbol.type, 0x20u);
}

// ========================================================================
// Tests de relocations
// ========================================================================

TEST_F(COFFWriterTest, AMD64RelocationTypes) {
    // Verificar que las constantes de relocation están definidas correctamente
    EXPECT_EQ(IMAGE_REL_AMD64_ABSOLUTE, 0x0000);
    EXPECT_EQ(IMAGE_REL_AMD64_ADDR64, 0x0001);
    EXPECT_EQ(IMAGE_REL_AMD64_ADDR32, 0x0002);
    EXPECT_EQ(IMAGE_REL_AMD64_REL32, 0x0004);
    EXPECT_EQ(IMAGE_REL_AMD64_REL32_1, 0x0005);
    EXPECT_EQ(IMAGE_REL_AMD64_REL32_2, 0x0006);
    EXPECT_EQ(IMAGE_REL_AMD64_REL32_3, 0x0007);
    EXPECT_EQ(IMAGE_REL_AMD64_REL32_4, 0x0008);
    EXPECT_EQ(IMAGE_REL_AMD64_REL32_5, 0x0009);
}

// ========================================================================
// Tests de integración completa
// ========================================================================

TEST_F(COFFWriterTest, FullCOFFObjectCreation) {
    // Crear un objeto COFF completo con datos, símbolos y relocations
    COFFObject object = createBasicCOFFObject();

    // Agregar código máquina simple
    object.sections[0].data = {
        0x48, 0x89, 0xC8,           // mov rax, rcx
        0x48, 0x83, 0xC0, 0x01,     // add rax, 1
        0xC3                        // ret
    };

    // Agregar datos
    object.sections[1].data = {0x41, 0x42, 0x43, 0x00}; // "ABC"

    // Agregar símbolo para la función
    COFFSymbol funcSymbol("_test_function", IMAGE_SYM_CLASS_EXTERNAL);
    funcSymbol.sectionNumber = 1; // .text
    funcSymbol.type = 0x20; // Function
    object.addSymbol(std::move(funcSymbol));

    // Agregar símbolo para los datos
    COFFSymbol dataSymbol("_test_data", IMAGE_SYM_CLASS_EXTERNAL);
    dataSymbol.sectionNumber = 2; // .data
    dataSymbol.type = 0x00; // Data
    object.addSymbol(std::move(dataSymbol));

    fs::path testFile = getTempFile("full_test.obj");

    // Escribir objeto completo
    EXPECT_TRUE(writeCOFFObject(object, testFile.string()));

    // Verificar archivo
    EXPECT_TRUE(fs::exists(testFile));
    auto fileSize = fs::file_size(testFile);

    // Calcular tamaño esperado
    size_t expectedSize = sizeof(IMAGE_FILE_HEADER) +
                         3 * sizeof(IMAGE_SECTION_HEADER) +     // 3 secciones
                         7 +                                     // código (7 bytes)
                         4 +                                     // datos (4 bytes)
                         0 +                                     // .rdata vacía
                         2 * sizeof(IMAGE_SYMBOL);              // 2 símbolos

    EXPECT_EQ(fileSize, expectedSize);

    // Verificar que podemos hacer dump
    std::stringstream output;
    COFFDumper dumper;
    EXPECT_TRUE(dumper.dumpFile(testFile.string(), output));
}
