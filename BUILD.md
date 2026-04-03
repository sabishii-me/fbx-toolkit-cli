# Build Instructions

## Quick Start

### Windows
```powershell
.\extract_and_build.ps1
```

Requires: 7-Zip (https://www.7-zip.org/)

### Linux/macOS
```bash
chmod +x build.sh
./build.sh
```

## What Happens

1. **Auto-extracts** FBX SDK from `third_party/` (embedded installers)
2. **Configures** CMake with extracted SDK
3. **Builds** the fbx-toolkit executable
4. **Tests** the output

## CI/CD

GitHub Actions automatically:
- Extracts FBX SDK on all platforms (Windows, Linux, macOS)
- Builds binaries
- Uploads artifacts
- Creates releases on tags

No manual SDK installation needed!

## Manual SDK Path

If you have FBX SDK installed elsewhere:

```bash
cmake -B build -DUSE_SYSTEM_FBX_SDK=ON -DFBX_SDK_ROOT="/your/path" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Output

- Windows: `build/bin/Release/fbx-toolkit.exe`
- Linux/macOS: `build/bin/fbx-toolkit`
