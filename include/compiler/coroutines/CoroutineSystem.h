/**
 * @file CoroutineSystem.h
 * @brief Sistema completo de corroutinas C++20
 */

#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <type_traits>

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
// Coroutine Frame - Layout del frame ABI-compliant
// ============================================================================

/**
 * @brief Frame de corrutina con layout ABI-compliant
 */
class CoroutineFrame {
public:
    /**
     * @brief Constructor
     */
    CoroutineFrame(size_t frameSize, void* promisePtr);

    /**
     * @brief Destructor
     */
    ~CoroutineFrame();

    /**
     * @brief Obtener tamaño del frame
     */
    size_t getFrameSize() const { return frameSize_; }

    /**
     * @brief Obtener puntero al frame
     */
    void* getFramePtr() const { return framePtr_; }

    /**
     * @brief Obtener puntero a la promesa
     */
    void* getPromisePtr() const { return promisePtr_; }

    /**
     * @brief Establecer función de continuación
     */
    void setResumeFunction(std::function<void(CoroutineFrame*)> resumeFunc);

    /**
     * @brief Establecer función de destrucción
     */
    void setDestroyFunction(std::function<void(CoroutineFrame*)> destroyFunc);

    /**
     * @brief Obtener estado de la corrutina
     */
    CoroutineState getState() const { return state_; }

    /**
     * @brief Establecer estado de la corrutina
     */
    void setState(CoroutineState state) { state_ = state; }

    /**
     * @brief Obtener punto de suspensión actual
     */
    SuspensionPoint getCurrentPoint() const { return currentPoint_; }

    /**
     * @brief Establecer punto de suspensión actual
     */
    void setCurrentPoint(SuspensionPoint point) { currentPoint_ = point; }

    /**
     * @brief Reanudar la corrutina
     */
    void resume();

    /**
     * @brief Destruir la corrutina
     */
    void destroy();

    /**
     * @brief Verificar si está lista para destrucción
     */
    bool isReadyForDestruction() const;

private:
    size_t frameSize_;
    void* framePtr_;
    void* promisePtr_;
    CoroutineState state_;
    SuspensionPoint currentPoint_;

    std::function<void(CoroutineFrame*)> resumeFunction_;
    std::function<void(CoroutineFrame*)> destroyFunction_;

    // Datos del frame ABI-compliant
    struct FrameData {
        void* resumePtr;           // Puntero a función de reanudación
        void* destroyPtr;          // Puntero a función de destrucción
        void* promisePtr;          // Puntero a la promesa
        uint32_t index;           // Índice del punto de suspensión actual
        uint32_t frameSize;       // Tamaño del frame
        void* exceptionPtr;       // Puntero a excepción pendiente
    };

    FrameData* frameData_;
};

// ============================================================================
// Coroutine Promise - Concepto base para promesas
// ============================================================================

/**
 * @brief Base para todas las promesas de corrutinas
 */
class CoroutinePromise {
public:
    /**
     * @brief Constructor
     */
    CoroutinePromise();

    /**
     * @brief Destructor
     */
    virtual ~CoroutinePromise() = default;

    /**
     * @brief Obtener el valor de retorno (para co_return)
     */
    virtual void return_value(auto&& value) {
        // Por defecto, ignorar el valor
    }

    /**
     * @brief Obtener el valor de yield (para co_yield)
     */
    virtual auto yield_value(auto&& value) {
        // Por defecto, devolver el valor sin modificar
        return value;
    }

    /**
     * @brief Manejar suspensión inicial
     */
    virtual std::suspend_always initial_suspend() noexcept {
        return {};
    }

    /**
     * @brief Manejar suspensión final
     */
    virtual std::suspend_always final_suspend() noexcept {
        return {};
    }

    /**
     * @brief Manejar excepciones no capturadas
     */
    virtual void unhandled_exception() {
        // Por defecto, re-lanzar la excepción
        throw;
    }

    /**
     * @brief Obtener el resultado final
     */
    virtual auto get_return_object() {
        // Esto debe ser implementado por promesas concretas
        throw std::runtime_error("get_return_object not implemented");
    }

    /**
     * @brief Obtener el frame de la corrutina
     */
    CoroutineFrame* getFrame() const { return frame_; }

    /**
     * @brief Establecer el frame de la corrutina
     */
    void setFrame(CoroutineFrame* frame) { frame_ = frame; }

private:
    CoroutineFrame* frame_;
};

// ============================================================================
// Awaitable y Awaiter - Sistema de await
// ============================================================================

