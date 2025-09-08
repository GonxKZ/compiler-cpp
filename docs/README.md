# Documentación del Compilador C++20

Esta carpeta contiene toda la documentación del proyecto del compilador C++20.

## 📋 Índice de Documentación

### 🏗️ **Planificación y Arquitectura**
- **[IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md)** - Plan completo de implementación por capas
- **[COMPILATION_ERROR_STRATEGY.md](COMPILATION_ERROR_STRATEGY.md)** - Estrategia para manejo de errores y warnings

### 🧪 **Testing y Validación**
- **[TESTING_REPORT.md](TESTING_REPORT.md)** - Reporte completo de testing
- **[TESTING_CONSTEXPR_REPORT.md](TESTING_CONSTEXPR_REPORT.md)** - Reporte específico de constexpr
- **[CAPA1_README.md](CAPA1_README.md)** - Documentación específica de Capa 1 (ABI/COFF)

### 🚀 **Características Específicas**
- **[README_coroutines.md](README_coroutines.md)** - Documentación del sistema de corrutinas C++20

## 📖 **Documentación Principal**

Para información general del proyecto, ver:
- **[../README.md](../README.md)** - README principal del proyecto

## 🏛️ **Estructura del Proyecto**

```
compiler-cpp/
├── docs/                    # 📚 Documentación consolidada
├── src/                     # 🔧 Código fuente por componentes
├── include/                 # 📋 Headers públicos
├── tests/                   # 🧪 Tests unitarios e integración
├── examples/                # 💡 Ejemplos de uso
├── scripts/                 # 🛠️ Scripts de build y automatización
├── cmake/                   # ⚙️ Configuración CMake
└── build/                   # 🏗️ Archivos de build (generados)
```

## 📚 **Documentación por Componente**

### Front-end
- **Lexer**: Análisis léxico con 8 fases de traducción
- **Preprocessor**: Preprocesamiento completo con macros
- **Parser**: Parser recursivo descendente C++20

### Back-end
- **ABI Contract**: Contrato binario x86_64-pc-windows-msvc
- **COFF Writer**: Generación de archivos objeto COFF
- **Frame Builder**: Gestión de stack frames y convenciones de llamada
- **Unwind System**: Manejo de excepciones y desenrollado de pila

### Infraestructura
- **Type System**: Sistema completo de tipos C++20
- **Symbol Table**: Gestión de símbolos y ámbitos
- **AST**: Árbol de sintaxis abstracta
- **Diagnostics**: Sistema de reporte de errores y warnings

## 🔗 **Enlaces Importantes**

- [Estándar C++20 (N4861)](https://eel.is/c++draft/)
- [Microsoft x64 Calling Convention](https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention)
- [PE/COFF Specification](https://docs.microsoft.com/en-us/windows/win32/debug/pe-format)

## 🤝 **Contribuir**

Para contribuir a la documentación:
1. Mantener formato consistente en Markdown
2. Usar encabezados descriptivos
3. Incluir ejemplos de código cuando sea relevante
4. Actualizar este índice cuando se agregue nueva documentación

---

**Última actualización**: Septiembre 2025
**Versión del compilador**: 1.0.0
**Estado**: ✅ Completado y funcional
