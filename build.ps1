# Local build script for Windows
param(
    [string]$Config = "Release",
    [string]$FBXSDKPath = "C:/Program Files/Autodesk/FBX/FBX SDK/2020.3.7"
)

Write-Host "=== FBX Toolkit Build Script ===" -ForegroundColor Cyan
Write-Host ""

# Check if FBX SDK exists
if (-not (Test-Path "$FBXSDKPath/include/fbxsdk.h")) {
    Write-Host "ERROR: FBX SDK not found at: $FBXSDKPath" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please either:" -ForegroundColor Yellow
    Write-Host "  1. Install FBX SDK to the default location" -ForegroundColor Yellow
    Write-Host "  2. Specify the path: .\build.ps1 -FBXSDKPath 'C:\Your\Path'" -ForegroundColor Yellow
    Write-Host "  3. Set environment variable: `$env:FBX_SDK_ROOT = 'C:\Your\Path'" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Download from: https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2020-3-7" -ForegroundColor Cyan
    exit 1
}

Write-Host "✓ Found FBX SDK at: $FBXSDKPath" -ForegroundColor Green
Write-Host ""

# Create build directory
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Configure
Write-Host "Configuring CMake..." -ForegroundColor Cyan
cmake -B build -DUSE_SYSTEM_FBX_SDK=ON -DFBX_SDK_ROOT="$FBXSDKPath" -DCMAKE_BUILD_TYPE=$Config

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: CMake configuration failed!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "✓ Configuration successful" -ForegroundColor Green
Write-Host ""

# Build
Write-Host "Building project..." -ForegroundColor Cyan
cmake --build build --config $Config

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "✓ Build successful!" -ForegroundColor Green
Write-Host ""

# Check output
$exePath = "build\bin\$Config\fbx-toolkit.exe"
if (Test-Path $exePath) {
    Write-Host "Executable: $exePath" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Testing executable..." -ForegroundColor Cyan
    & $exePath
    Write-Host ""
    Write-Host "=== Build Complete ===" -ForegroundColor Green
} else {
    Write-Host "WARNING: Executable not found at expected location: $exePath" -ForegroundColor Yellow
    Write-Host "Searching for executable..." -ForegroundColor Yellow
    Get-ChildItem "build" -Recurse -Filter "fbx-toolkit.exe" | ForEach-Object {
        Write-Host "Found: $($_.FullName)" -ForegroundColor Cyan
    }
}
