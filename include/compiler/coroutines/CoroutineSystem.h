/**
 * @file CoroutineSystem.h
 * @brief Sistema completo de corroutinas C++20 compatible con MSVC
 */

#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <iostream>
#include <thread>
#include <chrono>
#include <any>
#include <exception>

// Configuración específica por compilador para corroutinas
#if defined(_MSC_VER) && !defined(__clang__)
    // MSVC: intenta usar coroutines estándar si están disponibles
    #if __cpp_coroutines >= 201703L
        #include <coroutine>
        #define COROUTINE_SUSPEND_ALWAYS std::suspend_always{}
        #define COROUTINE_INITIAL_SUSPEND std::suspend_always{}
        #define COROUTINE_FINAL_SUSPEND std::suspend_always{}
        #define COROUTINE_HANDLE std::coroutine_handle
        #define HAS_COROUTINES 1
    #else
        // Fallback para MSVC sin soporte completo
        #define COROUTINE_SUSPEND_ALWAYS nullptr
        #define COROUTINE_INITIAL_SUSPEND nullptr
        #define COROUTINE_FINAL_SUSPEND nullptr
        #define COROUTINE_HANDLE void*
        #define HAS_COROUTINES 0
    #endif
#else
    // Otros compiladores: usa <coroutine>
    #include <coroutine>
    #define COROUTINE_SUSPEND_ALWAYS std::suspend_always{}
    #define COROUTINE_INITIAL_SUSPEND std::suspend_always{}
    #define COROUTINE_FINAL_SUSPEND std::suspend_always{}
    #define COROUTINE_HANDLE std::coroutine_handle
    #define HAS_COROUTINES 1
#endif

namespace cpp20::compiler::coroutines {

// ============================================================================
// Forward declarations
// ============================================================================

class CoroutineFrame;
class CoroutinePromise;
class Awaitable;
class Awaiter;
class CoroutineHandle;
class CoroutineTransformer;
class CoroutineRuntime;

// ============================================================================
// Enums y tipos básicos
// ============================================================================

/**
 * @brief Estado de una corrutina
 */
enum class CoroutineState {
    Suspended,      // Suspendida (puede ser reanudada)
    Running,        // Ejecutándose
    Done,          // Completada
    Destroyed,     // Destruida
    Exception      // Excepción pendiente
};

/**
 * @brief Tipo de suspensión
 */
enum class SuspensionPoint {
    Initial,       // Punto inicial (antes del primer await)
    Yield,         // Punto de yield (co_yield)
    Final,         // Punto final (antes de return/destrucción)
    Exception      // Punto de excepción
};

/**
 * @brief Resultado de una operación await
 */
enum class AwaitResult {
    Ready,         // El awaitable está listo
    Suspended,     // La corrutina debe suspenderse
    Done           // La corrutina ha terminado
};

// ============================================================================
// SimpleCoroutine - Implementación simplificada
// ============================================================================

/**
 * @brief Corroutina simple para demostración
 */
class SimpleCoroutine {
public:
    /**
     * @brief Constructor
     */
    SimpleCoroutine(std::function<void()> func);

    /**
     * @brief Destructor
     */
    ~SimpleCoroutine() = default;

    /**
     * @brief Reanudar la corrutina
     */
    void resume();

    /**
     * @brief Verificar si está completada
     */
    bool isDone() const { return state_ == CoroutineState::Done; }

    /**
     * @brief Obtener estado
     */
    CoroutineState getState() const { return state_; }

private:
    std::function<void()> function_;
    CoroutineState state_;
};

// ============================================================================
// PingPongCoroutine - Demostración específica
// ============================================================================

/**
 * @brief Corroutina especializada para ping-pong
 */
class PingPongCoroutine {
public:
    /**
     * @brief Constructor
     */
    PingPongCoroutine(const std::string& name, int* counter, int max_count);

    /**
     * @brief Reanudar la corroutina
     */
    void resume();

    /**
     * @brief Verificar si está completada
     */
    bool isDone() const;

    /**
     * @brief Obtener contador
     */
    int getCounter() const;

private:
    std::string name_;
    int* counter_;
    int max_count_;
    int current_step_;
    bool done_;
};

// ============================================================================
// Task - Implementación básica de Task
// ============================================================================

/**
 * @brief Task simple con valor de retorno
 */
template<typename T>
class Task {
public:
    /**
     * @brief Constructor
     */
    Task() : done_(false) {}

    /**
     * @brief Constructor con valor
     */
    explicit Task(T value) : value_(std::move(value)), done_(true) {}

    /**
     * @brief Verificar si está completada
     */
    bool isDone() const { return done_; }

    /**
     * @brief Obtener valor (solo si está completada)
     */
    T get() const {
        if (!done_) throw std::runtime_error("Task not completed");
        return value_;
    }

    /**
     * @brief Establecer valor y marcar como completada
     */
    void set_value(T val) {
        value_ = std::move(val);
        done_ = true;
    }

private:
    T value_;
    bool done_;
};

/**
 * @brief Task sin valor de retorno
 */
class VoidTask {
public:
    /**
     * @brief Constructor
     */
    VoidTask() : done_(false) {}

    /**
     * @brief Constructor completado
     */
    explicit VoidTask(bool completed) : done_(completed) {}

    /**
     * @brief Verificar si está completada
     */
    bool isDone() const { return done_; }

    /**
     * @brief Marcar como completada
     */
    void set_done() { done_ = true; }

private:
    bool done_;
};

// ============================================================================
// Función de demostración
// ============================================================================

/**
 * @brief Ejecutar ping-pong con corroutinas
 */
void run_ping_pong_demo(int max_count);

} // namespace cpp20::compiler::coroutines
