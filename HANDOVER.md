# FBX Toolkit - Project Handover

## ⚠️ IMPORTANT: Context Preservation Document
**This document exists because we're about to move files and lose conversation context.**  
**Read this completely before continuing work.**

---

## 🚀 Quick Start Checklist (Resume Work Here)

When you come back to this project in a new conversation:

- [ ] 1. **First Build Test**
  ```bash
  cd build
  cmake ..
  cmake --build . --config Release
  ```
  
- [ ] 2. **Fix Compilation Errors** (if any)
  - Check includes in main.cpp
  - Verify command class names match
  
- [ ] 3. **Test Runtime**
  ```bash
  ./bin/fbx-toolkit               # Should list commands
  ./bin/fbx-toolkit list          # Should output JSON
  ./bin/fbx-toolkit bone-reset test.fbx  # Test with real file
  ```
  
- [ ] 4. **Clean Up Old Files**
  - Archive FBXBoneReset/ and FBXAxisMender/
  - Delete generated VS files (*.sln, *.vcxproj)
  
- [ ] 5. **GitHub Prep**
  - Commit new structure
  - Set up GitHub Actions (see sections below)
  
**Current Status**: Step 1 not done yet ⚠️

---

## Project Status: ✅ Restructured & Ready for CI/CD

### What We Did (Conversation Summary)
Reorganized a collection of FBX tools into a unified CLI toolkit with:
- Clean modular architecture
- Easy command addition workflow
- MCP-ready JSON output
- Preparation for GitHub Actions builds

**Original Problem**: Project was messy, had two separate tools in FBXBoneReset/ and FBXAxisMender/ directories, hard to maintain and extend.

**Solution**: Created a unified `fbx-toolkit` CLI with command pattern, making it easy to add new FBX processing commands in the future.

### Project Structure

```
OptitrackFBXPostprocessing/
├── src/
│   ├── main.cpp                    # CLI dispatcher & command registry
│   └── commands/
│       ├── command.h               # Base interface for all commands
│       ├── bone_reset.cpp          # Optitrack bone fix tool
│       └── axis_mender.cpp         # FBX retargeting tool
├── FBXBoneReset/                   # OLD - can be archived
│   └── main.cpp                    # Original implementation
├── FBXAxisMender/                  # OLD - can be archived  
│   └── main.cpp                    # Original implementation
├── CMakeLists.txt                  # ✅ Updated for new structure
├── fbxsdk.cmake                    # FBX SDK configuration
├── README.md                       # User documentation
└── HANDOVER.md                     # This file

build/                              # Build artifacts (gitignore)
```

### Current Commands

#### 1. bone-reset
**Purpose**: Fix Optitrack FBX exports for game engines
- Resets all bone rotations to zero on first frame
- Fixes thumb rotations (45° for natural pose)
- Centers hip X/Z translation
- Generates timestamped reports

**Usage**: `fbx-toolkit bone-reset <file_or_directory>`

#### 2. axis-mender
**Purpose**: Retarget animations with coordinate system conversion
- Applies -90° X-axis rotation for coordinate remapping
- Preserves animation timing and hierarchy
- Outputs to `*_retargeted.fbx`

**Usage**: `fbx-toolkit axis-mender <input_file.fbx>`

### How to Build Locally

```bash
# 1. Ensure FBX SDK is installed and fbxsdk.cmake points to it
# 2. Generate build files
mkdir build
cd build
cmake ..

# 3. Build
cmake --build . --config Release

# 4. Output is in: build/bin/fbx-toolkit.exe
```

### Adding New Commands

**Steps**:
1. Create `src/commands/my_command.cpp`
2. Define a class inheriting from `Command` interface:
   ```cpp
   class MyCommand : public Command {
   public:
       const char* GetName() const override { return "my-command"; }
       const char* GetDescription() const override { return "Description"; }
       const char* GetUsage() const override { return "my-command <args>"; }
       int Execute(const std::vector<std::string>& args) override {
           // Implementation
           return 0;
       }
   };
   ```
3. Include in `src/main.cpp`: `#include "commands/my_command.cpp"`
4. Register in `GetCommands()`: `commands.emplace_back(new MyCommand());`

