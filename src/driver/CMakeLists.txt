# =============================================================================
# Driver del Compilador C++20
# =============================================================================

set(DRIVER_SOURCES
    main.cpp
    CompilerDriver.cpp
    CommandLineParser.cpp
)

set(DRIVER_HEADERS
    CompilerDriver.h
    CommandLineParser.h
)

# Ejecutable principal
add_executable(cpp20-compiler
    ${DRIVER_SOURCES}
)

# Dependencias
target_link_libraries(cpp20-compiler
    PRIVATE
        cpp20-compiler::common
        cpp20-compiler::frontend
        cpp20-compiler::types
        cpp20-compiler::ast
        cpp20-compiler::symbols
)

if(CPP20_COMPILER_USE_LLVM)
    target_link_libraries(cpp20-compiler
        PRIVATE
            cpp20-compiler::backend
            cpp20-compiler::ir
            ${llvm_libs}
    )
endif()

if(CPP20_COMPILER_ENABLE_MODULES)
    target_link_libraries(cpp20-compiler
        PRIVATE
            cpp20-compiler::modules
    )
endif()

if(CPP20_COMPILER_ENABLE_COROUTINES)
    target_link_libraries(cpp20-compiler
        PRIVATE
            cpp20-compiler::coroutines
    )
endif()

# Configuración de compilación
target_include_directories(cpp20-compiler
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

if(MSVC)
    target_compile_options(cpp20-compiler PRIVATE /W4)
else()
    target_compile_options(cpp20-compiler PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Propiedades del ejecutable
set_target_properties(cpp20-compiler PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_NAME "cpp20-compiler"
)

# Instalación
install(TARGETS cpp20-compiler
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
