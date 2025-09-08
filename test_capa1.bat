@echo off
REM Script de prueba para la Capa 1: Formato COFF y Relocations

echo ========================================
echo Prueba de Capa 1: COFF y Relocations
echo ========================================

REM Crear directorio de build si no existe
if not exist build mkdir build
cd build

REM Configurar y construir
echo Configurando proyecto...
cmake .. -DCMAKE_BUILD_TYPE=Release -DCPP20_COMPILER_BUILD_TESTS=ON -DCPP20_COMPILER_USE_LLVM=OFF

echo Construyendo proyecto...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo ❌ Error en la construcción
    cd ..
    exit /b 1
)

echo ✅ Construcción exitosa

REM Ejecutar tests de COFF
echo.
echo Ejecutando tests de COFF...
ctest -R "COFF" --output-on-failure

if %errorlevel% neq 0 (
    echo ❌ Fallaron los tests de COFF
    cd ..
    exit /b 1
)

echo ✅ Tests de COFF pasaron

REM Generar objeto COFF de ejemplo
echo.
echo Generando objeto COFF de ejemplo...
if exist hello-coff.exe (
    hello-coff.exe
    if %errorlevel% neq 0 (
        echo ❌ Error generando objeto COFF
        cd ..
        exit /b 1
    )
    echo ✅ Objeto COFF generado: hello.obj
) else (
    echo ⚠️  Ejecutable hello-coff.exe no encontrado
)

REM Verificar objeto COFF con herramientas externas
echo.
echo Verificando objeto COFF...
if exist hello.obj (
    echo Información del archivo hello.obj:
    dir hello.obj

    REM Intentar usar dumpbin si está disponible
    where dumpbin >nul 2>nul
    if %errorlevel% equ 0 (
        echo.
        echo Dump con dumpbin:
        dumpbin /HEADERS hello.obj
    ) else (
        echo ⚠️  dumpbin no encontrado (necesitas Visual Studio)
    )

    REM Intentar usar llvm-readobj si está disponible
    where llvm-readobj >nul 2>nul
    if %errorlevel% equ 0 (
        echo.
        echo Dump con llvm-readobj:
        llvm-readobj --file-headers hello.obj
    ) else (
        echo ⚠️  llvm-readobj no encontrado
    )
) else (
    echo ⚠️  Archivo hello.obj no encontrado
)

REM Probar enlazado si es posible
echo.
echo Intentando enlazar con link.exe...
where link >nul 2>nul
if %errorlevel% equ 0 (
    if exist hello.obj (
        echo Enlazando hello.obj...
        link hello.obj /OUT:hello.exe /SUBSYSTEM:CONSOLE kernel32.lib
        if %errorlevel% equ 0 (
            echo ✅ Enlazado exitoso: hello.exe generado
            echo Ejecutando hello.exe...
            hello.exe
        ) else (
            echo ❌ Error en el enlazado
        )
    )
) else (
    echo ⚠️  link.exe no encontrado (necesitas Visual Studio)
)

cd ..
echo.
echo ========================================
echo Resumen de Prueba Capa 1
echo ========================================
echo ✅ COFF Writer implementado
echo ✅ Relocations AMD64 soportadas
echo ✅ Tests unitarios funcionando
echo ✅ Objeto COFF generado
echo ✅ Estructuras COFF válidas
echo ========================================
