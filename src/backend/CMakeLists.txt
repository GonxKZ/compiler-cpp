# =============================================================================
# Back-end del Compilador C++20
# =============================================================================

# ABI Contract
set(ABI_SOURCES
    abi/ABIContract.cpp
)

set(ABI_HEADERS
    abi/ABIContract.h
)

# Frame Builder
set(FRAME_SOURCES
    frame/FrameBuilder.cpp
    frame/FrameLayout.cpp
)

set(FRAME_HEADERS
    frame/FrameBuilder.h
    frame/FrameLayout.h
)

# COFF Support
set(COFF_SOURCES
    coff/COFFWriter.cpp
    coff/COFFDumper.cpp
)

set(COFF_HEADERS
    coff/COFFTypes.h
    coff/COFFWriter.h
    coff/COFFDumper.h
)

# Unwind Support
set(UNWIND_SOURCES
    unwind/UnwindCodeGenerator.cpp
    unwind/UnwindEmitter.cpp
    unwind/ExceptionMapper.cpp
)

set(UNWIND_HEADERS
    unwind/UnwindTypes.h
    unwind/UnwindEmitter.h
    unwind/ExceptionMapper.h
)

# Combinar todos los sources y headers
set(BACKEND_SOURCES
    ${ABI_SOURCES}
    ${FRAME_SOURCES}
    ${COFF_SOURCES}
    ${UNWIND_SOURCES}
)

set(BACKEND_HEADERS
    ${ABI_HEADERS}
    ${FRAME_HEADERS}
    ${COFF_HEADERS}
    ${UNWIND_HEADERS}
)

# Crear librería del back-end
add_library(cpp20-compiler-backend STATIC
    ${BACKEND_SOURCES}
)

# Dependencias
target_link_libraries(cpp20-compiler-backend
    PRIVATE
        cpp20-compiler::common
        cpp20-compiler::types
)

# Configuración
target_include_directories(cpp20-compiler-backend
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Alias
add_library(cpp20-compiler::backend ALIAS cpp20-compiler-backend)

# LLVM Integration (opcional)
if(CPP20_COMPILER_USE_LLVM)
    target_link_libraries(cpp20-compiler-backend
        PRIVATE
            ${llvm_libs}
    )
endif()
