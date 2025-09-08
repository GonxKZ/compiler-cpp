#!/bin/bash

# Script de construcción para Linux/Mac

echo "========================================"
echo "Construyendo Compilador C++20"
echo "========================================"

# Crear directorio de build si no existe
mkdir -p build
cd build

# Configurar con CMake
echo "Configurando proyecto con CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Construir
echo "Construyendo proyecto..."
make -j$(nproc)

# Verificar resultado
if [ $? -eq 0 ]; then
    echo "========================================"
    echo "¡Construcción exitosa!"
    echo "Ejecutable: build/cpp20-compiler"
    echo "========================================"
else
    echo "========================================"
    echo "Error en la construcción"
    echo "========================================"
    exit 1
fi

cd ..
echo "Para ejecutar el compilador:"
echo "  ./build/cpp20-compiler --help"
