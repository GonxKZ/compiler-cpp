# Scripts de Build y Automatización

Esta carpeta contiene scripts para automatizar tareas comunes de desarrollo y build.

## 📋 Scripts Disponibles

### Windows (`build.bat`)
Script de build automatizado para Windows con Visual Studio:
- Configuración automática de CMake
- Build en modo Release y Debug
- Ejecución de tests integrada
- Limpieza de archivos generados

### Linux/macOS (`build.sh`)
Script equivalente para sistemas Unix:
- Configuración automática de CMake
- Build con Make o Ninja
- Ejecución de tests integrada
- Compatibilidad con GCC y Clang

## 🚀 **Uso Rápido**

### Windows
```batch
# Build completo
scripts\build.bat

# Build específico
scripts\build.bat release
scripts\build.bat debug
scripts\build.bat clean
```

### Linux/macOS
```bash
# Build completo
./scripts/build.sh

# Build específico
./scripts/build.sh release
./scripts/build.sh debug
./scripts/build.sh clean
```

## ⚙️ **Características**

- **Detección automática** del generador (Visual Studio, Make, Ninja)
- **Configuración optimizada** para cada plataforma
- **Validación integrada** con tests
- **Limpieza automática** de archivos temporales
- **Reportes detallados** de errores

## 📝 **Notas de Desarrollo**

Estos scripts están diseñados para:
- **Entornos de desarrollo** consistentes
- **CI/CD pipelines** automatizadas
- **Validación rápida** de cambios
- **Debugging eficiente** de problemas de build

Para desarrollo avanzado, usar CMake directamente:
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

---

**Estado**: ✅ Scripts funcionales y probados
