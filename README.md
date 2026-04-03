# FBX Toolkit

Command-line tools for FBX file processing and querying.

## Usage

```bash
# Query resources (read-only, fast)
fbx-toolkit resources <file.fbx/resource[/path]>

# Process/modify files (write operations)
fbx-toolkit tools <tool-name> <args>
```

## Resources (Query FBX Data)

Available: `animations` `blendshapes` `cameras` `deformers` `lights` `materials` `media` `meshes` `nodes` `poses` `scene` `skeleton` `textures`

```bash
# Scene metadata
fbx-toolkit resources file.fbx/scene

# List all meshes
fbx-toolkit resources file.fbx/meshes

# Mesh details with FBX unique IDs
fbx-toolkit resources file.fbx/meshes/MeshName

# Extract embedded texture binary
fbx-toolkit resources file.fbx/media/TextureName/data > output.png

# Progressive animation loading
fbx-toolkit resources file.fbx/animations              # Fast list (header-only)
fbx-toolkit resources file.fbx/animations/AnimName     # Single anim metadata  
fbx-toolkit resources file.fbx/animations/AnimName/**  # Full curve data
```

## Tools (Modify FBX Files)

- **bone-reset** - Reset bone rotations, fix hand/hip positions
- **axis-mender** - Retarget animations with coordinate system conversion
- **split-skeleton** - Split multi-skeleton FBX into separate files

```bash
fbx-toolkit tools bone-reset <input.fbx> <output.fbx>
fbx-toolkit tools split-skeleton <input.fbx> <output_dir>
```

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Binary: `build/bin/Release/fbx-toolkit.exe`

## Key Features

- **Progressive loading** - Query metadata without full scene import
- **Raw FBX access** - Exposes FBX unique IDs for cross-referencing
- **Computed poses** - Generates bind poses from skin deformers when not stored
- **Media extraction** - Dumps embedded textures to system temp cache
- **Modular architecture** - Auto-registered resource handlers

See [ARCHITECTURE.md](ARCHITECTURE.md) and [ROADMAP.md](ROADMAP.md).
