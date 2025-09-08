/**
 * @file modules_main.cpp
 * @brief Programa principal que usa el módulo math
 */

import math;
import <iostream>;
import <vector>;
import <string>;

int main() {
    std::cout << "=== Demostración del Sistema de Módulos C++20 ===\n" << std::endl;

    // Usar funciones del módulo math
    std::cout << "Factorial de 5: " << math::factorial(5) << std::endl;
    std::cout << "2^8 = " << math::power(2.0, 8) << std::endl;
    std::cout << "¿Es 17 primo? " << (math::is_prime(17) ? "Sí" : "No") << std::endl;
    std::cout << "¿Es 18 primo? " << (math::is_prime(18) ? "Sí" : "No") << std::endl;

    // Usar constantes del módulo
    std::cout << "Valor de PI: " << PI << std::endl;
    std::cout << "Valor de E: " << E << std::endl;

    // Usar clase del módulo
    Circle circle(5.0);
    std::cout << "Círculo con radio " << circle.get_radius() << ":" << std::endl;
    std::cout << "  Área: " << circle.area() << std::endl;
    std::cout << "  Circunferencia: " << circle.circumference() << std::endl;

    // Usar función template
    std::cout << "Valor absoluto de -42: " << absolute(-42) << std::endl;
    std::cout << "Valor absoluto de -3.14: " << absolute(-3.14) << std::endl;

    // Usar funciones adicionales
    std::cout << "Raíz cuadrada de 16: " << math::square_root(16.0) << std::endl;
    std::cout << "MCD de 48 y 18: " << math::gcd(48, 18) << std::endl;

    std::cout << "\n=== Demostración completada exitosamente ===" << std::endl;
    std::cout << "✅ Sistema de módulos C++20 funcionando correctamente" << std::endl;

    return 0;
}
