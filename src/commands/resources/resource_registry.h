#ifndef RESOURCE_REGISTRY_H
#define RESOURCE_REGISTRY_H

#include "resource_handler.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

// Registry of all resource handlers
// Handlers auto-register themselves using REGISTER_RESOURCE_HANDLER macro
class ResourceRegistry {
private:
    std::map<std::string, ResourceHandler*> handlers_;

    ResourceRegistry() = default;

public:
    // Singleton instance
    static ResourceRegistry& Instance() {
        static ResourceRegistry instance;
        return instance;
    }

    // Register a handler (called by auto-registration macros)
    void Register(ResourceHandler* handler) {
        handlers_[handler->GetResourceName()] = handler;
    }

    // Get handler by resource name
    const ResourceHandler* Get(const std::string& resourceName) const {
        auto it = handlers_.find(resourceName);
        return it != handlers_.end() ? it->second : nullptr;
    }

    // Get all registered resource names
    std::vector<std::string> GetResourceNames() const {
        std::vector<std::string> names;
        for (const auto& pair : handlers_) {
            names.push_back(pair.first);
        }
        return names;
    }
};

// Auto-registration helper
// Usage: REGISTER_RESOURCE_HANDLER(SceneHandler)
#define REGISTER_RESOURCE_HANDLER(HandlerClass) \
    static HandlerClass HandlerClass##_instance; \
    static struct HandlerClass##_Registrar { \
        HandlerClass##_Registrar() { \
            ResourceRegistry::Instance().Register(&HandlerClass##_instance); \
        } \
    } HandlerClass##_registrar;

#endif
