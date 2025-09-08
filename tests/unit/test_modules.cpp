/**
 * @file test_modules.cpp
 * @brief Tests unitarios para el sistema de módulos C++20
 */

#include <gtest/gtest.h>
#include <memory>
#include <filesystem>
#include <compiler/modules/ModuleSystem.h>

using namespace cpp20::compiler::modules;

// Test para BinaryModuleInterface
TEST(BinaryModuleInterfaceTest, BasicCreation) {
    BinaryModuleInterface bmi("test_module");

    EXPECT_EQ(bmi.getModuleName(), "test_module");
    EXPECT_FALSE(bmi.isValid()); // Empty BMI is not valid
}

TEST(BinaryModuleInterfaceTest, AddExportedEntity) {
    BinaryModuleInterface bmi("test_module");

    ExportedEntity entity("MyClass", "MyModule::MyClass", ExportType::Type);
    bmi.addExportedEntity(entity);

    const auto& entities = bmi.getExportedEntities();
    ASSERT_EQ(entities.size(), 1);
    EXPECT_EQ(entities[0].name, "MyClass");
    EXPECT_EQ(entities[0].qualifiedName, "MyModule::MyClass");
    EXPECT_EQ(entities[0].type, ExportType::Type);
}

TEST(BinaryModuleInterfaceTest, AddDependency) {
    BinaryModuleInterface bmi("test_module");

    ModuleDependency dep("other_module", true);
    bmi.addDependency(dep);

    const auto& deps = bmi.getDependencies();
    ASSERT_EQ(deps.size(), 1);
    EXPECT_EQ(deps[0].moduleName, "other_module");
    EXPECT_TRUE(deps[0].isInterface);
}

TEST(BinaryModuleInterfaceTest, SerializeDeserialize) {
    BinaryModuleInterface original("test_module");

    ExportedEntity entity("MyFunction", "TestModule::MyFunction", ExportType::Function);
    original.addExportedEntity(entity);

    ModuleDependency dep("std", true);
    original.addDependency(dep);

    CompilationOptionsHash hash;
    hash.preprocessorHash = 12345;
    hash.compilerFlagsHash = 67890;
    hash.systemIncludesHash = 11111;
    original.setCompilationOptionsHash(hash);

    // Serialize
    auto data = original.serialize();
    ASSERT_FALSE(data.empty());

    // Deserialize
    auto restored = BinaryModuleInterface::deserialize(data);
    ASSERT_TRUE(restored != nullptr);
    EXPECT_EQ(restored->getModuleName(), "test_module");

    const auto& entities = restored->getExportedEntities();
    ASSERT_EQ(entities.size(), 1);
    EXPECT_EQ(entities[0].name, "MyFunction");

    const auto& deps = restored->getDependencies();
    ASSERT_EQ(deps.size(), 1);
    EXPECT_EQ(deps[0].moduleName, "std");
}

// Test para ModuleInterface
TEST(ModuleInterfaceTest, BasicCreation) {
    std::filesystem::path sourcePath("/path/to/module.ixx");
    ModuleInterface module("my_module", sourcePath);

    EXPECT_EQ(module.getModuleName(), "my_module");
    EXPECT_EQ(module.getSourcePath(), sourcePath);
    EXPECT_FALSE(module.isReady());
}

TEST(ModuleInterfaceTest, AddPartition) {
    std::filesystem::path sourcePath("/path/to/module.ixx");
    ModuleInterface module("my_module", sourcePath);

    module.addPartition("partition1");
    module.addPartition("partition2");

    const auto& partitions = module.getPartitions();
    ASSERT_EQ(partitions.size(), 2);
    EXPECT_EQ(partitions[0], "partition1");
    EXPECT_EQ(partitions[1], "partition2");
}

