# FBX Toolkit

A command-line toolkit for FBX file processing, designed for game development workflows.

## Features

- **Modular architecture** - Easy to add new commands
- **CLI interface** - Simple command-based interaction
- **MCP-ready** - Can be piped to MCP servers
- **Batch processing** - Process single files or entire directories

## Available Commands

### bone-reset
Reset bone rotations and fix hand/hip positions in Optitrack FBX files.

```bash
fbx-toolkit bone-reset <file_or_directory>
```

- Resets all bone rotations to zero on the first frame
- Fixes thumb rotations (45° adjustments for natural hand pose)
- Centers hip position (removes X/Z translation)
- Generates timestamped processing reports

### axis-mender
Retarget FBX animations with coordinate system adjustments.

```bash
fbx-toolkit axis-mender <input_fbx_file>
```

- Retargets animations between skeletons
- Applies -90° X-axis rotation for coordinate system conversion
- Outputs to `<filename>_retargeted.fbx`

## Usage

List all available commands:
```bash
fbx-toolkit
```

List commands in JSON format (for MCP):
```bash
fbx-toolkit list
```

Run a specific command:
```bash
fbx-toolkit <command> [arguments]
```

## Building

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

The executable will be in `build/bin/fbx-toolkit.exe`

## Adding New Commands

1. Create a new `.cpp` file in `src/commands/`
2. Implement the `Command` interface
3. Add factory function: `extern "C" Command* CreateCommand()`
4. Register in `src/main.cpp` in `GetCommands()`

Example:
```cpp
#include "command.h"

class MyCommand : public Command {
public:
    const char* GetName() const override { return "my-command"; }
    const char* GetDescription() const override { return "Does something cool"; }
    const char* GetUsage() const override { return "my-command <args>"; }
    
    int Execute(const std::vector<std::string>& args) override {
        // Your implementation
        return 0;
    }
};

extern "C" Command* CreateCommand() {
    return new MyCommand();
}
```

## Architecture

```
fbx-toolkit/
├── src/
│   ├── main.cpp              # CLI dispatcher & command registry
│   └── commands/
│       ├── command.h         # Base command interface
│       ├── bone_reset.cpp    # Bone reset implementation
│       └── axis_mender.cpp   # Axis mender implementation
├── CMakeLists.txt
└── README.md
```

Each command is self-contained and independent, making the codebase easy to maintain and extend.
