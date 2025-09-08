/**
 * @file HashUtils.cpp
 * @brief Utilidades para hashing
 */

#include <compiler/common/utils/HashUtils.h>
#include <functional>

namespace cpp20::compiler::common::utils {

// ========================================================================
// Funciones de hashing
// ========================================================================

size_t hashString(const std::string& str) {
    return std::hash<std::string>{}(str);
}

size_t hashCombine(size_t seed, size_t value) {
    // Boost hash_combine algorithm
    return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

size_t hashPointer(const void* ptr) {
    return std::hash<const void*>{}(ptr);
}

uint32_t fnv1a32(const std::string& str) {
    uint32_t hash = 2166136261u;
    for (char c : str) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619u;
    }
    return hash;
}

uint64_t fnv1a64(const std::string& str) {
    uint64_t hash = 14695981039346656037ull;
    for (char c : str) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 1099511628211ull;
    }
    return hash;
}

} // namespace cpp20::compiler::common::utils
