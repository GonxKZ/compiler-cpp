# =============================================================================
# Tests del Compilador C++20
# =============================================================================

# Tests unitarios
set(UNIT_TESTS
    unit/test_abi_contract.cpp
)

# Tests de integración
set(INTEGRATION_TESTS
    # integration/test_capa0_basic.cpp
)

# Tests de conformidad
set(CONFORMANCE_TESTS
    # conformance/test_c_standard.cpp
)

# Combinar todos los tests
set(ALL_TESTS
    ${UNIT_TESTS}
    ${INTEGRATION_TESTS}
    ${CONFORMANCE_TESTS}
)

# Crear ejecutable de tests
add_executable(cpp20-compiler-tests
    ${ALL_TESTS}
)

# Dependencias de tests
target_link_libraries(cpp20-compiler-tests
    PRIVATE
        cpp20-compiler::common
        cpp20-compiler::backend
        cpp20-compiler::types
        GTest::gtest_main
)

# Configuración
target_include_directories(cpp20-compiler-tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Propiedades de compilación
if(MSVC)
    target_compile_options(cpp20-compiler-tests PRIVATE /W4)
else()
    target_compile_options(cpp20-compiler-tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Configurar Google Test
include(GoogleTest)
gtest_discover_tests(cpp20-compiler-tests)
