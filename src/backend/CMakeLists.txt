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

# Combinar todos los sources y headers
set(BACKEND_SOURCES
    ${ABI_SOURCES}
    ${FRAME_SOURCES}
)

set(BACKEND_HEADERS
    ${ABI_HEADERS}
    ${FRAME_HEADERS}
)

# Crear librería del back-end
add_library(cpp20-compiler-backend STATIC
    ${BACKEND_SOURCES}
    ${BACKEND_HEADERS}
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