/**
 * @brief Concepto base para objetos awaitable
 */
class Awaitable {
public:
    /**
     * @brief Constructor
     */
    Awaitable() = default;

    /**
     * @brief Destructor
     */
    virtual ~Awaitable() = default;

    /**
     * @brief Crear awaiter para este awaitable
     */
    virtual Awaiter* createAwaiter() = 0;

    /**
     * @brief Verificar si el awaitable está listo
     */
    virtual bool await_ready() const noexcept = 0;

    /**
     * @brief Suspender la corrutina
     */
    virtual void await_suspend(CoroutineHandle handle) = 0;

    /**
     * @brief Reanudar después de suspensión
     */
    virtual auto await_resume() = 0;
};

/**
 * @brief Awaiter que maneja la suspensión/reanudación
 */
class Awaiter {
public:
    /**
     * @brief Constructor
     */
    Awaiter(Awaitable* awaitable);

    /**
     * @brief Destructor
     */
    virtual ~Awaiter() = default;

    /**
     * @brief Verificar si está listo
     */
    bool await_ready() const noexcept;

    /**
     * @brief Suspender la corrutina
     */
    void await_suspend(CoroutineHandle handle);

    /**
     * @brief Reanudar después de suspensión
     */
    auto await_resume();

    /**
     * @brief Obtener resultado de la operación await
     */
    AwaitResult getResult() const { return result_; }

    /**
     * @brief Establecer resultado de la operación await
     */
    void setResult(AwaitResult result) { result_ = result; }

private:
    Awaitable* awaitable_;
    AwaitResult result_;
    CoroutineHandle suspendedHandle_;
};

// ============================================================================
// Coroutine Handle - Handle para controlar corrutinas
// ============================================================================

/**
 * @brief Handle para controlar corrutinas externas
 */
class CoroutineHandle {
public:
    /**
     * @brief Constructor
     */
    CoroutineHandle(CoroutineFrame* frame = nullptr);

    /**
     * @brief Destructor
     */
    ~CoroutineHandle() = default;

    /**
     * @brief Verificar si el handle es válido
     */
    bool isValid() const { return frame_ != nullptr; }

    /**
     * @brief Reanudar la corrutina
     */
    void resume();

    /**
     * @brief Destruir la corrutina
     */
    void destroy();

    /**
     * @brief Verificar si está listo para reanudación
     */
    bool isDone() const;

    /**
     * @brief Obtener el frame de la corrutina
     */
    CoroutineFrame* getFrame() const { return frame_; }

    /**
     * @brief Obtener estado de la corrutina
     */
    CoroutineState getState() const;

    /**
     * @brief Crear handle desde promesa
     */
    template<typename PromiseType>
    static CoroutineHandle from_promise(PromiseType& promise) {
        CoroutineFrame* frame = promise.getFrame();
        return CoroutineHandle(frame);
    }

    /**
     * @brief Crear handle nulo
     */
    static CoroutineHandle null() {
        return CoroutineHandle(nullptr);
    }

private:
    CoroutineFrame* frame_;
};

// ============================================================================
// Coroutine Transformer - Transformador de funciones en corroutinas
// ============================================================================

/**
 * @brief Transformador que convierte funciones normales en corroutinas
 */
class CoroutineTransformer {
public:
    /**
     * @brief Constructor
     */
    CoroutineTransformer();

    /**
     * @brief Destructor
     */
    ~CoroutineTransformer() = default;

    /**
     * @brief Transformar función en corrutina
     */
    template<typename Func, typename... Args>
    auto transformToCoroutine(Func&& func, Args&&... args) {
        // Crear frame para la corrutina
        constexpr size_t frameSize = 1024; // Tamaño estimado del frame
        auto frame = std::make_unique<CoroutineFrame>(frameSize, nullptr);

        // Crear promesa
        using PromiseType = typename std::decay_t<Func>::promise_type;
        auto promise = std::make_unique<PromiseType>();
        promise->setFrame(frame.get());

        // Configurar funciones de control
        frame->setResumeFunction([this, &func, &args..., promise = promise.get()](CoroutineFrame* f) {
            this->executeCoroutineFunction(f, promise, func, args...);
        });

        frame->setDestroyFunction([promise = promise.get()](CoroutineFrame* f) {
            // Destrucción segura
            promise->~PromiseType();
        });

        // Crear handle
        CoroutineHandle handle(frame.get());

        // Transferir ownership del frame
        frame.release(); // El runtime se encargará de la destrucción

        return std::make_pair(handle, promise->get_return_object());
    }

private:
    /**
     * @brief Ejecutar función de corrutina
     */
    template<typename PromiseType, typename Func, typename... Args>
    void executeCoroutineFunction(CoroutineFrame* frame, PromiseType* promise,
                                Func&& func, Args&&... args) {
        try {
            // Ejecutar la función con la promesa
            auto coro = func(std::forward<Args>(args)...);

            // Configurar el frame en la promesa
            promise->setFrame(frame);

            // El resto se maneja por la implementación específica
        } catch (...) {
            promise->unhandled_exception();
        }
    }
};

