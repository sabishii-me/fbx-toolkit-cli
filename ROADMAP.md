# FBX Toolkit Roadmap

## Current Status (v0.1)

✅ **Implemented:**
- Resources command (URI-based queries: scene, nodes, skeleton, meshes, materials, animations)
- Progressive disclosure (brief by default, full with `**`, agent-safe)
- Tools command dispatcher (bone-reset, axis-mender, split-skeleton)
- Split-skeleton with proper scene cloning + parent node preservation
- Cross-platform build (Windows/Linux/macOS with embedded FBX SDK)
- GitHub CI workflow

---

## Future Work

### 1. MCP CLI Framework (Reusable Library)

**Goal:** Extract reusable MCP server framework that ANY CLI tool can use

**Library: `mcp-cli-framework`**
- C++ library for building CLI tools with MCP support
- Three execution modes:
  1. **CLI mode** - Normal command-line execution
  2. **MCP stdio mode** - JSON-RPC over stdin/stdout (MCP protocol)
  3. **Host mode** - Long-running server with session state
- Features:
  - `MCPServer` class for protocol handling
  - `CommandRegistry` for auto-discovery
  - JSON serialization helpers
  - Transport abstraction (stdio, UDP, pipes, shared memory)
  - Session management for stateful operations

**Usage Pattern:**
```cpp
int main(int argc, char** argv) {
    MCPServer server("my-tool");
    server.registerCommand(new MyCommand());
    
    if (contains(argv, "--mcp")) {
        return server.runMCP();  // MCP stdio mode
    } else if (contains(argv, "--host")) {
        return server.runHost();  // Long-running session mode
    } else {
        return server.runCLI(argc, argv);  // Normal CLI
    }
}
```

**Benefits:**
- Write once, works as CLI + MCP server + Host server
- No MCP knowledge needed in command implementations
- Reusable across projects (FBX, GLTF, OBJ tools, etc.)
- Commands stay simple and testable

---

### 2. Tool Registry Auto-Discovery

**Problem:** Hardcoding tools in `tools.cpp` doesn't scale to 50+ tools

**Solution:** Plugin-like architecture
```cpp
// Each tool registers itself at startup
REGISTER_TOOL(SplitSkeletonCommand);
REGISTER_TOOL(MergeSkeletonsCommand);
REGISTER_TOOL(RetargetAnimationCommand);

// Dispatcher auto-discovers all tools
ToolRegistry::dispatch(toolName, args);
```

**Implementation:**
- Static registration pattern
- Reflection/metadata for tool discovery
- Help text auto-generation
- No manual dispatcher updates needed

---

### 3. Additional FBX Tools

**Planned Tools:**
- `merge-skeletons` - Combine multiple FBX files into one
- `retarget-animation` - Copy animation from one skeleton to another
- `bake-animation` - Sample animation curves to keyframes
- `optimize-mesh` - Reduce vertex/polygon count
- `extract-textures` - Export embedded textures
- `convert-units` - Change scale/coordinate system
- `validate` - Check FBX file integrity
- `diff` - Compare two FBX files

---

### 4. Session/Host Mode Implementation

**Use Case:** Avoid reloading large FBX files for multiple operations

**Architecture:**
```
fbx-toolkit --host file.fbx

Session started: file_fbx_12345
└── FBX scene loaded in memory
    └── Accepts commands via:
        ├── UDP socket (localhost:port)
        ├── Named pipes
        └── Shared memory

Client sends: {"session": "file_fbx_12345", "command": "resources", "args": ["skeleton"]}
Server responds: {...skeleton JSON...}
```

**Benefits:**
- Load once, query many times
- Fast iteration for agents/scripts
- Stateful operations (undo/redo)
- Watch mode for live updates

**Transport Options:**
1. **UDP** - Simple, cross-platform, no connection management
2. **Named pipes** - OS-native IPC
3. **Shared memory** - Fastest, zero-copy reads

---

### 5. Additional Resource Types

**Expand `resources` command:**
- `lights` - Scene lighting information
- `cameras` - Camera properties and transforms
- `constraints` - IK, aim, parent constraints
- `blend-shapes` - Morph targets/shape keys
- `custom-properties` - User-defined metadata
- `thumbnails` - Embedded preview images

---

### 6. Advanced Features

**Animation System:**
- Animation layering/blending
- Curve editing (add/remove keyframes)
- IK baking
- Motion path extraction

**Mesh Operations:**
- UV unwrapping/optimization
- Normal recalculation
- LOD generation
- Skinning weight editing

**Performance:**
- Streaming large files (don't load all in memory)
- Parallel processing for batch operations
- Progress reporting for long operations

---

## Non-Goals

❌ Not building a full DCC tool (use Blender/Maya for that)
❌ Not replacing FBX SDK (we wrap it)
❌ Not a GUI application (CLI/MCP only)
❌ Not a file format converter (use existing tools)

---

## Next Session Priorities

1. ✅ Commit current work (DONE)
2. Add Windows FBX SDK to repository (fbx_win.exe 115MB)
3. Push to GitHub and test CI
4. Start `mcp-cli-framework` design document
5. Design tool registry pattern

---

## Long-Term Vision

**Make FBX accessible to:**
- AI agents (via MCP protocol)
- Scripts/automation (via CLI)
- Other applications (via host mode API)

**Extend pattern to:**
- GLTF toolkit (similar architecture)
- OBJ toolkit (simple mesh format)
- USD toolkit (Universal Scene Description)
- Generic 3D asset pipeline tools

**Ultimate Goal:**
- Unified MCP-based 3D asset toolchain
- Agent-friendly, scriptable, performant
- Reusable framework for any CLI tool
