# Roadmap

## Current (v0.1)

✅ 13 resource handlers (animations, blendshapes, cameras, deformers, lights, materials, media, meshes, nodes, poses, scene, skeleton, textures)
✅ Progressive loading (header-only → metadata → full data)
✅ Raw FBX data access (unique IDs, mapping modes, element details)
✅ 3 tools (bone-reset, axis-mender, split-skeleton)
✅ Embedded media extraction with temp cache
✅ Computed bind poses from skin deformers

## Future Work

### MCP CLI Framework
Reusable C++ library: CLI + MCP stdio + Host modes. Commands register once, work everywhere.

### Tool Registry Auto-Discovery
Static registration pattern (like resource handlers). No manual dispatcher edits.

### Session/Host Mode
Load FBX once, query many times via IPC (UDP/pipes/shared memory). Fast iteration, stateful operations.

### Additional Tools
merge-skeletons, retarget-animation, bake-animation, optimize-mesh, convert-units, validate, diff

### Additional Resources
constraints, custom-properties, thumbnails

### Advanced Features
Animation layering, curve editing, IK baking, UV optimization, LOD generation, streaming large files
