# Registry Pattern Refactoring

## Goal
Remove all hardcoded tool/resource lists and enable auto-registration pattern that scales to 50+ tools and 10+ resource handlers.

## Status: IN PROGRESS

### Completed
- ✅ Added `GetDescription()` and `GetUsageExample()` to ResourceHandler base class
- ✅ Updated `resources.cpp` to auto-generate help from registered handlers
- ✅ Updated `scene_handler.cpp` as example implementation
- ✅ Created `tool.h` - base interface for all tools
- ✅ Created `tool_registry.h` - auto-registration system for tools

### In Progress
- 🔄 Update remaining resource handlers (12 of 13 remaining):
  - animations_handler.cpp
  - blendshapes_handler.cpp  
  - cameras_handler.cpp
  - deformers_handler.cpp
  - lights_handler.cpp
  - materials_handler.cpp
  - media_handler.cpp
  - meshes_handler.cpp
  - nodes_handler.cpp
  - poses_handler.cpp
  - skeleton_handler.cpp
  - textures_handler.cpp

### Todo
- ⏳ Refactor existing tools to use Tool base class:
  - bone_reset.cpp → BoneResetTool + REGISTER_TOOL
  - axis_mender.cpp → AxisMenderTool + REGISTER_TOOL
  - split_skeleton.cpp → SplitSkeletonTool + REGISTER_TOOL
  - create.cpp → CreateTool + REGISTER_TOOL

- ⏳ Update `tools.cpp` to use ToolRegistry for auto-discovery
- ⏳ Remove hardcoded tool execution functions from `main.cpp`
- ⏳ Test complete system

## Resource Handler Metadata Template

```cpp
const char* GetResourceName() const override {
    return "resource_name";
}

const char* GetDescription() const override {
    return "Brief description of what this resource provides";
}

const char* GetUsageExample() const override {
    return "fbx-toolkit resources file.fbx/resource_name";
}
```

## Metadata for Each Handler

| Handler | Description | Example |
|---------|-------------|---------|
| animations | Animation takes, curves, keyframes | file.fbx/animations |
| blendshapes | Morph targets and shape keys | file.fbx/blendshapes |
| cameras | Camera properties and transforms | file.fbx/cameras |
| deformers | Skin and blend shape deformers | file.fbx/deformers |
| lights | Light types, colors, intensities | file.fbx/lights |
| materials | Material properties and parameters | file.fbx/materials |
| media | Embedded texture data (binary extraction) | file.fbx/media |
| meshes | Geometry vertices, polygons, normals, UVs | file.fbx/meshes |
| nodes | Scene graph hierarchy with transforms | file.fbx/nodes |
| poses | Bind poses (computed from skin if missing) | file.fbx/poses |
| scene | Scene metadata, units, coordinate system, statistics | file.fbx/scene |
| skeleton | Bone hierarchy with parent-child relationships | file.fbx/skeleton |
| textures | Texture references and properties | file.fbx/textures |

## Tool Metadata Template

```cpp
class MyTool : public Tool {
public:
    const char* GetName() const override { return "tool-name"; }
    const char* GetDescription() const override { return "What this tool does"; }
    const char* GetUsagePattern() const override { return "<input.fbx> <output.fbx>"; }
    const char* GetUsageExample() const override { return "fbx-toolkit tools tool-name input.fbx output.fbx"; }
    int Execute(const std::vector<std::string>& args) override { /* ... */ }
};
REGISTER_TOOL(MyTool)
```

## Benefits After Completion

1. **Scalability**: Add new tool/resource = create file + use macro (1 step, not 5)
2. **Maintainability**: No hardcoded lists, metadata lives with implementation
3. **Auto-documentation**: Help text auto-generated from registered handlers
4. **Consistency**: All tools/resources follow same pattern
5. **Discoverability**: Easy to see what's available via registry queries

## Next Steps

Continue in next session - update remaining resource handlers with metadata, then tackle tool refactoring.