TEST(ModuleInterfaceTest, SetBMI) {
    std::filesystem::path sourcePath("/path/to/module.ixx");
    ModuleInterface module("my_module", sourcePath);

    auto bmi = std::make_unique<BinaryModuleInterface>("my_module");
    module.setBMI(std::move(bmi));

    EXPECT_TRUE(module.isReady());
    EXPECT_TRUE(module.getBMI() != nullptr);
}

// Test para ModuleDependencyScanner
TEST(ModuleDependencyScannerTest, BasicCreation) {
    ModuleDependencyScanner scanner;
    // Scanner should be created without issues
    SUCCEED();
}

TEST(ModuleDependencyScannerTest, ExtractModuleName) {
    ModuleDependencyScanner scanner;

    EXPECT_EQ(scanner.extractModuleName("export module math;"), "math");
    EXPECT_EQ(scanner.extractModuleName("module utils;"), "utils");
    EXPECT_EQ(scanner.extractModuleName("export module math.submodule;"), "math.submodule");
}

TEST(ModuleDependencyScannerTest, ExtractImportName) {
    ModuleDependencyScanner scanner;

    EXPECT_EQ(scanner.extractImportName("import std;"), "std");
    EXPECT_EQ(scanner.extractImportName("import <iostream>;"), "<iostream>");
    EXPECT_EQ(scanner.extractImportName("import math.utils;"), "math.utils");
}

// Test para ModuleCache
TEST(ModuleCacheTest, BasicCreation) {
    std::filesystem::path cacheDir("./test_cache");
    ModuleCache cache(cacheDir);

    auto stats = cache.getStats();
    EXPECT_EQ(stats.totalEntries, 0);
    EXPECT_EQ(stats.hits, 0);
    EXPECT_EQ(stats.misses, 0);
}

TEST(ModuleCacheTest, StoreAndRetrieve) {
    std::filesystem::path cacheDir("./test_cache_store");
    ModuleCache cache(cacheDir);

    BinaryModuleInterface bmi("test_module");
    ExportedEntity entity("TestClass", "TestModule::TestClass", ExportType::Type);
    bmi.addExportedEntity(entity);

    // Store
    bool stored = cache.store("test_module", bmi);
    EXPECT_TRUE(stored);

    // Retrieve
    auto retrieved = cache.retrieve("test_module");
    ASSERT_TRUE(retrieved != nullptr);
    EXPECT_EQ(retrieved->getModuleName(), "test_module");

    const auto& entities = retrieved->getExportedEntities();
    ASSERT_EQ(entities.size(), 1);
    EXPECT_EQ(entities[0].name, "TestClass");

    // Check stats
    auto stats = cache.getStats();
    EXPECT_EQ(stats.totalEntries, 1);
    EXPECT_EQ(stats.hits, 1);
}

// Test para ModuleSystem
TEST(ModuleSystemTest, BasicCreation) {
    std::filesystem::path cacheDir("./test_module_cache");
    ModuleSystem system(cacheDir);

    auto stats = system.getStats();
    EXPECT_EQ(stats.totalModules, 0);
    EXPECT_EQ(stats.interfacesCompiled, 0);
}

TEST(ModuleSystemTest, Initialize) {
    std::filesystem::path cacheDir("./test_init_cache");
    ModuleSystem system(cacheDir);

    bool initialized = system.initialize();
    EXPECT_TRUE(initialized);

    auto stats = system.getStats();
    EXPECT_EQ(stats.totalModules, 0);
}

TEST(ModuleSystemTest, ModuleExists) {
    std::filesystem::path cacheDir("./test_exists_cache");
    ModuleSystem system(cacheDir);

    EXPECT_FALSE(system.moduleExists("nonexistent"));
    EXPECT_FALSE(system.moduleExists("math"));
}

// Test para tipos de datos
TEST(ModuleTypesTest, ExportTypeValues) {
    EXPECT_EQ(static_cast<int>(ExportType::Type), 0);
    EXPECT_EQ(static_cast<int>(ExportType::Function), 1);
    EXPECT_EQ(static_cast<int>(ExportType::Variable), 2);
    EXPECT_EQ(static_cast<int>(ExportType::Template), 3);
    EXPECT_EQ(static_cast<int>(ExportType::Namespace), 4);
    EXPECT_EQ(static_cast<int>(ExportType::Concept), 5);
}

