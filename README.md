# FBX Toolkit

Command-line tool for querying and modifying FBX files. Designed for AI agents and automation.

## Installation

Download prebuilt binaries from [Releases](https://github.com/sabishii-me/fbx-toolkit-cli/releases):

- **Windows**: `fbx-toolkit.exe` + `libfbxsdk.dll`
- **Linux**: `fbx-toolkit`
- **macOS**: `fbx-toolkit`

Place in your PATH or use the full path to the binary.

## Usage

Run `fbx-toolkit` with no arguments to see all available commands:

```bash
fbx-toolkit
```

The CLI will show you:
- Available commands
- Usage patterns
- Resource types
- Tool options

All output is JSON (except binary data extraction). The CLI is self-documenting - explore it to learn what it can do.

## Quick Examples

```bash
# Query scene metadata
fbx-toolkit resources character.fbx/scene

# List available resources
fbx-toolkit resources character.fbx/animations
fbx-toolkit resources character.fbx/meshes
fbx-toolkit resources character.fbx/skeleton

# Modify FBX files
fbx-toolkit tools bone-reset input.fbx output.fbx
fbx-toolkit tools split-skeleton input.fbx output_dir/
```

### Animation frame-rate tools

These tools intentionally have different behavior:

```bash
# True resampling: preserve duration/speed and bake interpolated transform keys at 60 FPS.
# Key count and file size may change.
fbx-toolkit tools resample input.fbx output_60fps.fbx 60 --zero-start

# FPS correction: keep existing transform keys but retimestamp them as 180 FPS samples.
# Duration and playback speed change; key count is preserved.
fbx-toolkit tools reinterpret-fps slow_motion.fbx corrected_180fps.fbx 180 --zero-start
```

`--zero-start` is optional and moves the animation start to `00:00:00`. Both tools write a new file and reject identical input/output paths.

## Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md) - Design and implementation
- [ROADMAP.md](ROADMAP.md) - Planned features

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file.

**Note**: This software uses the Autodesk FBX SDK, which is proprietary software licensed separately by Autodesk. The FBX SDK is free for development use but subject to Autodesk's terms and conditions.
