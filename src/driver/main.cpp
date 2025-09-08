/**
 * @file main.cpp
 * @brief Punto de entrada principal del compilador C++20
 */

#include "CompilerDriver.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    try {
        // Crear el driver del compilador
        cpp20::compiler::CompilerDriver driver;

        // Procesar argumentos de línea de comandos y ejecutar
        int result = driver.run(argc, argv);

        return result;

    } catch (const std::exception& e) {
        std::cerr << "Error fatal del compilador: " << e.what() << std::endl;
        return EXIT_FAILURE;

    } catch (...) {
        std::cerr << "Error fatal desconocido del compilador" << std::endl;
        return EXIT_FAILURE;
    }
}