// ============================================================================
// Coroutine Runtime - Runtime para gestión de corroutinas
// ============================================================================

/**
 * @brief Runtime para gestión completa del ciclo de vida de corroutinas
 */
class CoroutineRuntime {
public:
    /**
     * @brief Constructor
     */
    CoroutineRuntime();

    /**
     * @brief Destructor
     */
    ~CoroutineRuntime();

    /**
     * @brief Crear nueva corrutina
     */
    template<typename Func, typename... Args>
    CoroutineHandle createCoroutine(Func&& func, Args&&... args) {
        return transformer_.transformToCoroutine(std::forward<Func>(func),
                                               std::forward<Args>(args)...).first;
    }

    /**
     * @brief Crear nueva corrutina con promesa personalizada
     */
    template<typename PromiseType, typename Func, typename... Args>
    CoroutineHandle createCoroutineWithPromise(Func&& func, Args&&... args) {
        // Similar pero con tipo de promesa específico
        return createCoroutine(std::forward<Func>(func), std::forward<Args>(args)...);
    }

    /**
     * @brief Reanudar corrutina
     */
    void resume(CoroutineHandle handle);

    /**
     * @brief Destruir corrutina
     */
    void destroy(CoroutineHandle handle);

    /**
     * @brief Verificar si corrutina está lista
     */
    bool isDone(CoroutineHandle handle) const;

    /**
     * @brief Obtener estadísticas del runtime
     */
    struct RuntimeStats {
        size_t totalCoroutines = 0;
        size_t activeCoroutines = 0;
        size_t suspendedCoroutines = 0;
        size_t completedCoroutines = 0;
        size_t destroyedCoroutines = 0;
    };
    RuntimeStats getStats() const;

    /**
     * @brief Limpiar corrutinas completadas
     */
    void cleanup();

private:
    CoroutineTransformer transformer_;
    std::vector<std::unique_ptr<CoroutineFrame>> frames_;

    RuntimeStats stats_;

    void updateStats();
};

// ============================================================================
// Awaitables predefinidos
// ============================================================================

/**
 * @brief Awaitable simple que siempre está listo
 */
class ReadyAwaitable : public Awaitable {
public:
    template<typename T>
    ReadyAwaitable(T&& value) : value_(std::forward<T>(value)) {}

    Awaiter* createAwaiter() override;
    bool await_ready() const noexcept override { return true; }
    void await_suspend(CoroutineHandle handle) override {}
    auto await_resume() override { return std::move(value_); }

private:
    std::any value_;
};

/**
 * @brief Awaitable que suspende por tiempo
 */
class SleepAwaitable : public Awaitable {
public:
    SleepAwaitable(std::chrono::milliseconds duration);

    Awaiter* createAwaiter() override;
    bool await_ready() const noexcept override;
    void await_suspend(CoroutineHandle handle) override;
    auto await_resume() override;

private:
    std::chrono::milliseconds duration_;
    std::chrono::steady_clock::time_point startTime_;
};

// ============================================================================
// Funciones helper para crear corroutinas
// ============================================================================

/**
 * @brief Helper para crear corroutinas simples
 */
template<typename T>
struct Task {
    struct promise_type {
        T value;

        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        void return_value(T v) { value = v; }
        void unhandled_exception() { throw; }
    };

    std::coroutine_handle<promise_type> handle;

    Task(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~Task() { if (handle) handle.destroy(); }

    T get() { return handle.promise().value; }
    bool isDone() const { return handle.done(); }
    void resume() { handle.resume(); }
};

/**
 * @brief Task sin valor de retorno
 */
struct VoidTask {
    struct promise_type {
        VoidTask get_return_object() {
            return VoidTask{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        void return_void() {}
        void unhandled_exception() { throw; }
    };

    std::coroutine_handle<promise_type> handle;

    VoidTask(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~VoidTask() { if (handle) handle.destroy(); }

    bool isDone() const { return handle.done(); }
    void resume() { handle.resume(); }
};

} // namespace cpp20::compiler::coroutines