That's it! The command will automatically appear in help and list output.

### Critical Build Information

#### Current Build Status
⚠️ **NOT YET TESTED** - The refactoring is complete but hasn't been compiled yet.

**Why**: User interrupted the build command to create this handover first.

**Next Immediate Steps**:
1. Test build locally: `cd build && cmake --build . --config Release`
2. Fix any compilation errors (likely minor issues with includes/linking)
3. Test executables with sample FBX files
4. Then proceed with GitHub setup

#### Potential Issues to Watch For

1. **Factory Pattern Simplification**
   - Removed `extern "C"` factory functions
   - Commands are now directly instantiated in `GetCommands()`
   - If you get linker errors, check that command classes are properly included

2. **Include Order**
   - `main.cpp` includes command implementations directly
   - This is intentional to avoid complex linking
   - Each command is self-contained in one .cpp file

3. **Windows-Specific Code**
   - `freopen_s()` in bone_reset.cpp (line ~230)
   - `localtime_s()` in bone_reset.cpp (line ~16)
   - These will break on Linux/Mac - need `#ifdef _WIN32` guards

### File Location Changes Coming
**You mentioned moving files**, so note these dependencies:

**If you move the project**:
- `fbxsdk.cmake` - Update FBX SDK path references
- `.vscode/settings.json` - Update any absolute paths
- CMakeLists.txt - Uses relative paths, should be fine

**If you rename directories**:
- Update CMakeLists.txt line 63: `add_subdirectory()` calls
- Update #include paths in main.cpp if you move commands/

### Next Steps for GitHub CI/CD

#### 1. Clean Up Repository
- [ ] Archive old directories: `FBXBoneReset/`, `FBXAxisMender/`
- [ ] Add `.gitignore`:
  ```
  build/
  .vs/
  .vscode/
  *.user
  *.sln
  *.vcxproj*
  CMakeCache.txt
  CMakeFiles/
  cmake_install.cmake
  *.exe
  *.dll
  *.lib
  *.exp
  ```

#### 2. FBX SDK Dependency Strategy
**Challenge**: FBX SDK cannot be redistributed freely

**Options**:
- **A. Self-hosted runner** with FBX SDK pre-installed
- **B. Secure artifact** - Store SDK as private GitHub artifact
- **C. Download script** - Auto-download from Autodesk (requires account)
- **D. Docker image** - Private container with SDK baked in

**Recommendation**: Start with option B (private artifact) for fastest setup.

#### 3. GitHub Actions Workflow

Create `.github/workflows/build.yml`:

```yaml
name: Build FBX Toolkit

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  build-windows:
    runs-on: windows-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Setup FBX SDK
      # TODO: Implement based on chosen strategy above
      run: |
        # Download or restore FBX SDK
        
    - name: Configure CMake
      run: cmake -B build -DCMAKE_BUILD_TYPE=Release
      
    - name: Build
      run: cmake --build build --config Release
      
    - name: Upload Artifact
      uses: actions/upload-artifact@v3
      with:
        name: fbx-toolkit-windows
        path: build/bin/fbx-toolkit.exe
```

#### 4. Release Workflow

Create `.github/workflows/release.yml`:

```yaml
name: Release

on:
  push:
    tags:
      - 'v*'

jobs:
  release:
    runs-on: windows-latest
    steps:
    - uses: actions/checkout@v3
    
    # ... build steps ...
    
    - name: Create Release
      uses: softprops/action-gh-release@v1
      with:
        files: |
          build/bin/fbx-toolkit.exe
          README.md
```

#### 5. Version Management

Consider adding version to the code:
```cpp
// src/version.h
#define FBX_TOOLKIT_VERSION "1.0.0"
#define FBX_TOOLKIT_BUILD_DATE __DATE__
```

### Testing Strategy

**Current**: Manual testing
**Needed**: 
- [ ] Sample FBX files for automated testing
- [ ] Output validation scripts
- [ ] Regression test suite

**Test files could include**:
- Simple cube with animation
- Humanoid skeleton
- Multi-take FBX
- Malformed inputs

### Documentation Improvements

- [ ] Add example outputs/screenshots to README
- [ ] Create troubleshooting guide
- [ ] Document FBX SDK version compatibility
- [ ] Add command video demos

