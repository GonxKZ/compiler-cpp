/**
 * @file MemoryPool.cpp
 * @brief Pool de memoria para allocations eficientes
 */

#include <compiler/common/utils/MemoryPool.h>
#include <cstdlib>
#include <cstring>
#include <new>

namespace cpp20::compiler::common::utils {

// ========================================================================
// MemoryPool implementation
// ========================================================================

MemoryPool::MemoryPool(size_t blockSize, size_t initialBlocks)
    : blockSize_(blockSize), initialBlocks_(initialBlocks) {
    allocateNewBlock();
}

MemoryPool::~MemoryPool() {
    for (auto* block : blocks_) {
        std::free(block);
    }
}

void* MemoryPool::allocate(size_t size) {
    if (size == 0) return nullptr;

    // For simplicity, just use malloc for now
    // TODO: Implement proper pooling for Capa 4+
    void* ptr = std::malloc(size);
    if (!ptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

void MemoryPool::deallocate(void* ptr, size_t size) {
    // For simplicity, just use free for now
    // TODO: Implement proper pooling for Capa 4+
    std::free(ptr);
}

void MemoryPool::reset() {
    // Free all blocks except the first one
    for (size_t i = 1; i < blocks_.size(); ++i) {
        std::free(blocks_[i]);
    }

    if (!blocks_.empty()) {
        blocks_.resize(1);
        currentBlock_ = blocks_[0];
        used_ = 0;
    }
}

size_t MemoryPool::totalAllocated() const {
    return blocks_.size() * blockSize_;
}

size_t MemoryPool::totalUsed() const {
    return used_;
}

void MemoryPool::allocateNewBlock() {
    void* block = std::malloc(blockSize_);
    if (!block) {
        throw std::bad_alloc();
    }

    blocks_.push_back(block);
    currentBlock_ = static_cast<char*>(block);
    used_ = 0;
}

} // namespace cpp20::compiler::common::utils
