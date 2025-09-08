# Compilador C++20 para Windows x64

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.20+-green.svg)](https://cmake.org/)

Compilador completo C++20 desde cero, orientado específicamente a Windows x64. Implementa un **front-end personalizado** con **back-end basado en LLVM** para optimizar la generación de código y asegurar compatibilidad con el ecosistema PE/COFF.

## Arquitectura

```
Front-end Personalizado          Back-end LLVM
├── Preprocesador completo       ├── Generación IR SSA
├── Lexer con UCNs/UCN           ├── ABI x64 Microsoft
├── Parser recursivo descendente ├── Name mangling MSVC
├── Sistema de tipos completo     ├── VTables y RTTI
├── Motor de plantillas           ├── EH Windows/LLVM
├── Constexpr evaluator          └── LLD integration
└── Módulos C++20
```

## Características Principales

### 🎯 **C++20 Completo**
- ✅ C++17 como base sólida
- 🔄 **constexpr** extendido (constinit, consteval)
- 🔄 **Lambdas** genéricas y captura mejorada
- 🔄 **auto/decltype** mejorados
- 🔄 **Conceptos** y restricciones (requires)
- 🔄 **Módulos** C++20 (module, import, export)
- 🔄 **Corrutinas** (co_await, co_yield, co_return)

### 🏗️ **Arquitectura Robusta**
- **Front-end propio** con 8 fases de traducción normativas
- **Sistema de tipos** con deduction, cualificaciones y referencias
- **Motor de lookup** con two-phase name lookup
- **Sistema de símbolos** por ámbitos (namespace, clase, plantilla)
- **AST fuertemente tipado** con visitor pattern
- **Sistema de diagnósticos** completo con localización precisa

### 🔧 **Back-end Optimizado**
- **Integración LLVM** 15.0+ para generación de código
- **ABI x64 Microsoft** completo (convención de llamadas, shadow space)
- **Name mangling** compatible con MSVC
- **RTTI y excepciones** nativas de Windows
- **LLD** como enlazador principal

### 🎛️ **Driver Profesional**
- CLI compatible con expectativas modernas
- Modos: `-c`, `-S`, `-E`, `--emit-bmi`
- Opciones: `-O0-3`, `-g`, `-std=c++20`
- Detección automática de entorno Windows SDK/CRT
- Response files y compatibilidad MSVC

## Ecosistema Windows

### ✅ **Compatibilidad Total**
- **Convención de llamadas** MS x64
- **Formato PE/COFF** nativo
- **Name mangling** MSVC
- **SDK/CRT** de Microsoft
- **Herramientas**: dumpbin, llvm-readobj

### 🔄 **Interoperabilidad**
- Enlazado con bibliotecas del sistema
- Excepciones cruzadas con MSVC
- Virtual dispatch mixto
- Debugging con PDB

## Requisitos del Sistema

### 📋 **Dependencias Obligatorias**
- **CMake** 3.20+
- **Visual Studio 2022** (MSVC) o **GCC/Clang** con C++20
- **Windows 10/11** x64
- **Windows SDK** 10.0.19041.0+

### 🔗 **Dependencias Opcionales (Recomendadas)**
- **LLVM** 15.0+ con Clang (para desarrollo avanzado)
- **LLD** (incluido con LLVM)
- **Ninja** (para construcción más rápida)

## Instalación y Construcción

### 🚀 **Construcción Rápida**

#### Windows (PowerShell/Command Prompt)
```batch
# Clonar y construir
git clone <repository>
cd compiler-cpp

# Construir automáticamente
.\build.bat
```

#### Linux/Mac (Terminal)
```bash
# Clonar y construir
git clone <repository>
cd compiler-cpp

# Hacer ejecutable el script
chmod +x build.sh

# Construir
./build.sh
```

### 🔧 **Construcción Manual**

```bash
# Crear directorio de build
mkdir build && cd build

# Configurar con CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Construir
make -j$(nproc)

# Instalar (opcional)
make install
```

### ⚙️ **Opciones de CMake**

```bash
# Configuración completa
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCPP20_COMPILER_BUILD_TESTS=ON \
    -DCPP20_COMPILER_ENABLE_LTO=ON \
    -DCPP20_COMPILER_USE_LLVM=ON \
    -DCPP20_COMPILER_ENABLE_MODULES=ON \
    -DCPP20_COMPILER_ENABLE_COROUTINES=ON
```

## Uso

### 📖 **Sintaxis Básica**
```bash
# Compilar a ejecutable
cpp20-compiler hello.cpp -o hello.exe

# Solo compilar (generar .obj)
cpp20-compiler -c source.cpp

# Preprocesar únicamente
cpp20-compiler -E source.cpp

# Generar ensamblador
cpp20-compiler -S source.cpp

# Con optimizaciones
cpp20-compiler -O2 -g source.cpp

# Verbose
cpp20-compiler -v --std=c++20 source.cpp
```

### 🎯 **Ejemplo Completo**

```cpp
// hello.cpp
#include <iostream>

int main() {
    std::cout << "¡Hola desde C++20!" << std::endl;
    return 0;
}
```

```bash
# Compilar y ejecutar
cpp20-compiler hello.cpp -o hello.exe
./hello.exe
```

## Plan de Desarrollo

### 📅 **Fases de Bootstrap**

#### **Etapa A** ✅ **Completada**
- ✅ Framework base del compilador
- ✅ Estructura modular CMake
- ✅ Sistema de diagnósticos completo
- ✅ Driver básico funcional

#### **Etapa B** 🔄 **En Progreso**
- 🔄 Front-end completo (lexer, parser, semantic)
- 🔄 Sistema de tipos y símbolos
- 🔄 Integración LLVM básica
- 🔄 Soporte para plantillas básico

#### **Etapa C** ⏳ **Pendiente**
- ⏳ Auto-compilación completa
- ⏳ Features C++20 avanzadas
- ⏳ Optimizaciones y polimento

### 🎯 **Hitos Clave**

1. **"Hello World" C básico** ✅
2. **C++ con clases/herencia** 🔄
3. **Templates complejos** ⏳
4. **Constexpr avanzado** ⏳
5. **Módulos C++20** ⏳
6. **Corrutinas funcionales** ⏳

## Estructura del Proyecto

```
compiler-cpp/
├── src/                    # Código fuente
│   ├── common/            # Utilidades comunes
│   ├── types/             # Sistema de tipos
│   ├── symbols/           # Tabla de símbolos
│   ├── ast/               # Abstract Syntax Tree
│   ├── frontend/          # Front-end (lexer, parser, semantic)
│   ├── backend/           # Back-end LLVM
│   ├── driver/            # CLI del compilador
│   └── [otros módulos]
├── include/               # Headers públicos
├── tests/                 # Sistema de pruebas
├── examples/              # Ejemplos de uso
├── docs/                  # Documentación
├── cmake/                 # Módulos CMake
├── third_party/           # Dependencias externas
├── CMakeLists.txt         # Configuración principal
├── build.bat/.sh          # Scripts de construcción
└── implementation-plan.mdc # Plan detallado
```

## Contribución

### 🤝 **Cómo Contribuir**

1. **Fork** el proyecto
2. **Crea** una rama feature: `git checkout -b feature/nueva-funcionalidad`
3. **Implementa** siguiendo el `implementation-plan.mdc`
4. **Prueba** exhaustivamente
5. **Commit**: `git commit -m 'feat: descripción clara'`
6. **Push**: `git push origin feature/nueva-funcionalidad`
7. **Pull Request** con descripción detallada

### 📋 **Estándares de Código**
- **C++20** como estándar base
- **CamelCase** para tipos, **snake_case** para funciones/variables
- **RAII** y smart pointers
- **Documentación** Doxygen en headers públicos
- **Tests** obligatorios para nueva funcionalidad

### 🐛 **Reportar Issues**
- Usa templates específicos para bugs/features
- Incluye **MWE** (Minimal Working Example)
- Especifica **versión del compilador** y **sistema operativo**
- Adjunta **logs** de construcción/ejecución

## Licencia

Este proyecto está bajo la **Licencia MIT**. Ver [LICENSE](LICENSE) para detalles.

## Estado del Proyecto

- **Versión**: 0.1.0 (Pre-alpha)
- **Estabilidad**: Experimental
- **Cobertura**: ~30% de features C++17
- **Plataformas**: Windows x64 (primaria)
- **CI/CD**: En desarrollo

---

**Desarrollado con ❤️ para la comunidad C++**

Para más detalles técnicos, ver [`implementation-plan.mdc`](implementation-plan.mdc)