### Known Issues / TODOs

1. **Windows-specific code**: `freopen_s`, `localtime_s` in bone_reset.cpp
   - Consider cross-platform alternatives for Linux/Mac support
   
2. **Error handling**: Some FBX operations could fail silently
   - Add more validation and error messages
   
3. **Progress reporting**: Large batch operations have no progress indication
   - Consider adding progress bar for directory processing
   
4. **MCP integration**: JSON output exists but needs MCP server wrapper
   - Create MCP server definition file

### Architecture Decisions Made (For Future Reference)

#### Why Command Pattern?
- **Extensibility**: Drop in a new .cpp file, add one line to main.cpp, done
- **Discoverability**: All commands auto-list with `fbx-toolkit` or `fbx-toolkit list`
- **MCP-ready**: JSON output format matches MCP tool listing pattern
- **Self-documenting**: Each command has name, description, usage baked in

#### Why Not Separate Executables?
- **User friction**: One tool to install vs many
- **Code reuse**: Shared FBX utilities can be extracted later
- **Versioning**: Single version number for whole toolkit

#### Why Include .cpp Files in main.cpp?
- **Simplicity**: Avoids CMake complexity for small project
- **Speed**: No need for separate compilation units initially
- **Easy to refactor**: Can split into libraries later if needed

#### Code Organization Philosophy
- Each command is **completely self-contained** in one file
- No shared state between commands
- main.cpp is **just a dispatcher** - minimal logic
- Commands don't know about each other

### What's NOT Done Yet

❌ **Build testing** - Hasn't been compiled since refactor  
❌ **Runtime testing** - No test FBX files run through it  
❌ **Cross-platform** - Windows-only code still present  
❌ **Error handling** - Basic error messages, could be better  
❌ **Progress bars** - Batch operations are silent  
❌ **Unit tests** - None exist  
❌ **CI/CD** - No GitHub Actions yet  
❌ **Versioning** - No version number in code  

### Files to Archive/Delete After Verification

Once the new unified build works:
```
FBXBoneReset/          # Old source - archive
FBXAxisMender/         # Old source - archive
build/                 # CMake artifacts - delete
*.sln                  # VS solution files - delete (auto-generated)
*.vcxproj*             # VS project files - delete (auto-generated)
CMakeCache.txt         # Delete (auto-generated)
CMakeFiles/            # Delete (auto-generated)
cmake_install.cmake    # Delete (auto-generated)
ALL_BUILD.*            # Delete (auto-generated)
ZERO_CHECK.*           # Delete (auto-generated)
```

**How to archive**: Create `_archive/` folder and move old directories there, or just delete if committed to git.

### Contact & Handoff

**Current State**: Core refactoring complete ✅ | Build untested ⚠️  
**Next Owner**: Should test build first, then focus on CI/CD  
**Timeline**: ~1 hour build testing, ~2-4 hours for GitHub Actions  
**Blockers**: 
  1. Build verification needed immediately
  2. FBX SDK licensing/distribution strategy decision for CI

### Conversation Context Lost After This Point

**If you're reading this in a new conversation**:
- The refactoring was done in one session (2026-04-02)
- User wanted: "easy to maintain CLI that lists commands like MCP"
- User plans to: "pipe CLI to MCP" eventually
- Build philosophy: Keep it simple, add complexity only when needed
- Primary use case: Daily game dev FBX processing workflows

**Original tools purpose**:
- bone-reset: Fix Optitrack mocap exports for Unity/Unreal
- axis-mender: Coordinate system conversion between DCC tools

**Future expansion ideas mentioned**: More FBX commands for game dev workflows, but none specified yet

### Quick Reference Commands

```bash
# List all commands
fbx-toolkit

# JSON output for MCP
fbx-toolkit list

# Process single file
fbx-toolkit bone-reset path/to/file.fbx

# Process directory
fbx-toolkit bone-reset path/to/directory/

# Retarget animation
fbx-toolkit axis-mender input.fbx
```

---

**Last Updated**: 2026-04-02  
**Status**: Ready for CI/CD implementation  
**Architecture**: Stable and extensible
