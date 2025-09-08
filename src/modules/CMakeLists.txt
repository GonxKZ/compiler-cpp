# CMakeLists.txt for C++20 Modules System

# Create modules library
add_library(cpp20-compiler-modules STATIC
    ModuleSystem.cpp
)

# Set include directories
target_include_directories(cpp20-compiler-modules
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

# Link dependencies
target_link_libraries(cpp20-compiler-modules
    PUBLIC
        cpp20-compiler-common
)

# Set C++ standard
target_compile_features(cpp20-compiler-modules PUBLIC cxx_std_20)

# Set compile options
target_compile_options(cpp20-compiler-modules PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra>
)

# Alias para uso consistente
add_library(cpp20-compiler::modules ALIAS cpp20-compiler-modules)