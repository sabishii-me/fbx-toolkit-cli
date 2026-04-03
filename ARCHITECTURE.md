# Architecture

## Design Principles

**Resources vs Tools** - Read-only queries (resources) separated from write operations (tools).

**Progressive Loading** - Query metadata without full scene import via `LoadStrategy`:
- `HEADER_ONLY` - Animation takes without curve data (86x faster)
- `SELECTIVE` - Only required scene objects
- `MINIMAL` - Basic scene structure

**Raw FBX Access** - Expose FBX unique IDs and internal structure for cross-referencing:
- Node IDs, mesh IDs, material IDs link across resources
- Mapping modes (ByControlPoint, ByPolygonVertex) and reference modes (Direct, Index)
- Element layer details for UV sets, normals, colors

**Modular Handlers** - Auto-registered resource handlers via `REGISTER_RESOURCE_HANDLER()` macro.

## File Structure

```
src/
├── main.cpp                          # Command dispatcher
├── commands/
│   ├── command.h                     # Command interface
│   ├── resources.cpp                 # Resources command with FBX loader
│   ├── tools.cpp                     # Tools dispatcher
│   ├── bone_reset.cpp                # Bone transform fixes
│   ├── axis_mender.cpp               # Coordinate system conversion
│   ├── split_skeleton.cpp            # Multi-skeleton splitter
│   └── resources/
│       ├── resource_handler.h        # Handler interface + LoadStrategy enum
│       ├── resource_registry.h       # Auto-registration system
│       ├── animations_handler.cpp    # Progressive animation loading
│       ├── blendshapes_handler.cpp   # Morph targets
│       ├── cameras_handler.cpp       # Scene cameras
│       ├── deformers_handler.cpp     # Skin + blend shape deformers
│       ├── lights_handler.cpp        # Scene lights
│       ├── materials_handler.cpp     # Material properties
│       ├── media_handler.cpp         # Embedded texture extraction
│       ├── meshes_handler.cpp        # Mesh geometry + element details
│       ├── nodes_handler.cpp         # Scene graph
│       ├── poses_handler.cpp         # Bind poses (with computed fallback)
│       ├── scene_handler.cpp         # Scene metadata
│       ├── skeleton_handler.cpp      # Bone hierarchy
│       └── textures_handler.cpp      # Texture references
└── commands/fbx/
    ├── fbx_loader.h                  # FBX SDK wrapper
    └── fbx_loader.cpp

```

## Key Implementation Details

**Computed Bind Poses** - Extract from skin deformers when not stored explicitly (common in Blender/Unity exports).

**Binary Output** - Windows `_setmode(_fileno(stdout), _O_BINARY)` prevents CRLF corruption for media extraction.

**Temp Cache** - `%TEMP%/fbx_toolkit_cache` stores extracted embedded textures (avoid re-extraction).

See code for implementation details.
