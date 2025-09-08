/**
 * @file ExceptionMapper.h
 * @brief Mapeo de excepciones C++ a Windows Exception Handling
 */

#pragma once

#include <compiler/backend/unwind/UnwindTypes.h>
#include <vector>
#include <memory>

namespace cpp20::compiler::backend::unwind {

/**
 * @brief Información de una región try/catch
 */
struct TryCatchRegion {
    uint32_t tryStart;        // RVA de inicio del bloque try
    uint32_t tryEnd;          // RVA de fin del bloque try
    uint32_t catchStart;      // RVA de inicio del bloque catch
    uint32_t catchEnd;        // RVA de fin del bloque catch
    uint32_t exceptionTypeRVA; // RVA del tipo de excepción manejada (0 = catch-all)

    TryCatchRegion(uint32_t tryS, uint32_t tryE, uint32_t catchS, uint32_t catchE, uint32_t typeRVA = 0)
        : tryStart(tryS), tryEnd(tryE), catchStart(catchS), catchEnd(catchE), exceptionTypeRVA(typeRVA) {}
};

/**
 * @brief Información de un throw
 */
struct ThrowSite {
    uint32_t throwRVA;        // RVA del sitio del throw
    uint32_t exceptionTypeRVA; // RVA del tipo de excepción lanzada

    ThrowSite(uint32_t throwAddr, uint32_t typeRVA)
        : throwRVA(throwAddr), exceptionTypeRVA(typeRVA) {}
};

/**
 * @brief Mapeador de excepciones C++ a Windows EH
 *
 * Esta clase traduce las construcciones try/catch/throw de C++
 * a las estructuras de exception handling de Windows x64.
 */
class ExceptionMapper {
public:
    /**
     * @brief Constructor
     */
    ExceptionMapper();

    /**
     * @brief Destructor
     */
    ~ExceptionMapper();

    /**
     * @brief Añade una región try/catch
     */
    void addTryCatchRegion(const TryCatchRegion& region);

    /**
     * @brief Añade un sitio de throw
     */
    void addThrowSite(const ThrowSite& throwSite);

    /**
     * @brief Genera información de exception handler para Windows EH
     * @return RVA del exception handler (para UNWIND_INFO)
     */
    uint32_t generateExceptionHandler();

    /**
     * @brief Genera datos de exception data para la función
     * @return Vector de bytes con la información de excepciones
     */
    std::vector<uint8_t> generateExceptionData();

    /**
     * @brief Verifica si una función tiene excepciones
     */
    bool hasExceptions() const { return !tryCatchRegions_.empty() || !throwSites_.empty(); }

    /**
     * @brief Obtiene el número de regiones try/catch
     */
    size_t getTryCatchRegionCount() const { return tryCatchRegions_.size(); }

    /**
     * @brief Obtiene el número de sitios de throw
     */
    size_t getThrowSiteCount() const { return throwSites_.size(); }

private:
    /**
     * @brief Estructura interna para exception handler de Windows
     */
    struct WindowsExceptionHandler {
        uint32_t handlerRVA;          // RVA del handler
        uint32_t exceptionDataRVA;    // RVA de los datos de excepción
        std::vector<TryCatchRegion> regions;
        std::vector<ThrowSite> throws;

        WindowsExceptionHandler(uint32_t hRVA, uint32_t edRVA)
            : handlerRVA(hRVA), exceptionDataRVA(edRVA) {}
    };

    /**
     * @brief Genera el código del exception handler
     * @return RVA del handler generado
     */
    uint32_t generateHandlerCode();

    /**
     * @brief Genera datos de excepción en formato Windows
     */
    std::vector<uint8_t> generateWindowsExceptionData();

    // Miembros privados
    std::vector<TryCatchRegion> tryCatchRegions_;
    std::vector<ThrowSite> throwSites_;
    std::unique_ptr<WindowsExceptionHandler> windowsHandler_;
};

} // namespace cpp20::compiler::backend::unwind
