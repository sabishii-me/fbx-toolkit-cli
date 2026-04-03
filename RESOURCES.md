# FBX Toolkit - Resource Design

## Resource URI Pattern

```
fbx://<file-path>/<resource-type>[/<resource-id>]
```

---

## Generic 3D Resources

| Resource | URI Example | Description | Data Format |
|----------|-------------|-------------|-------------|
| **Scene** | `fbx://file.fbx/scene` | Scene metadata (units, coordinate system, up-axis) | JSON |
| **Nodes** | `fbx://file.fbx/nodes` | All scene graph nodes with hierarchy | JSON |
| **Node** | `fbx://file.fbx/nodes/Hips` | Single node with transform and children | JSON |
| **Meshes** | `fbx://file.fbx/meshes` | List all mesh names | JSON |
| **Mesh** | `fbx://file.fbx/meshes/BodyMesh` | Mesh geometry (vertices, normals, UVs, faces) | JSON/Binary |
| **Materials** | `fbx://file.fbx/materials` | All materials with properties | JSON |
| **Material** | `fbx://file.fbx/materials/Skin` | Single material definition | JSON |
| **Textures** | `fbx://file.fbx/textures` | All texture references | JSON |
| **Texture** | `fbx://file.fbx/textures/diffuse.png` | Texture metadata and file path | JSON |
| **Skeleton** | `fbx://file.fbx/skeleton` | Complete bone hierarchy | JSON |
| **Bone** | `fbx://file.fbx/skeleton/Hips` | Single bone with transform | JSON |
| **Animations** | `fbx://file.fbx/animations` | All animation takes/clips | JSON |
| **Animation** | `fbx://file.fbx/animations/Take001` | Animation curves and keyframes | JSON |
| **Skin Weights** | `fbx://file.fbx/meshes/Body/weights` | Vertex-to-bone weight mapping | JSON/Binary |
| **Normals** | `fbx://file.fbx/meshes/Body/normals` | Vertex normals | Binary |
| **UVs** | `fbx://file.fbx/meshes/Body/uvs` | UV coordinates (per UV set) | Binary |
| **Vertex Colors** | `fbx://file.fbx/meshes/Body/colors` | Per-vertex color data | Binary |
| **Tangents** | `fbx://file.fbx/meshes/Body/tangents` | Tangent vectors for normal mapping | Binary |
| **Lights** | `fbx://file.fbx/lights` | All lights in scene | JSON |
| **Cameras** | `fbx://file.fbx/cameras` | All cameras in scene | JSON |
| **Custom Attrs** | `fbx://file.fbx/nodes/Hips/attributes` | Custom properties/user data | JSON |

---

## FBX-Specific Resources

| Resource | URI Example | Description | Data Format |
|----------|-------------|-------------|-------------|
| **FBX Info** | `fbx://file.fbx/fbx-info` | FBX version, creator, timestamp | JSON |
| **Takes** | `fbx://file.fbx/fbx-takes` | FBX-specific animation stacks | JSON |
| **Poses** | `fbx://file.fbx/fbx-poses` | Bind poses and rest poses | JSON |
| **Constraints** | `fbx://file.fbx/fbx-constraints` | IK, aim, parent constraints | JSON |

---

## Resource Operations

### Read (GET)
```bash
fbx-toolkit get fbx://character.fbx/skeleton
fbx-toolkit get fbx://character.fbx/meshes/Body
```

### Export (SAVE)
```bash
fbx-toolkit export fbx://character.fbx/skeleton --to skeleton.json
fbx-toolkit export fbx://character.fbx/meshes --to meshes/
```

### List (DISCOVER)
```bash
fbx-toolkit list fbx://character.fbx
fbx-toolkit list fbx://character.fbx/meshes
```

---

## Generic Data Format Example

```json
{
  "resource_type": "skeleton",
  "format_version": "1.0",
  "source": "character.fbx",
  "timestamp": "2026-04-02T22:30:00Z",
  "data": {
    "root": "Hips",
    "bone_count": 52,
    "bones": [
      {
        "name": "Hips",
        "parent": null,
        "transform": {
          "translation": [0.0, 100.0, 0.0],
          "rotation": [0.0, 0.0, 0.0],
          "scale": [1.0, 1.0, 1.0]
        },
        "children": ["Spine", "LeftUpLeg", "RightUpLeg"]
      }
    ]
  }
}
```

---

## Notes

- **JSON** for metadata and small data
- **Binary** for large vertex/animation data (optional optimization)
- **Streaming** support for huge meshes (future)
- **Cross-format** compatible (works for glTF, OBJ later)
