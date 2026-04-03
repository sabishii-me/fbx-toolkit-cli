# Implementation Notes & Context

## Current Session Status (2026-04-02)

### ✅ What's Working

1. **Resources/Tools Architecture**
   - `resources` command - URI-based read-only inspection
   - `tools` command - Write operations dispatcher
   - Clean separation of concerns

2. **Safe Resource Queries**
   - Default: Brief output (~200 bytes, agent-safe)
   - Full tree: `skeleton/**` with warning (38KB+)
   - Grep-able: Works with `| grep`, `| jq`
   - Absolute paths: Works on Windows/Linux/Mac

3. **Resource Handlers**
   - `scene` - Scene metadata
   - `nodes` - Scene graph
   - `skeleton` - Bone hierarchy (tree structure)
   - `meshes` - Geometry info
   - `materials` - Material list
   - `animations` - Animation data

### ❌ What's Broken

1. **split-skeleton tool** (src/commands/split_skeleton.cpp)
   - **Problem**: Uses `SetSelected()` + export, which doesn't work
   - **Result**: Copies entire file 3 times (281MB → 843MB total)
   - **Why**: FBX SDK exporter ignores `SetSelected()` flags
   - **Status**: Needs complete rewrite using scene cloning

---

## Key Learnings from FBX SDK Documentation

### FBX SDK Export Patterns

**❌ WRONG APPROACH (current split_skeleton.cpp):**
```cpp
node->SetSelected(true);  // This is for DCC UI only!
exporter->Export(scene);   // Exports EVERYTHING, ignores selection
```

**✅ CORRECT APPROACH (see axis_mender.cpp):**
```cpp
// 1. Create NEW empty scene
FbxScene* newScene = FbxScene::Create(sdkManager, "filtered");

// 2. Clone ONLY wanted nodes to new scene
CloneNode(skelRoot, newScene, boneMap);
CloneMeshWithSkin(mesh, newScene, boneMap); // Remap skin clusters!
CopyMaterials(mesh, newScene);
CopyAnimations(animStack, newScene, boneMap);

// 3. Export the NEW scene
exporter->Export(newScene);
```

### Critical Requirements for Split-Skeleton

1. **Scene Cloning** - Must create new scene, not filter export
2. **Bone Mapping** - `std::map<std::string, FbxNode*>` old→new bones
3. **Skin Remapping** - Relink FbxCluster to NEW bone nodes
4. **Material Copying** - Clone materials/textures referenced by meshes
5. **Animation Baking** - Sample keyframes, remap to new bones

### Reference Implementation

`axis_mender.cpp` already does this correctly:
- Line 23-45: `CreateTargetSkeleton()` - scene cloning
- Line 155-195: Animation baking pattern
- This is the template to follow!

---

## Real-World Test Results

### Test File: "Teller Audition 2.fbx"
- **Size**: 281MB
- **Skeletons**: 3 (all named "Hips")
- **Total bones**: 192 (64 per skeleton)
- **Meshes**: 0 (pure mocap data)
- **Animations**: Large animation curves

### Query Results

**Brief (default - safe for agents):**
```bash
fbx-toolkit resources file.fbx/skeleton
→ {"skeleton_count": 3, "total_bones": 192, "skeletons": ["Hips", "Hips", "Hips"]}
```

**Full tree (with warning):**
```bash
fbx-toolkit resources file.fbx/skeleton/**
→ Warning: Large output (38347 bytes). Consider using grep or -o file.json
→ [nested tree with all 192 bones]
```

**Grep usage:**
```bash
fbx-toolkit resources file.fbx/skeleton/** 2>/dev/null | grep -i "thumb"
→ Lists all thumb bones across 3 skeletons
```

---

## Architecture Decisions

### Progressive Disclosure Design

**Default = Brief** (agent-safe):
- Small payload (~200 bytes)
- Quick overview
- Prevents accidental 40KB reads

**Explicit flags for details**:
- `**` or `-r` for full tree
- `-t TRS` for transform data
- `-o file.json` for dump to file

### Tool Registry Problem

**Current**: Hardcoded in `tools.cpp`
```cpp
if (toolName == "bone-reset") ...
else if (toolName == "axis-mender") ...
else if (toolName == "split-skeleton") ...
// This doesn't scale to 50+ tools!
```

**TODO**: Auto-discovery pattern
- Plugin-like architecture
- Tools register themselves at startup
- No hardcoding in dispatcher

---

## Next Steps

### Immediate (This Commit)
1. Remove `split_skeleton.cpp` from build
2. Remove from `main.cpp` includes
3. Remove from `tools.cpp` dispatcher
4. Commit working: resources + tools architecture
5. Push to CI for cross-platform testing

### Future (Next Session)
1. Study `axis_mender.cpp` scene cloning thoroughly
2. Design proper split-skeleton with:
   - Scene cloning (not selection export)
   - Mesh+skin cloning with bone remapping
   - Material/texture copying
   - Animation baking/remapping
3. Implement tool auto-discovery/registry
4. Add more resource types (lights, cameras, constraints)

---

## Files Modified This Session

### New Files
- `src/commands/resources.cpp` - URI-based resource queries
- `src/commands/tools.cpp` - Tool dispatcher
- `src/commands/split_skeleton.cpp` - **BROKEN, needs removal**
- `RESOURCES.md` - Resource design reference
- `ARCHITECTURE.md` - System architecture guide
- `BUILD.md` - Build instructions

### Modified Files
- `src/main.cpp` - Updated for resources/tools pattern
- `CMakeLists.txt` - Fixed FBX SDK path detection
- `.github/workflows/build.yml` - CI with embedded FBX SDK
- `.gitignore` - Cleaned up

### Embedded Dependencies
- `third_party/fbx_win.exe` (115MB)
- `third_party/fbx_linux.tar.gz` (39MB)
- `third_party/fbx_mac.pkg.tgz` (92MB)

---

## Important Context for Next Session

### Don't Forget
1. **SetSelected() doesn't work** - FBX SDK limitation
2. **axis_mender.cpp is the reference** - Correct scene cloning pattern
3. **Mesh skinning is complex** - Must remap FbxCluster links to new bones
4. **Tool registry needs redesign** - Current hardcoding won't scale
5. **Progressive disclosure works well** - Keep brief defaults

### Quick Reference
- **Test file location**: `C:/Users/spjc0/Downloads/Teller Audition 2.fbx`
- **Build command**: `extract_and_build.ps1` or `cd build && cmake --build . --config Release`
- **Executable**: `build/bin/Release/fbx-toolkit.exe`

### FBX SDK Key APIs
- `FbxScene::Create()` - New empty scene
- `FbxNode::Create()` - Clone node
- `FbxSkin/FbxCluster` - Skin deformer (must remap!)
- `FbxExporter::Export()` - Exports entire scene (no selection filter)
