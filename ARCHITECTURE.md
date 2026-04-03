# FBX Toolkit - Architecture

## Design: Resources + Tools

The toolkit separates **read-only inspection** (resources) from **write operations** (tools).

```
fbx-toolkit
├── resources <uri>     # Read resources (stateless, JSON output)
├── tools <name> [args] # Modify FBX files (operations)
└── list                # MCP integration (JSON)
```

---

## Resources (Read-Only)

**URI Pattern**: `file.fbx/resource[/sub-resource]`

### Available Resources

| Resource | Example | Output |
|----------|---------|--------|
| `scene` | `character.fbx/scene` | Scene metadata, units, coordinate system |
| `nodes` | `character.fbx/nodes` | All scene graph nodes with transforms |
| `skeleton` | `character.fbx/skeleton` | Complete bone hierarchy |
| `meshes` | `character.fbx/meshes` | Mesh list with vertex/polygon counts |
| `materials` | `character.fbx/materials` | Material definitions |
| `animations` | `character.fbx/animations` | Animation takes and timing |

### Usage

```bash
# Get scene info
fbx-toolkit resources character.fbx/scene

# Get skeleton
fbx-toolkit resources character.fbx/skeleton

# All output is JSON
fbx-toolkit resources character.fbx/scene | jq '.statistics'
```

---

## Tools (Modifications)

Tools perform write operations on FBX files.

### Available Tools

| Tool | Purpose | Usage |
|------|---------|-------|
| `bone-reset` | Fix Optitrack exports | `tools bone-reset <file>` |
| `axis-mender` | Coordinate system conversion | `tools axis-mender <file>` |

### Usage

```bash
# Reset bone rotations
fbx-toolkit tools bone-reset character.fbx

# Convert coordinate system
fbx-toolkit tools axis-mender animation.fbx
```

---

## MCP Integration

```bash
# List all commands (JSON)
fbx-toolkit list
```

Output:
```json
{
  "commands": [
    {
      "name": "resources",
      "description": "Read resources from FBX files (URI-based, stateless)",
      "usage": "resources <uri>"
    },
    {
      "name": "tools",
      "description": "Execute FBX modification tools",
      "usage": "tools <tool-name> [args...]"
    }
  ]
}
```

---

## Generic Data Format

All resources output a generic JSON format:

```json
{
  "resource_type": "skeleton",
  "format_version": "1.0",
  "data": {
    ...
  }
}
```

This allows:
- **Cross-format compatibility** (future: glTF, OBJ)
- **Export/import** between formats
- **MCP tool integration**

---

## Future: Session Mode (Stateful)

Planned for in-memory operations:

```bash
# Start session server
fbx-toolkit character.fbx --host

# Use resources in-memory (fast)
fbx-toolkit resources character.fbx/skeleton
fbx-toolkit tools bone-reset character.fbx

# Save when ready
fbx-toolkit save character.fbx output.fbx
fbx-toolkit close character.fbx
```

Session benefits:
- **In-memory** (fast repeated access)
- **Transactional** (undo/redo)
- **IPC** via socket/pipe

---

## Implementation

### File Structure

```
src/
├── main.cpp                    # Command dispatcher
└── commands/
    ├── command.h               # Command interface
    ├── resources.cpp           # Resources command
    ├── tools.cpp               # Tools command dispatcher
    ├── bone_reset.cpp          # Bone reset tool
    └── axis_mender.cpp         # Axis mender tool
```

### Adding New Resources

Edit `src/commands/resources.cpp`:

```cpp
// 1. Add handler function
std::string GetMyResource(FbxScene* scene) {
    // Extract data, return JSON
}

// 2. Route in Execute()
if (resourcePath == "my-resource") {
    output = GetMyResource(scene);
}
```

### Adding New Tools

Edit `src/commands/tools.cpp`:

```cpp
// 1. Create tool implementation (e.g., my_tool.cpp)
// 2. Add export function in main.cpp
int ExecuteMyTool(const std::vector<std::string>& args);

// 3. Route in tools.cpp Execute()
if (toolName == "my-tool") {
    return ExecuteMyTool(toolArgs);
}
```

---

## Testing

```bash
# Test resources
fbx-toolkit resources test.fbx/scene
fbx-toolkit resources test.fbx/skeleton

# Test tools
fbx-toolkit tools bone-reset test.fbx

# Test MCP
fbx-toolkit list
```
