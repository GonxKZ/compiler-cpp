/**
 * @file modules_math.ixx
 * @brief Ejemplo de módulo de interfaz C++20 para matemáticas
 */

export module math;

// Exportar funciones matemáticas básicas
export namespace math {

    /**
     * @brief Calcula el factorial de un número
     */
    export int factorial(int n) {
        if (n <= 1) return 1;
        return n * factorial(n - 1);
    }

    /**
     * @brief Calcula la potencia de un número
     */
    export double power(double base, int exponent) {
        double result = 1.0;
        for (int i = 0; i < exponent; ++i) {
            result *= base;
        }
        return result;
    }

    /**
     * @brief Verifica si un número es primo
     */
    export bool is_prime(int n) {
        if (n <= 1) return false;
        if (n <= 3) return true;
        if (n % 2 == 0 || n % 3 == 0) return false;

        for (int i = 5; i * i <= n; i += 6) {
            if (n % i == 0 || n % (i + 2) == 0) return false;
        }

        return true;
    }

}

// Exportar constantes matemáticas
export constexpr double PI = 3.141592653589793;
export constexpr double E = 2.718281828459045;

// Exportar clase para operaciones geométricas
export class Circle {
private:
    double radius_;

public:
    Circle(double radius) : radius_(radius) {}

    double area() const {
        return PI * radius_ * radius_;
    }

    double circumference() const {
        return 2 * PI * radius_;
    }

    double get_radius() const {
        return radius_;
    }
};

// Exportar función template para cálculo genérico
export template<typename T>
T absolute(T value) {
    return value < 0 ? -value : value;
}
