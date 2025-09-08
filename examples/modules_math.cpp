/**
 * @file modules_math.cpp
 * @brief Implementación del módulo math
 */

module math;

// Implementación de funciones no exportadas (privadas del módulo)
namespace math {

    // Función helper no exportada
    double square(double x) {
        return x * x;
    }

}

// Implementación de funciones exportadas adicionales
export namespace math {

    /**
     * @brief Calcula la raíz cuadrada usando aproximación
     */
    export double square_root(double x) {
        if (x < 0) return 0.0;

        double guess = x / 2.0;
        for (int i = 0; i < 10; ++i) {
            guess = (guess + x / guess) / 2.0;
        }
        return guess;
    }

    /**
     * @brief Calcula el máximo común divisor
     */
    export int gcd(int a, int b) {
        while (b != 0) {
            int temp = b;
            b = a % b;
            a = temp;
        }
        return a;
    }

}
