#pragma once

#include <string>
#include <cstdint>
#include <cstddef>

namespace cpp20::compiler::common::utils {

/**
 * @brief Utilidades para hashing
 */

// Standard library hashing
size_t hashString(const std::string& str);
size_t hashPointer(const void* ptr);

// Hash combining
size_t hashCombine(size_t seed, size_t value);

// FNV-1a hashing (fast and good distribution)
uint32_t fnv1a32(const std::string& str);
uint64_t fnv1a64(const std::string& str);

} // namespace cpp20::compiler::common::utils
