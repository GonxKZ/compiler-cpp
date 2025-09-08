# CI/CD Pipeline - Compilador C++20

Este directorio contiene la configuración de Integración Continua (CI) y Despliegue Continuo (CD) para el proyecto del compilador C++20.

## 🚀 Pipeline de CI/CD

### Activadores Automáticos

El pipeline se ejecuta automáticamente en:
- **Push** a las ramas `main`, `master`, o `develop`
- **Pull Requests** a las ramas principales
- Solo cuando cambian archivos relevantes (`.cpp`, `.hpp`, `.h`, `CMakeLists.txt`)

### Jobs del Pipeline

#### 1. `build-and-test-windows`
- **Plataforma**: Windows con Visual Studio 2022
- **Configuraciones**: Release y Debug
- **Funciones**:
  - Configura CMake con Visual Studio
  - Compila el proyecto completo
  - Ejecuta pruebas unitarias con CTest
  - Verifica que los ejecutables se generen correctamente
  - Compila ejemplos (`hello-world`, `hello-coff`)
  - Sube artefactos de build (Release only)

#### 2. `integration-test`
- **Dependencias**: Requiere que `build-and-test-windows` pase
- **Funciones**:
  - Pruebas de integración end-to-end
  - Genera reporte de resultados
  - Valida funcionalidad básica del compilador

#### 3. `deploy-artifacts` (Solo en main)
- **Condición**: Solo se ejecuta en push a `main`
- **Funciones**:
  - Crea archivo ZIP con el build
  - Sube artefactos de release
  - Prepara assets para distribución

## 🛠️ Scripts de Validación

### `build-validation.ps1`
Script de PowerShell que:
- Valida que todos los archivos necesarios existan
- Verifica que las compilaciones sean exitosas
- Genera reportes de validación
- Proporciona feedback detallado sobre fallos

### `run-full-validation.ps1`
Script maestro que ejecuta:
1. **Fase 1**: Validación de build
2. **Fase 2**: Tests del compilador
3. **Fase 3**: Tests unitarios
4. **Fase 4**: Validación de ejemplos
5. **Fase 5**: Reporte final consolidado

## 🎯 Uso Local

### Validación Manual

```powershell
# Validación completa
.\run-full-validation.ps1 -BuildConfig Release

# Solo build validation
.\build-validation.ps1 -BuildConfig Release

# Con opciones avanzadas
.\run-full-validation.ps1 -BuildConfig Debug -SkipExamples -Verbose
```

### Verificación de Resultados

Después de ejecutar la validación, revisa:
1. **ci_cd_validation_report.txt** - Reporte completo
2. **build/build_validation_report.txt** - Detalles del build
3. Artefactos en `build/bin/` y `build/lib/`

## ✅ Estados de Validación

### ✅ Éxito Completo
- Todos los archivos requeridos existen
- Compilación sin errores
- Tests pasan
- Ejemplos compilan y ejecutan

### ⚠️ Advertencias
- Algunos tests opcionales fallan
- Warnings en compilación (dentro de límites)

### ❌ Fallos Críticos
- Compilación falla
- Archivos esenciales faltan
- Tests críticos fallan

## 🔧 Solución de Problemas

### Errores Comunes

1. **MSVC no encontrado**
   ```
   Solución: Verificar que Visual Studio esté instalado correctamente
   ```

2. **CMake falla**
   ```
   Solución: Verificar versión de CMake (mínimo 3.20)
   ```

3. **Tests fallan**
   ```
   Solución: Revisar logs detallados en build/Testing/Temporary/
   ```

4. **Artefactos faltan**
   ```
   Solución: Verificar permisos de escritura en directorio de build
   ```

### Logs y Debug

Para debug detallado:
```powershell
# Build con verbose
cmake --build . --config Release --verbose

# Tests con detalle
ctest --output-on-failure --verbose

# Validación completa con logs
.\run-full-validation.ps1 -Verbose
```

## 📊 Métricas de Calidad

El pipeline valida automáticamente:
- ✅ Cobertura de código (>70%)
- ✅ Tiempo máximo de compilación (<5 min)
- ✅ Número máximo de warnings (<50 total)
- ✅ Tests pasados (100% requerido)

## 🚀 Despliegue

### Automático (GitHub)
- Push a `main` dispara despliegue automático
- Artefactos disponibles en "Actions" -> "Artifacts"
- Release automático con versionado

### Manual
```powershell
# Crear release manual
# 1. Crear tag
git tag v1.0.0
git push origin v1.0.0

# 2. Usar artefactos del CI/CD
# Descargar desde GitHub Actions
```

## 📞 Soporte

Para problemas con CI/CD:
1. Revisar logs en GitHub Actions
2. Ejecutar validación local con `run-full-validation.ps1`
3. Verificar configuración en los scripts
4. Revisar esta documentación

---

**Nota**: Esta configuración está optimizada para el proyecto del compilador C++20 y puede requerir ajustes para otros proyectos.
