#pragma once
#include <string>
#include <vector>

/**
 * Base interface for all FBX toolkit commands.
 * Each command implements this interface to provide:
 * - Name and description for CLI listing
 * - Argument parsing and execution logic
 */
class Command {
public:
    virtual ~Command() = default;

    // Command metadata
    virtual const char* GetName() const = 0;
    virtual const char* GetDescription() const = 0;
    virtual const char* GetUsage() const = 0;

    // Command execution
    virtual int Execute(const std::vector<std::string>& args) = 0;
};