TEST(ModuleTypesTest, ModuleStateValues) {
    EXPECT_EQ(static_cast<int>(ModuleState::Discovered), 0);
    EXPECT_EQ(static_cast<int>(ModuleState::Scanning), 1);
    EXPECT_EQ(static_cast<int>(ModuleState::InterfacesReady), 2);
    EXPECT_EQ(static_cast<int>(ModuleState::Compiling), 3);
    EXPECT_EQ(static_cast<int>(ModuleState::Ready), 4);
    EXPECT_EQ(static_cast<int>(ModuleState::Error), 5);
}

TEST(ModuleTypesTest, ModuleTypeValues) {
    EXPECT_EQ(static_cast<int>(ModuleType::Interface), 0);
    EXPECT_EQ(static_cast<int>(ModuleType::Implementation), 1);
    EXPECT_EQ(static_cast<int>(ModuleType::Partition), 2);
    EXPECT_EQ(static_cast<int>(ModuleType::Global), 3);
}

// Test para CompilationOptionsHash
TEST(CompilationOptionsHashTest, CombinedHash) {
    CompilationOptionsHash hash1;
    hash1.preprocessorHash = 100;
    hash1.compilerFlagsHash = 200;
    hash1.systemIncludesHash = 300;

    CompilationOptionsHash hash2;
    hash2.preprocessorHash = 100;
    hash2.compilerFlagsHash = 200;
    hash2.systemIncludesHash = 300;

    EXPECT_EQ(hash1.combined(), hash2.combined());

    CompilationOptionsHash hash3;
    hash3.preprocessorHash = 999;
    hash3.compilerFlagsHash = 200;
    hash3.systemIncludesHash = 300;

    EXPECT_NE(hash1.combined(), hash3.combined());
}

// Test para ExportedEntity
TEST(ExportedEntityTest, BasicConstruction) {
    ExportedEntity entity("test", "qualified::test", ExportType::Function, "file.cpp:10");

    EXPECT_EQ(entity.name, "test");
    EXPECT_EQ(entity.qualifiedName, "qualified::test");
    EXPECT_EQ(entity.type, ExportType::Function);
    EXPECT_EQ(entity.sourceLocation, "file.cpp:10");
    EXPECT_FALSE(entity.isInline);
    EXPECT_FALSE(entity.isConstexpr);
}

TEST(ExportedEntityTest, Flags) {
    ExportedEntity entity("test", "qualified::test", ExportType::Function);

    entity.isInline = true;
    entity.isConstexpr = true;

    EXPECT_TRUE(entity.isInline);
    EXPECT_TRUE(entity.isConstexpr);
}

// Test para ModuleDependency
TEST(ModuleDependencyTest, BasicConstruction) {
    ModuleDependency dep("module_name", true, "file.cpp:5");

    EXPECT_EQ(dep.moduleName, "module_name");
    EXPECT_TRUE(dep.isInterface);
    EXPECT_EQ(dep.sourceLocation, "file.cpp:5");
}

TEST(ModuleDependencyTest, HeaderUnit) {
    ModuleDependency dep("iostream", false, "file.cpp:1");

    EXPECT_EQ(dep.moduleName, "iostream");
    EXPECT_FALSE(dep.isInterface);
}

