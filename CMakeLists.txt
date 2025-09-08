cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# =============================================================================
# Proyecto: Compilador C++20 para Windows x64
# =============================================================================

project(cpp20-compiler
    VERSION 0.1.0
    DESCRIPTION "Compilador C++20 completo desde cero para Windows x64"
    LANGUAGES CXX C
)

# =============================================================================
# Configuración Global
# =============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configuración específica para Windows x64
if(WIN32)
    set(CMAKE_SYSTEM_PROCESSOR "AMD64")
    set(CMAKE_GENERATOR_PLATFORM "x64")

    # Configuración para MSVC
    if(MSVC)
        # Opciones de compilación mejoradas para manejo de errores
        add_compile_options(
            /W4                    # Nivel máximo de warnings
            /permissive-           # Cumplimiento estricto del estándar
            /Zc:__cplusplus        # Reportar __cplusplus correctamente
            /utf-8                 # Soporte UTF-8
            /diagnostics:caret     # Ubicaciones precisas de errores
            /Zc:wchar_t            # wchar_t como tipo nativo
            /Zc:inline             # Remover funciones inline no utilizadas
            /Zc:throwingNew        # Verificar excepciones en new
        )

        # Definiciones para compatibilidad
        add_compile_definitions(
            _CRT_SECURE_NO_WARNINGS
            NOMINMAX
            _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
            _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
        )

        # Configuración de linker mejorada
        add_link_options(
            /DEBUG:FULL           # Información de debug completa
            /INCREMENTAL:NO       # Linker no incremental para builds limpios
        )
    endif()
endif()

# Configuración de build
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# =============================================================================
# Opciones del Proyecto
# =============================================================================

option(CPP20_COMPILER_BUILD_TESTS "Construir tests del compilador" ON)
option(CPP20_COMPILER_BUILD_EXAMPLES "Construir ejemplos" ON)
option(CPP20_COMPILER_ENABLE_LTO "Habilitar Link Time Optimization" OFF)
option(CPP20_COMPILER_ENABLE_PCH "Habilitar Precompiled Headers" ON)
option(CPP20_COMPILER_USE_LLVM "Usar LLVM como back-end" ON)
option(CPP20_COMPILER_ENABLE_COROUTINES "Habilitar soporte para corrutinas C++20" ON)
option(CPP20_COMPILER_ENABLE_MODULES "Habilitar soporte para módulos C++20" ON)

# =============================================================================
# Dependencias Externas
# =============================================================================

# LLVM Configuration
if(CPP20_COMPILER_USE_LLVM)
    find_package(LLVM 15.0 REQUIRED CONFIG)
    find_package(LLD REQUIRED CONFIG)

    message(STATUS "Encontrado LLVM ${LLVM_PACKAGE_VERSION}")
    message(STATUS "Usando LLVM en: ${LLVM_DIR}")

    # Componentes LLVM requeridos
    llvm_map_components_to_libnames(llvm_libs
        Core
        Support
        IRReader
        BitWriter
        Analysis
        TransformUtils
        ScalarOpts
        InstCombine
        Target
        X86CodeGen
        X86AsmPrinter
        X86TargetMachine
        MC
        MCParser
        MCDisassembler
        Object
        Coroutines
        IPO
    )

    # Incluir directorios LLVM
    include_directories(${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})
endif()

# Google Test para testing
if(CPP20_COMPILER_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# =============================================================================
# Configuración de Directorios
# =============================================================================

# Directorios de include
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

# =============================================================================
# Librerías del Compilador
# =============================================================================

# Librería común
add_subdirectory(src/common)

# Sistema de tipos
add_subdirectory(src/types)

# Tabla de símbolos
add_subdirectory(src/symbols)

# AST
add_subdirectory(src/ast)

# Front-end
add_subdirectory(src/frontend)

# Back-end
add_subdirectory(src/backend)

# IR Intermedio
add_subdirectory(src/ir)

# Templates
add_subdirectory(src/templates)

# Constexpr
add_subdirectory(src/constexpr)

# Módulos C++20
option(CPP20_COMPILER_ENABLE_MODULES "Enable C++20 modules support" ON)
if(CPP20_COMPILER_ENABLE_MODULES)
    add_subdirectory(src/modules)
endif()

# Corroutinas C++20
option(CPP20_COMPILER_ENABLE_COROUTINES "Enable C++20 coroutines support" ON)
if(CPP20_COMPILER_ENABLE_COROUTINES)
    add_subdirectory(src/coroutines)
endif()

# =============================================================================
# Ejecutable Principal
# =============================================================================

add_subdirectory(src/driver)

# =============================================================================
# Testing
# =============================================================================

if(CPP20_COMPILER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# =============================================================================
# Ejemplos
# =============================================================================

if(CPP20_COMPILER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# =============================================================================
# Utilidades de Desarrollo
# =============================================================================

add_subdirectory(tools)

# =============================================================================
# Configuración de Paquetes
# =============================================================================

# Generar archivo de configuración
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpp20-compiler-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cpp20-compiler-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpp20-compiler
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/cpp20-compiler-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# =============================================================================
# Instalación
# =============================================================================

install(TARGETS cpp20-compiler
    EXPORT cpp20-compiler-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT cpp20-compiler-targets
    FILE cpp20-compiler-targets.cmake
    NAMESPACE cpp20-compiler::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpp20-compiler
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/cpp20-compiler-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cpp20-compiler-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpp20-compiler
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# =============================================================================
# Información del Proyecto
# =============================================================================

message(STATUS "=======================================")
message(STATUS "Compilador C++20 - Configuración")
message(STATUS "=======================================")
message(STATUS "Versión: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Plataforma: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Tests: ${CPP20_COMPILER_BUILD_TESTS}")
message(STATUS "Examples: ${CPP20_COMPILER_BUILD_EXAMPLES}")
message(STATUS "LLVM Support: ${CPP20_COMPILER_USE_LLVM}")
message(STATUS "Coroutines: ${CPP20_COMPILER_ENABLE_COROUTINES}")
message(STATUS "Modules: ${CPP20_COMPILER_ENABLE_MODULES}")
if(CPP20_COMPILER_USE_LLVM)
    message(STATUS "LLVM Version: ${LLVM_PACKAGE_VERSION}")
endif()
message(STATUS "=======================================")
