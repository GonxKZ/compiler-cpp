# =============================================================================
# Librería Común del Compilador C++20
# =============================================================================

set(COMMON_SOURCES
    diagnostics/DiagnosticEngine.cpp
    diagnostics/Diagnostic.cpp
    diagnostics/SourceLocation.cpp
    diagnostics/SourceManager.cpp
    utils/StringUtils.cpp
    utils/FileUtils.cpp
    utils/MemoryPool.cpp
    utils/HashUtils.cpp
)

set(COMMON_HEADERS
    diagnostics/DiagnosticEngine.h
    diagnostics/Diagnostic.h
    diagnostics/SourceLocation.h
    diagnostics/SourceManager.h
    utils/StringUtils.h
    utils/FileUtils.h
    utils/MemoryPool.h
    utils/HashUtils.h
)

# Crear librería común
add_library(cpp20-compiler-common STATIC
    ${COMMON_SOURCES}
)

# Configuración de la librería
target_include_directories(cpp20-compiler-common
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Propiedades de compilación
if(MSVC)
    target_compile_options(cpp20-compiler-common PRIVATE /W4)
else()
    target_compile_options(cpp20-compiler-common PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Dependencias
if(CPP20_COMPILER_USE_LLVM)
    target_link_libraries(cpp20-compiler-common
        PUBLIC
            LLVMCore
            LLVMSupport
    )
endif()

# Alias para uso consistente
add_library(cpp20-compiler::common ALIAS cpp20-compiler-common)

# Configuración de instalación
install(TARGETS cpp20-compiler-common
    EXPORT cpp20-compiler-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${COMMON_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/compiler/common
)
