#include "commands/command.h"
#include <iostream>
#include <vector>
#include <memory>
#include <string>

// Forward declarations for command factory functions
extern Command* CreateCommand(); // from bone_reset.cpp or axis_mender.cpp

// Registry of all available commands - ADD NEW COMMANDS HERE
std::vector<std::unique_ptr<Command>> GetCommands();

// Resource handlers (modular architecture)
#include "commands/resources/scene_handler.cpp"
#include "commands/resources/skeleton_handler.cpp"
#include "commands/resources/nodes_handler.cpp"
#include "commands/resources/meshes_handler.cpp"
#include "commands/resources/materials_handler.cpp"
#include "commands/resources/animations_handler.cpp"
#include "commands/resources.cpp"

// Tool implementations
#include "commands/tools.cpp"
#include "commands/bone_reset.cpp"
#include "commands/axis_mender.cpp"
#include "commands/split_skeleton.cpp"

// Export tool execution functions for tools.cpp
int ExecuteBoneReset(const std::vector<std::string>& args) {
    BoneResetCommand cmd;
    return cmd.Execute(args);
}

int ExecuteAxisMender(const std::vector<std::string>& args) {
    AxisMenderCommand cmd;
    return cmd.Execute(args);
}

int ExecuteSplitSkeleton(const std::vector<std::string>& args) {
    SplitSkeletonCommand cmd;
    return cmd.Execute(args);
}

// Command registry implementation
std::vector<std::unique_ptr<Command>> GetCommands() {
    std::vector<std::unique_ptr<Command>> commands;

    // Top-level commands
    commands.emplace_back(new ResourcesCommand());
    commands.emplace_back(new ToolsCommand());

    return commands;
}

void PrintUsage() {
    std::cout << "FBX Toolkit - Command-line tools for FBX file processing\n\n";
    std::cout << "Usage: fbx-toolkit <command> [arguments]\n\n";
    std::cout << "Available commands:\n";

    auto commands = GetCommands();
    for (const auto& cmd : commands) {
        std::cout << "  " << cmd->GetName() << "\n";
        std::cout << "    " << cmd->GetDescription() << "\n";
        std::cout << "    Usage: " << cmd->GetUsage() << "\n\n";
    }

    std::cout << "For MCP integration, run: fbx-toolkit list\n";
}

void ListCommandsJSON() {
    std::cout << "{\n";
    std::cout << "  \"commands\": [\n";

    auto commands = GetCommands();
    for (size_t i = 0; i < commands.size(); ++i) {
        const auto& cmd = commands[i];
        std::cout << "    {\n";
        std::cout << "      \"name\": \"" << cmd->GetName() << "\",\n";
        std::cout << "      \"description\": \"" << cmd->GetDescription() << "\",\n";
        std::cout << "      \"usage\": \"" << cmd->GetUsage() << "\"\n";
        std::cout << "    }";
        if (i < commands.size() - 1) {
            std::cout << ",";
        }
        std::cout << "\n";
    }

    std::cout << "  ]\n";
    std::cout << "}\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        PrintUsage();
        return 0;
    }

    std::string commandName = argv[1];

    // Special command to list all commands in JSON format (for MCP)
    if (commandName == "list") {
        ListCommandsJSON();
        return 0;
    }

    // Find and execute the requested command
    auto commands = GetCommands();
    for (const auto& cmd : commands) {
        if (commandName == cmd->GetName()) {
            std::vector<std::string> args;
            for (int i = 2; i < argc; ++i) {
                args.push_back(argv[i]);
            }
            return cmd->Execute(args);
        }
    }

    std::cerr << "Unknown command: " << commandName << "\n\n";
    PrintUsage();
    return 1;
}
