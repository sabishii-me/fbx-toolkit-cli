# FBX Toolkit CLI

Command-line tool for querying and modifying FBX files. Designed for AI agents, automation, and programmatic access.

## For AI Agents: How to Use This Tool

This CLI provides **read-only queries** (`resources`) and **write operations** (`tools`).

### Command Structure

```
fbx-toolkit resources <file.fbx/resource[/path]>
fbx-toolkit tools <tool-name> <args>
```

All resource queries return **JSON** to stdout. Parse it to extract data.

---

## Resources: Query FBX Data

**Pattern**: `file.fbx/<resource>[/<name>][/**]`

### Available Resources

| Resource | Description | Example Path |
|----------|-------------|--------------|
| `scene` | Scene metadata, units, coordinate system | `file.fbx/scene` |
| `nodes` | Scene graph hierarchy with transforms | `file.fbx/nodes` |
| `skeleton` | Bone hierarchy with parent-child relationships | `file.fbx/skeleton` |
| `meshes` | Geometry: vertices, polygons, normals, UVs | `file.fbx/meshes` |
| `materials` | Material properties and parameters | `file.fbx/materials` |
| `textures` | Texture references and properties | `file.fbx/textures` |
| `media` | Embedded texture data (PNG, JPG, etc.) | `file.fbx/media` |
| `animations` | Animation takes, curves, keyframes | `file.fbx/animations` |
| `poses` | Bind poses (computed if not stored) | `file.fbx/poses` |
| `deformers` | Skin and blend shape deformers | `file.fbx/deformers` |
| `blendshapes` | Morph targets/shape keys | `file.fbx/blendshapes` |
| `cameras` | Camera properties and transforms | `file.fbx/cameras` |
| `lights` | Light types, colors, intensities | `file.fbx/lights` |

### Progressive Loading Pattern

Query in stages to avoid loading unnecessary data:

```bash
# 1. List all (fast, header-only)
fbx-toolkit resources character.fbx/animations

# 2. Get metadata for specific item (medium, selective loading)
fbx-toolkit resources character.fbx/animations/WalkCycle

# 3. Get full data (slow, loads all curve data)
fbx-toolkit resources character.fbx/animations/WalkCycle/**
```

### Cross-Referencing with FBX Unique IDs

All resources expose `fbx_unique_id` fields. Use these to link data across queries:

```bash
# Get mesh with material reference
fbx-toolkit resources file.fbx/meshes/Head
# Output: {"material_id": 12345, ...}

# Look up material by ID
fbx-toolkit resources file.fbx/materials
# Find material with "fbx_unique_id": 12345
```

### Extracting Binary Data

Media resources support `/data` suffix to output raw binary:

```bash
# Extract embedded texture
fbx-toolkit resources character.fbx/media/DiffuseTexture/data > texture.png
```

**Important**: Output is binary (PNG/JPG). Do not parse as JSON.

---

## Tools: Modify FBX Files

### bone-reset

Fixes Optitrack motion capture exports by resetting bone rotations and adjusting hand/hip positions.

```bash
fbx-toolkit tools bone-reset input.fbx output.fbx
```

**Use when**: Optitrack FBX has incorrect bone orientations.

### axis-mender

Converts coordinate systems for animation retargeting (e.g., Y-up to Z-up).

```bash
fbx-toolkit tools axis-mender input.fbx output.fbx
```

**Use when**: Animations need coordinate system conversion.

### split-skeleton

Splits FBX files with multiple skeletons into separate files (one per root bone).

```bash
fbx-toolkit tools split-skeleton input.fbx output_directory/
```

**Use when**: FBX contains multiple characters that need to be separated.

---

## Learning the CLI

### 1. List all commands
```bash
fbx-toolkit
```

### 2. Explore a file's resources
```bash
# Start with scene info
fbx-toolkit resources file.fbx/scene

# List available resources
fbx-toolkit resources file.fbx/meshes
fbx-toolkit resources file.fbx/materials
fbx-toolkit resources file.fbx/animations
```

### 3. Drill down into specific items
```bash
# Get details for specific mesh
fbx-toolkit resources file.fbx/meshes/HeadMesh

# Get animation metadata
fbx-toolkit resources file.fbx/animations/RunCycle
```

### 4. Parse JSON output
```bash
# Use jq for parsing (recommended)
fbx-toolkit resources file.fbx/skeleton | jq '.bones[] | .name'

# Or parse with your language's JSON library
```

---

## JSON Output Format

All resources return this structure:

```json
{
  "resource_type": "meshes",
  "format_version": "1.0",
  "data": { ... }
}
```

- `resource_type`: Identifies the resource kind
- `format_version`: Schema version (currently "1.0")
- `data`: Resource-specific payload

---

## Key Concepts for AI Agents

### Progressive Loading
Query incrementally to minimize load time:
- **List**: Fast overview (e.g., `/animations`)
- **Metadata**: Single item details (e.g., `/animations/Walk`)
- **Full data**: Complete details (e.g., `/animations/Walk/**`)

### Raw FBX Data Access
This tool exposes **FBX SDK internals**, not converted formats:
- `fbx_unique_id`: Cross-reference resources
- `mapping_mode`: How data maps to geometry (ByControlPoint, ByPolygonVertex, etc.)
- `reference_mode`: How indices work (Direct, Index, IndexToDirect)

Use these for **accurate FBX manipulation**, not for direct rendering (use glTF/OBJ converters for that).

### Computed Data
Some resources are computed when missing:
- **Bind poses**: Extracted from skin deformers if not explicitly stored
- **Media paths**: Extracted to `%TEMP%/fbx_toolkit_cache` (cached between runs)

---

## Building from Source

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Binary location: `build/bin/Release/fbx-toolkit.exe` (Windows) or `build/bin/fbx-toolkit` (Linux/macOS)

---

## Technical Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md) - Design principles, file structure
- [ROADMAP.md](ROADMAP.md) - Planned features and future work

---

## License

Autodesk FBX SDK (proprietary, free for development)
