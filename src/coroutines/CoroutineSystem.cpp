#include <compiler/coroutines/CoroutineSystem.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <string>

namespace cpp20
{
namespace compiler
{
namespace coroutines
{

SimpleCoroutine::SimpleCoroutine(std::function<void()> func)
    : function_(std::move(func)), state_(CoroutineState::Suspended)
{
}

void SimpleCoroutine::resume()
{
    if (state_ == CoroutineState::Suspended)
    {
        state_ = CoroutineState::Running;
        try
        {
            function_();
            state_ = CoroutineState::Done;
        }
        catch (...)
        {
            state_ = CoroutineState::Exception;
            throw;
        }
    }
}

PingPongCoroutine::PingPongCoroutine(const std::string& name, int* counter, int max_count)
    : name_(name), counter_(counter), max_count_(max_count),
      current_step_(0), done_(false)
{
}

void PingPongCoroutine::resume()
{
    if (done_) return;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "[CORO] " << name_ << " (" << *counter_ << ")" << std::endl;

    (*counter_)++;

    if (*counter_ >= max_count_)
    {
        done_ = true;
        std::cout << "[DONE] " << name_ << " completado!" << std::endl;
    }
}

bool PingPongCoroutine::isDone() const
{
    return done_;
}

int PingPongCoroutine::getCounter() const
{
    return *counter_;
}

void run_ping_pong_demo(int max_count)
{
    std::cout << "=== DEMOSTRACION DE CORROUTINAS - PING PONG ===" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "Conteo maximo: " << max_count << std::endl;
    std::cout << "Las corroutinas alternaran turnos" << std::endl;
    std::cout << std::string(50, '=') << std::endl;

    int counter = 0;

    PingPongCoroutine ping("PING", &counter, max_count);
    PingPongCoroutine pong("PONG", &counter, max_count);

    while (!ping.isDone() || !pong.isDone())
    {
        if (!ping.isDone())
        {
            ping.resume();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        if (!pong.isDone())
        {
            pong.resume();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << std::string(50, '=') << std::endl;
    std::cout << "Ping-Pong completado!" << std::endl;
    std::cout << "Contador final: " << counter << std::endl;
    std::cout << "Ping completo: " << ping.getCounter() << " turnos" << std::endl;
    std::cout << "Pong completo: " << pong.getCounter() << " turnos" << std::endl;
}

} // namespace coroutines
} // namespace compiler
} // namespace cpp20