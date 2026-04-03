# Extract FBX SDK and build locally
param(
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

Write-Host "=== FBX Toolkit - Extract & Build ===" -ForegroundColor Cyan
Write-Host ""

# Check if 7-Zip is available
$sevenZip = "C:\Program Files\7-Zip\7z.exe"
if (-not (Test-Path $sevenZip)) {
    Write-Host "ERROR: 7-Zip not found at: $sevenZip" -ForegroundColor Red
    Write-Host "Please install 7-Zip from https://www.7-zip.org/" -ForegroundColor Yellow
    exit 1
}

# Extract FBX SDK
$sdkPath = "C:\FBX_SDK_LOCAL"
if (-not (Test-Path "$sdkPath\include\fbxsdk.h")) {
    Write-Host "Extracting FBX SDK to $sdkPath..." -ForegroundColor Cyan
    & $sevenZip x third_party\fbx_win.exe -o"$sdkPath" -y | Out-Null

    if (Test-Path "$sdkPath\include\fbxsdk.h") {
        Write-Host "✓ FBX SDK extracted" -ForegroundColor Green
    } else {
        Write-Host "ERROR: Extraction failed" -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "✓ FBX SDK already extracted" -ForegroundColor Green
}

Write-Host ""

# Build
Write-Host "Configuring CMake..." -ForegroundColor Cyan
cmake -B build -DUSE_SYSTEM_FBX_SDK=ON -DFBX_SDK_ROOT="$sdkPath" -DCMAKE_BUILD_TYPE=$Config

if ($LASTEXITCODE -ne 0) { exit 1 }

Write-Host ""
Write-Host "Building..." -ForegroundColor Cyan
cmake --build build --config $Config

if ($LASTEXITCODE -ne 0) { exit 1 }

Write-Host ""
Write-Host "✓ Build complete!" -ForegroundColor Green
Write-Host ""

# Test
$exe = "build\bin\$Config\fbx-toolkit.exe"
if (Test-Path $exe) {
    Write-Host "Testing executable..." -ForegroundColor Cyan
    & $exe
    Write-Host ""
    Write-Host "=== Success ===" -ForegroundColor Green
    Write-Host "Executable: $exe" -ForegroundColor Cyan
} else {
    Write-Host "WARNING: Executable not found" -ForegroundColor Yellow
}
