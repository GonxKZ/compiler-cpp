#pragma once

#include <vector>
#include <cstddef>

namespace cpp20::compiler::common::utils {

/**
 * @brief Pool de memoria para allocations eficientes
 *
 * TODO: Implementar pooling real en Capa 4+
 * Por ahora es un wrapper simple sobre malloc/free
 */
class MemoryPool {
public:
    /**
     * @brief Constructor
     * @param blockSize Tamaño de cada bloque
     * @param initialBlocks Número inicial de bloques
     */
    MemoryPool(size_t blockSize = 4096, size_t initialBlocks = 1);

    /**
     * @brief Destructor
     */
    ~MemoryPool();

    /**
     * @brief Alloca memoria
     * @param size Tamaño a allocar
     * @return Puntero a la memoria allocada
     */
    void* allocate(size_t size);

    /**
     * @brief Libera memoria
     * @param ptr Puntero a liberar
     * @param size Tamaño original (para future pooling)
     */
    void deallocate(void* ptr, size_t size);

    /**
     * @brief Resetea el pool (libera bloques extra)
     */
    void reset();

    /**
     * @brief Obtiene total de memoria allocada
     */
    size_t totalAllocated() const;

    /**
     * @brief Obtiene total de memoria usada
     */
    size_t totalUsed() const;

private:
    size_t blockSize_;
    size_t initialBlocks_;
    std::vector<void*> blocks_;
    char* currentBlock_;
    size_t used_;

    void allocateNewBlock();
};

} // namespace cpp20::compiler::common::utils
