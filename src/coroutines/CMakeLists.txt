# CMakeLists.txt for C++20 Coroutines System

# Create coroutines library
add_library(cpp20-compiler-coroutines STATIC
    CoroutineSystem.cpp
)

# Set include directories
target_include_directories(cpp20-compiler-coroutines
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

# Link dependencies
target_link_libraries(cpp20-compiler-coroutines
    PUBLIC
        cpp20-compiler-common
)

# Set C++ standard and coroutine flags
target_compile_features(cpp20-compiler-coroutines PUBLIC cxx_std_20)

# Set compile options
target_compile_options(cpp20-compiler-coroutines PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /await:strict>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -fcoroutines>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -fcoroutines-ts>
)

# Add coroutine-specific definitions
target_compile_definitions(cpp20-compiler-coroutines
    PUBLIC
        CPP20_COROUTINES_ENABLED=1
)

# Alias para uso consistente
add_library(cpp20-compiler::coroutines ALIAS cpp20-compiler-coroutines)