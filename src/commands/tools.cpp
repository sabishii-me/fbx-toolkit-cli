#include "command.h"
#include <iostream>
#include <string>
#include <vector>
#include <memory>

// Forward declarations for tool implementations
class BoneResetCommand;
class AxisMenderCommand;

class ToolsCommand : public Command {
public:
    const char* GetName() const override {
        return "tools";
    }

    const char* GetDescription() const override {
        return "Execute FBX modification tools";
    }

    const char* GetUsage() const override {
        return "tools <tool-name> [args...]";
    }

    int Execute(const std::vector<std::string>& args) override {
        if (args.size() < 1) {
            std::cout << "Usage: " << GetUsage() << "\n\n";
            std::cout << "Available tools:\n";
            std::cout << "  bone-reset <file_or_directory>\n";
            std::cout << "    Reset bone rotations and fix hand/hip positions\n\n";
            std::cout << "  axis-mender <input_fbx_file>\n";
            std::cout << "    Retarget animations with coordinate system adjustments\n\n";
            std::cout << "  split-skeleton <input.fbx> [output_dir]\n";
            std::cout << "    Split multi-skeleton FBX into separate files (proper scene cloning)\n\n";
            std::cout << "Examples:\n";
            std::cout << "  tools bone-reset character.fbx\n";
            std::cout << "  tools axis-mender animation.fbx\n";
            std::cout << "  tools split-skeleton mocap.fbx output/\n";
            return 1;
        }

        std::string toolName = args[0];
        std::vector<std::string> toolArgs(args.begin() + 1, args.end());

        // Dispatch to appropriate tool
        if (toolName == "bone-reset") {
            extern int ExecuteBoneReset(const std::vector<std::string>& args);
            return ExecuteBoneReset(toolArgs);
        } else if (toolName == "axis-mender") {
            extern int ExecuteAxisMender(const std::vector<std::string>& args);
            return ExecuteAxisMender(toolArgs);
        } else if (toolName == "split-skeleton") {
            extern int ExecuteSplitSkeleton(const std::vector<std::string>& args);
            return ExecuteSplitSkeleton(toolArgs);
        } else {
            std::cerr << "Error: Unknown tool: " << toolName << "\n\n";
            std::cout << "Available tools: bone-reset, axis-mender, split-skeleton\n";
            return 1;
        }
    }
};
