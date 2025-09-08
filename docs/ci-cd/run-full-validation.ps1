# Complete CI/CD validation script
param(
    [string]$BuildConfig = "Release",
    [switch]$SkipTests,
    [switch]$SkipExamples,
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"

Write-Host "🚀 Starting Complete CI/CD Validation..." -ForegroundColor Green
Write-Host "Configuration: $BuildConfig" -ForegroundColor Yellow
Write-Host "Skip Tests: $SkipTests" -ForegroundColor Yellow
Write-Host "Skip Examples: $SkipExamples" -ForegroundColor Yellow
Write-Host "Verbose: $Verbose" -ForegroundColor Yellow
Write-Host ""

# Function to run command with error handling
function Invoke-Step {
    param(
        [string]$Command,
        [string]$StepName,
        [switch]$Optional
    )

    Write-Host "📋 $StepName..." -ForegroundColor Cyan

    try {
        if ($Verbose) {
            Write-Host "Command: $Command" -ForegroundColor Gray
        }

        $result = Invoke-Expression $Command

        if ($LASTEXITCODE -eq 0) {
            Write-Host "✅ $StepName completed successfully" -ForegroundColor Green
            return $true
        } else {
            Write-Host "❌ $StepName failed with exit code $LASTEXITCODE" -ForegroundColor Red
            if (!$Optional) {
                throw "$StepName failed"
            }
            return $false
        }
    } catch {
        Write-Host "❌ $StepName failed: $($_.Exception.Message)" -ForegroundColor Red
        if (!$Optional) {
            throw
        }
        return $false
    }
}

# Track overall success
$overallSuccess = $true
$validationResults = @{}

# Step 1: Build validation
try {
    Write-Host "`n🏗️  PHASE 1: Build Validation" -ForegroundColor Magenta
    Write-Host "=" * 50 -ForegroundColor Magenta

    $buildSuccess = Invoke-Step -Command ".\build-validation.ps1 -BuildConfig $BuildConfig" -StepName "Build Validation"
    $validationResults["build"] = $buildSuccess
    $overallSuccess = $overallSuccess -and $buildSuccess

} catch {
    Write-Host "❌ Build validation failed: $($_.Exception.Message)" -ForegroundColor Red
    $validationResults["build"] = $false
    $overallSuccess = $false
}

# Step 2: Compiler-specific tests
if ($overallSuccess -or !$SkipTests) {
    try {
        Write-Host "`n🧪 PHASE 2: Compiler Tests" -ForegroundColor Magenta
        Write-Host "=" * 50 -ForegroundColor Magenta

        $compilerTestSuccess = Invoke-Step -Command ".\compiler-tests.ps1" -StepName "Compiler Functionality Tests" -Optional:$SkipTests
        $validationResults["compiler_tests"] = $compilerTestSuccess
        if (!$SkipTests) {
            $overallSuccess = $overallSuccess -and $compilerTestSuccess
        }

    } catch {
        Write-Host "❌ Compiler tests failed: $($_.Exception.Message)" -ForegroundColor Red
        $validationResults["compiler_tests"] = $false
        if (!$SkipTests) {
            $overallSuccess = $false
        }
    }
}

# Step 3: Unit tests
if ($overallSuccess -or !$SkipTests) {
    try {
        Write-Host "`n🧪 PHASE 3: Unit Tests" -ForegroundColor Magenta
        Write-Host "=" * 50 -ForegroundColor Magenta

        # Navigate to build directory for tests
        Push-Location "build"

        $unitTestSuccess = Invoke-Step -Command "ctest --build-config $BuildConfig --output-on-failure $(if ($Verbose) { '--verbose' } else { '' })" -StepName "Unit Tests" -Optional:$SkipTests
        $validationResults["unit_tests"] = $unitTestSuccess
        if (!$SkipTests) {
            $overallSuccess = $overallSuccess -and $unitTestSuccess
        }

        Pop-Location

    } catch {
        Write-Host "❌ Unit tests failed: $($_.Exception.Message)" -ForegroundColor Red
        $validationResults["unit_tests"] = $false
        if (!$SkipTests) {
            $overallSuccess = $false
        }
    }
}

# Step 4: Examples validation
if ($overallSuccess -or !$SkipExamples) {
    try {
        Write-Host "`n🎯 PHASE 4: Examples Validation" -ForegroundColor Magenta
        Write-Host "=" * 50 -ForegroundColor Magenta

        # Navigate to build directory
        Push-Location "build"

        $examplesSuccess = $true

        # Test hello-world
        $helloWorldSuccess = Invoke-Step -Command "cmake --build . --config $BuildConfig --target hello-world" -StepName "Hello World Example" -Optional:$SkipExamples
        $examplesSuccess = $examplesSuccess -and $helloWorldSuccess

        # Test hello-coff
        $helloCoffSuccess = Invoke-Step -Command "cmake --build . --config $BuildConfig --target hello-coff" -StepName "Hello COFF Example" -Optional:$SkipExamples
        $examplesSuccess = $examplesSuccess -and $helloCoffSuccess

        $validationResults["examples"] = $examplesSuccess
        if (!$SkipExamples) {
            $overallSuccess = $overallSuccess -and $examplesSuccess
        }

        Pop-Location

    } catch {
        Write-Host "❌ Examples validation failed: $($_.Exception.Message)" -ForegroundColor Red
        $validationResults["examples"] = $false
        if (!$SkipExamples) {
            $overallSuccess = $false
        }
    }
}

# Step 5: Generate comprehensive report
Write-Host "`n📊 PHASE 5: Final Report" -ForegroundColor Magenta
Write-Host "=" * 50 -ForegroundColor Magenta

$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
$reportPath = "ci_cd_validation_report.txt"

$report = @"
COMPLETE CI/CD VALIDATION REPORT
================================
Timestamp: $timestamp
Configuration: $BuildConfig
Skip Tests: $SkipTests
Skip Examples: $SkipExamples

VALIDATION RESULTS:
"@

foreach ($phase in $validationResults.Keys) {
    $status = if ($validationResults[$phase]) { "PASSED" } else { "FAILED" }
    $icon = if ($validationResults[$phase]) { "✅" } else { "❌" }
    $report += "`n$icon $phase`: $status"
}

$report += @"


OVERALL RESULT: $(if ($overallSuccess) { "ALL VALIDATIONS PASSED" } else { "SOME VALIDATIONS FAILED" })

SUMMARY:
--------
Total Phases: $($validationResults.Count)
Passed: $(($validationResults.Values | Where-Object { $_ }).Count)
Failed: $(($validationResults.Values | Where-Object { -not $_ }).Count)

Generated Reports:
- build/ci-tests/compiler_test_report.txt
- build/ci-tests/build_validation_report.txt
- build/test_report.txt

Next Steps:
$(if ($overallSuccess) {
    "- Ready for deployment"
    "- Consider creating a release"
} else {
    "- Review failed phases"
    "- Check generated reports for details"
    "- Fix identified issues"
})
"@

$report | Out-File -FilePath $reportPath -Encoding UTF8

Write-Host "📄 Comprehensive report saved to: $reportPath" -ForegroundColor Cyan

# Display summary
Write-Host "`n📋 Validation Summary:" -ForegroundColor Yellow
foreach ($phase in $validationResults.Keys) {
    $color = if ($validationResults[$phase]) { "Green" } else { "Red" }
    $status = if ($validationResults[$phase]) { "PASSED" } else { "FAILED" }
    Write-Host "  $phase`: $status" -ForegroundColor $color
}

# Final result
if ($overallSuccess) {
    Write-Host "`n🎉 ALL CI/CD VALIDATIONS PASSED!" -ForegroundColor Green
    Write-Host "🚀 Ready for deployment" -ForegroundColor Cyan
    exit 0
} else {
    Write-Host "`n❌ SOME CI/CD VALIDATIONS FAILED!" -ForegroundColor Red
    Write-Host "🔧 Review the reports for details" -ForegroundColor Yellow
    exit 1
}
