@echo off
REM Script de construcción para Windows

echo ========================================
echo Construyendo Compilador C++20
echo ========================================

REM Crear directorio de build si no existe
if not exist build mkdir build
cd build

REM Configurar con CMake
echo Configurando proyecto con CMake...
cmake .. -G "Visual Studio 17 2022" -A x64

REM Construir
echo Construyendo proyecto...
cmake --build . --config Release

REM Verificar resultado
if %errorlevel% equ 0 (
    echo ========================================
    echo ¡Construcción exitosa!
    echo Ejecutable: build\Release\cpp20-compiler.exe
    echo ========================================
) else (
    echo ========================================
    echo Error en la construcción
    echo ========================================
)

cd ..
pause
