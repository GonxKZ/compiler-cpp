/**
 * @file test_templates.cpp
 * @brief Ejemplo de prueba para el sistema de templates C++20
 */

#include <iostream>
#include <vector>
#include <string>

// Simulación de templates C++20 con concepts (implementación básica)
template<typename T>
concept Integral = std::is_integral_v<T>;

template<typename T>
concept FloatingPoint = std::is_floating_point_v<T>;

template<typename T>
concept Numeric = Integral<T> || FloatingPoint<T>;

// Template function con constraints
template<Numeric T>
T max(T a, T b) {
    return a > b ? a : b;
}

// Template class
template<typename T>
class Container {
private:
    std::vector<T> data;

public:
    Container() = default;

    void add(const T& value) {
        data.push_back(value);
    }

    T get(size_t index) const {
        if (index < data.size()) {
            return data[index];
        }
        throw std::out_of_range("Index out of range");
    }

    size_t size() const {
        return data.size();
    }
};

// Template specialization
template<>
class Container<std::string> {
private:
    std::vector<std::string> data;

public:
    Container() = default;

    void add(const std::string& value) {
        data.push_back(value);
    }

    std::string get(size_t index) const {
        if (index < data.size()) {
            return data[index];
        }
        throw std::out_of_range("Index out of range");
    }

    size_t size() const {
        return data.size();
    }

    // Método específico para strings
    std::string join(const std::string& separator = " ") const {
        if (data.empty()) return "";

        std::string result = data[0];
        for (size_t i = 1; i < data.size(); ++i) {
            result += separator + data[i];
        }
        return result;
    }
};

// Template con múltiples parámetros
template<typename T, typename U>
class Pair {
private:
    T first;
    U second;

public:
    Pair(T f, U s) : first(std::move(f)), second(std::move(s)) {}

    const T& getFirst() const { return first; }
    const U& getSecond() const { return second; }

    void setFirst(const T& f) { first = f; }
    void setSecond(const U& s) { second = s; }

    void print() const {
        std::cout << "(" << first << ", " << second << ")" << std::endl;
    }
};

// Template con non-type parameter
template<typename T, size_t N>
class Array {
private:
    T data[N];

public:
    Array() = default;

    T& operator[](size_t index) {
        if (index >= N) {
            throw std::out_of_range("Index out of bounds");
        }
        return data[index];
    }

    const T& operator[](size_t index) const {
        if (index >= N) {
            throw std::out_of_range("Index out of bounds");
        }
        return data[index];
    }

    size_t size() const { return N; }

    void fill(const T& value) {
        for (size_t i = 0; i < N; ++i) {
            data[i] = value;
        }
    }
};

// Función template con requires clause (C++20)
template<typename T>
requires Integral<T>
T gcd(T a, T b) {
    while (b != 0) {
        T temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// Template con variadic arguments (C++11/14/17 style)
template<typename... Args>
void printAll(Args&&... args) {
    (std::cout << ... << args) << std::endl;
}

// Conceptos personalizados (simulados)
template<typename T>
concept Printable = requires(T t) {
    std::cout << t;
};

template<Printable T>
void printValue(const T& value) {
    std::cout << "Value: " << value << std::endl;
}

// Template metaprogramming helper
template<typename T>
struct TypeInfo {
    static const char* name() {
        if constexpr (std::is_same_v<T, int>) return "int";
        else if constexpr (std::is_same_v<T, double>) return "double";
        else if constexpr (std::is_same_v<T, std::string>) return "string";
        else return "unknown";
    }
};

int main() {
    std::cout << "=== Sistema de Templates C++20 ===\n";

    // 1. Template functions con concepts
    std::cout << "\n1. Template Functions con Concepts:\n";
    std::cout << "max(10, 20) = " << max(10, 20) << std::endl;
    std::cout << "max(3.14, 2.71) = " << max(3.14, 2.71) << std::endl;

    // 2. Template classes
    std::cout << "\n2. Template Classes:\n";
    Container<int> intContainer;
    intContainer.add(1);
    intContainer.add(2);
    intContainer.add(3);
    std::cout << "Container<int> size: " << intContainer.size() << std::endl;
    std::cout << "Container<int>[1]: " << intContainer.get(1) << std::endl;

    Container<std::string> stringContainer;
    stringContainer.add("Hello");
    stringContainer.add("World");
    stringContainer.add("C++20");
    std::cout << "Container<string> join: " << stringContainer.join(", ") << std::endl;

    // 3. Template con múltiples parámetros
    std::cout << "\n3. Template con Múltiples Parámetros:\n";
    Pair<int, std::string> pair(42, "Answer");
    pair.print();
    std::cout << "First: " << pair.getFirst() << std::endl;
    std::cout << "Second: " << pair.getSecond() << std::endl;

    // 4. Template con non-type parameter
    std::cout << "\n4. Template con Non-Type Parameter:\n";
    Array<int, 5> intArray;
    intArray.fill(42);
    std::cout << "Array<int, 5> size: " << intArray.size() << std::endl;
    std::cout << "Array[2]: " << intArray[2] << std::endl;

    // 5. Template con requires clause
    std::cout << "\n5. Template con Requires Clause:\n";
    std::cout << "gcd(48, 18) = " << gcd(48, 18) << std::endl;
    // gcd(3.14, 2.71); // Esto causaría error de compilación

    // 6. Template variadic
    std::cout << "\n6. Template Variadic:\n";
    printAll("Hello", ", ", "World", "! ", 2024);

    // 7. Template con concept personalizado
    std::cout << "\n7. Template con Concept Personalizado:\n";
    printValue(42);
    printValue(3.14);
    printValue(std::string("Hello"));

    // 8. Template metaprogramming
    std::cout << "\n8. Template Metaprogramming:\n";
    std::cout << "Type of int: " << TypeInfo<int>::name() << std::endl;
    std::cout << "Type of double: " << TypeInfo<double>::name() << std::endl;
    std::cout << "Type of string: " << TypeInfo<std::string>::name() << std::endl;

    // 9. Template specialization
    std::cout << "\n9. Template Specialization:\n";
    Container<std::string> specializedContainer;
    specializedContainer.add("Template");
    specializedContainer.add("Specialization");
    std::cout << "Specialized container join: " << specializedContainer.join(" ") << std::endl;

    std::cout << "\n=== Demo completada exitosamente ===\n";
    std::cout << "✅ Template functions con concepts" << std::endl;
    std::cout << "✅ Template classes con especialización" << std::endl;
    std::cout << "✅ Templates con múltiples parámetros" << std::endl;
    std::cout << "✅ Templates con non-type parameters" << std::endl;
    std::cout << "✅ Requires clauses y constraints" << std::endl;
    std::cout << "✅ Templates variádicos" << std::endl;
    std::cout << "✅ Concepts personalizados" << std::endl;
    std::cout << "✅ Template metaprogramming" << std::endl;

    return 0;
}
