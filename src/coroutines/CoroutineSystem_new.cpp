/**
 * @file CoroutineSystem.cpp
 * @brief Implementación del sistema de corroutinas compatible con MSVC
 */

#include <compiler/coroutines/CoroutineSystem.h>
#include <iostream>
#include <thread>
#include <chrono>

namespace cpp20::compiler::coroutines {

// ============================================================================
// SimpleCoroutine Implementation
// ============================================================================

SimpleCoroutine::SimpleCoroutine(std::function<void()> func)
    : function_(std::move(func)), state_(CoroutineState::Suspended) {
}

void SimpleCoroutine::resume() {
    if (state_ == CoroutineState::Suspended) {
        state_ = CoroutineState::Running;
        try {
            function_();
            state_ = CoroutineState::Done;
        } catch (...) {
            state_ = CoroutineState::Exception;
            throw;
        }
    }
}

// ============================================================================
// PingPongCoroutine Implementation
// ============================================================================

PingPongCoroutine::PingPongCoroutine(const std::string& name, int* counter, int max_count)
    : name_(name), counter_(counter), max_count_(max_count),
      current_step_(0), done_(false) {
}

void PingPongCoroutine::resume() {
    if (done_) return;

    // Simular trabajo de la corrutina
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Hacer ping o pong
    std::cout << "🎾 " << name_ << " (" << *counter_ << ")" << std::endl;

    // Incrementar contador
    (*counter_)++;

    // Verificar si hemos terminado
    if (*counter_ >= max_count_) {
        done_ = true;
        std::cout << "✅ " << name_ << " completado!" << std::endl;
    }
}

bool PingPongCoroutine::isDone() const {
    return done_;
}

int PingPongCoroutine::getCounter() const {
    return *counter_;
}

// ============================================================================
// Función de demostración
// ============================================================================

void run_ping_pong_demo(int max_count) {
    std::cout << "🎾 DEMOSTRACIÓN DE CORROUTINAS - PING PONG" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "📊 Conteo máximo: " << max_count << std::endl;
    std::cout << "🔄 Las corroutinas alternarán turnos" << std::endl;
    std::cout << std::string(50, '=') << std::endl;

    int counter = 0;

    // Crear corroutinas
    PingPongCoroutine ping("PING", &counter, max_count);
    PingPongCoroutine pong("PONG", &counter, max_count);

    // Ejecutar hasta que ambas terminen
    while (!ping.isDone() || !pong.isDone()) {
        // Reanudar ping si no está terminado
        if (!ping.isDone()) {
            ping.resume();
        }

        // Pequeña pausa para simular alternancia
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Reanudar pong si no está terminado
        if (!pong.isDone()) {
            pong.resume();
        }

        // Pequeña pausa entre turnos
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << std::string(50, '=') << std::endl;
    std::cout << "🎉 Ping-Pong completado!" << std::endl;
    std::cout << "📊 Contador final: " << counter << std::endl;
    std::cout << "🏆 Ping completó: " << ping.getCounter() << " turnos" << std::endl;
    std::cout << "🏆 Pong completó: " << pong.getCounter() << " turnos" << std::endl;
}

// ============================================================================
// Fin del archivo
// ============================================================================