// Test para integración completa
TEST(IntegrationTest, FullModuleWorkflow) {
    std::filesystem::path cacheDir("./integration_test_cache");
    ModuleSystem system(cacheDir);

    // Initialize system
    ASSERT_TRUE(system.initialize());

    // Create a mock module interface
    std::filesystem::path sourcePath("./test_module.ixx");
    auto module = std::make_unique<ModuleInterface>("test_math", sourcePath);

    // Add some exported entities
    ExportedEntity funcEntity("add", "test_math::add", ExportType::Function);
    funcEntity.isConstexpr = true;

    ExportedEntity classEntity("Calculator", "test_math::Calculator", ExportType::Type);

    auto bmi = std::make_unique<BinaryModuleInterface>("test_math");
    bmi->addExportedEntity(funcEntity);
    bmi->addExportedEntity(classEntity);

    module->setBMI(std::move(bmi));

    // Verify module is ready
    EXPECT_TRUE(module->isReady());
    EXPECT_EQ(module->getModuleName(), "test_math");

    const auto* retrievedBMI = module->getBMI();
    ASSERT_TRUE(retrievedBMI != nullptr);

    const auto& entities = retrievedBMI->getExportedEntities();
    ASSERT_EQ(entities.size(), 2);
    EXPECT_EQ(entities[0].name, "add");
    EXPECT_EQ(entities[1].name, "Calculator");
}

TEST(IntegrationTest, CacheWorkflow) {
    std::filesystem::path cacheDir("./workflow_cache");
    ModuleCache cache(cacheDir);

    // Create and store BMI
    BinaryModuleInterface bmi("workflow_test");
    ExportedEntity entity("test_func", "workflow_test::test_func", ExportType::Function);
    bmi.addExportedEntity(entity);

    bool stored = cache.store("workflow_test", bmi);
    ASSERT_TRUE(stored);

    // Retrieve and verify
    auto retrieved = cache.retrieve("workflow_test");
    ASSERT_TRUE(retrieved != nullptr);
    EXPECT_EQ(retrieved->getModuleName(), "workflow_test");

    const auto& entities = retrieved->getExportedEntities();
    ASSERT_EQ(entities.size(), 1);
    EXPECT_EQ(entities[0].name, "test_func");

    // Check cache stats
    auto stats = cache.getStats();
    EXPECT_EQ(stats.totalEntries, 1);
    EXPECT_EQ(stats.hits, 1);
    EXPECT_EQ(stats.misses, 0);
}

// Test para manejo de errores
TEST(ErrorHandlingTest, InvalidBMI) {
    std::vector<uint8_t> invalidData = {1, 2, 3}; // Too short
    auto bmi = BinaryModuleInterface::deserialize(invalidData);
    EXPECT_TRUE(bmi == nullptr);
}

TEST(ErrorHandlingTest, EmptyModuleName) {
    BinaryModuleInterface bmi("");
    EXPECT_FALSE(bmi.isValid());
}

TEST(ErrorHandlingTest, CacheInvalidation) {
    std::filesystem::path cacheDir("./invalidate_cache");
    ModuleCache cache(cacheDir);

    // Store something
    BinaryModuleInterface bmi("invalidate_test");
    cache.store("invalidate_test", bmi);

    // Invalidate
    cache.invalidate("invalidate_test");

    // Try to retrieve (should miss)
    auto retrieved = cache.retrieve("invalidate_test");
    EXPECT_TRUE(retrieved == nullptr);
}

// Test para rendimiento básico
TEST(PerformanceTest, BasicOperations) {
    std::filesystem::path cacheDir("./perf_cache");
    ModuleCache cache(cacheDir);

    // Test rapid store/retrieve operations
    for (int i = 0; i < 10; ++i) {
        std::string moduleName = "perf_module_" + std::to_string(i);
        BinaryModuleInterface bmi(moduleName);

        // Store
        bool stored = cache.store(moduleName, bmi);
        ASSERT_TRUE(stored);

        // Retrieve
        auto retrieved = cache.retrieve(moduleName);
        ASSERT_TRUE(retrieved != nullptr);
        EXPECT_EQ(retrieved->getModuleName(), moduleName);
    }

    auto stats = cache.getStats();
    EXPECT_EQ(stats.totalEntries, 10);
    EXPECT_EQ(stats.hits, 10);
}
