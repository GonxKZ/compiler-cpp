# PowerShell script for build validation
param(
    [string]$BuildConfig = "Release",
    [string]$BuildDir = "build"
)

Write-Host "🔍 Starting Build Validation..." -ForegroundColor Green
Write-Host "Configuration: $BuildConfig" -ForegroundColor Yellow
Write-Host "Build Directory: $BuildDir" -ForegroundColor Yellow

# Function to check if file exists
function Test-FileExists {
    param([string]$Path, [string]$Description)
    if (Test-Path $Path) {
        Write-Host "✓ $Description found: $Path" -ForegroundColor Green
        return $true
    } else {
        Write-Host "✗ $Description missing: $Path" -ForegroundColor Red
        return $false
    }
}

# Function to run command and check result
function Test-Command {
    param([string]$Command, [string]$Description)
    Write-Host "Running: $Description" -ForegroundColor Cyan
    try {
        $result = Invoke-Expression $Command
        if ($LASTEXITCODE -eq 0) {
            Write-Host "✓ $Description completed successfully" -ForegroundColor Green
            return $true
        } else {
            Write-Host "✗ $Description failed with exit code $LASTEXITCODE" -ForegroundColor Red
            return $false
        }
    } catch {
        Write-Host "✗ $Description failed: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    }
}

# Navigate to build directory
if (!(Test-Path $BuildDir)) {
    Write-Host "✗ Build directory does not exist: $BuildDir" -ForegroundColor Red
    exit 1
}

Push-Location $BuildDir

# Check essential files
$allTestsPass = $true

# Check compiler executable
$compilerPath = "bin\$BuildConfig\cpp20-compiler.exe"
$allTestsPass = $allTestsPass -and (Test-FileExists $compilerPath "Compiler executable")

# Check library files
$libFiles = @(
    "src\common\$BuildConfig\cpp20-compiler-common.lib",
    "src\types\$BuildConfig\cpp20-compiler-types.lib",
    "src\symbols\$BuildConfig\cpp20-compiler-symbols.lib",
    "src\ast\$BuildConfig\cpp20-compiler-ast.lib"
)

foreach ($libFile in $libFiles) {
    $allTestsPass = $allTestsPass -and (Test-FileExists $libFile "Library file")
}

# Test examples compilation
$allTestsPass = $allTestsPass -and (Test-Command "cmake --build . --config $BuildConfig --target hello-world" "Hello World example build")

$allTestsPass = $allTestsPass -and (Test-Command "cmake --build . --config $BuildConfig --target hello-coff" "COFF example build")

# Create validation report
$reportPath = "build_validation_report.txt"
$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

$report = @"
BUILD VALIDATION REPORT
======================
Timestamp: $timestamp
Configuration: $BuildConfig
Build Directory: $BuildDir

VALIDATION RESULTS:
"@

# Add file checks to report
$report += "`nFILE CHECKS:`n"
$report += "- Compiler executable: $(if (Test-Path $compilerPath) { 'PASS' } else { 'FAIL' })`n"

foreach ($libFile in $libFiles) {
    $fileName = Split-Path $libFile -Leaf
    $report += "- $fileName`: $(if (Test-Path $libFile) { 'PASS' } else { 'FAIL' })`n"
}

# Add build tests to report
$report += "`nBUILD TESTS:`n"
$report += "- Hello World example: $(if ($helloWorldExists) { 'PASS' } else { 'FAIL' })`n"
$report += "- Hello COFF example: $(if ($helloCoffExists) { 'PASS' } else { 'FAIL' })`n"

# Overall result
$report += "`nOVERALL RESULT: $(if ($allTestsPass) { 'ALL TESTS PASSED' } else { 'SOME TESTS FAILED' })`n"

# Save report
$report | Out-File -FilePath $reportPath -Encoding UTF8

Write-Host "`n📄 Validation report saved to: $reportPath" -ForegroundColor Cyan

Pop-Location

# Final result
if ($allTestsPass) {
    Write-Host "`n🎉 ALL VALIDATION TESTS PASSED!" -ForegroundColor Green
    exit 0
} else {
    Write-Host "`n❌ SOME VALIDATION TESTS FAILED!" -ForegroundColor Red
    exit 1
}
