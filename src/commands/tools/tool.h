#ifndef TOOL_H
#define TOOL_H

#include <string>
#include <vector>

// Base interface for tool handlers
// Each tool (bone-reset, axis-mender, etc.) implements this
class Tool {
public:
    virtual ~Tool() = default;

    // What tool is this? (e.g., "bone-reset", "axis-mender")
    virtual const char* GetName() const = 0;

    // Brief description for help text
    virtual const char* GetDescription() const = 0;

    // Usage pattern (e.g., "<input.fbx> <output.fbx>")
    virtual const char* GetUsagePattern() const = 0;

    // Detailed usage example
    virtual const char* GetUsageExample() const = 0;

    // Execute the tool
    virtual int Execute(const std::vector<std::string>& args) = 0;
};

#endif
