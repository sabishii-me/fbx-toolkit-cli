#ifndef TOOL_REGISTRY_H
#define TOOL_REGISTRY_H

#include "tool.h"
#include <map>
#include <vector>
#include <string>

// Registry for all tools - auto-registration pattern
class ToolRegistry {
public:
    static ToolRegistry& GetInstance() {
        static ToolRegistry instance;
        return instance;
    }

    void RegisterTool(Tool* tool) {
        tools_[tool->GetName()] = tool;
    }

    Tool* GetTool(const std::string& name) const {
        auto it = tools_.find(name);
        return (it != tools_.end()) ? it->second : nullptr;
    }

    std::vector<Tool*> GetAllTools() const {
        std::vector<Tool*> result;
        for (const auto& pair : tools_) {
            result.push_back(pair.second);
        }
        return result;
    }

private:
    ToolRegistry() = default;
    std::map<std::string, Tool*> tools_;
};

// Helper for auto-registration at static initialization time
struct ToolRegistrar {
    ToolRegistrar(Tool* tool) {
        ToolRegistry::GetInstance().RegisterTool(tool);
    }
};

// Macro for easy registration
#define REGISTER_TOOL(ToolClass) \
    static ToolClass g_##ToolClass##_instance; \
    static ToolRegistrar g_##ToolClass##_registrar(&g_##ToolClass##_instance);

#endif
