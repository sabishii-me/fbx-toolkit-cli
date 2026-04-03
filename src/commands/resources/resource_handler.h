#ifndef RESOURCE_HANDLER_H
#define RESOURCE_HANDLER_H

#include <fbxsdk.h>
#include <string>
#include <vector>

// Load strategy for resources
enum class LoadStrategy {
    HEADER_ONLY,    // No Import() - read file header only (fastest)
    MINIMAL,        // Import with minimal flags (skip animations, textures, materials)
    SELECTIVE,      // Import with custom flags based on what's needed
    FULL            // Import everything
};

// Base interface for resource handlers
// Each resource type (scene, skeleton, mesh, etc.) implements this
class ResourceHandler {
public:
    virtual ~ResourceHandler() = default;

    // What resource does this handle? (e.g., "scene", "skeleton", "meshes")
    virtual const char* GetResourceName() const = 0;

    // What loading strategy does this resource need?
    virtual LoadStrategy GetLoadStrategy(const std::vector<std::string>& pathSegments) const = 0;

    // Configure FBX import settings before Import() is called
    // Only called for MINIMAL, SELECTIVE, and FULL strategies
    virtual void ConfigureImport(FbxIOSettings* ios, const std::vector<std::string>& pathSegments) const {}

    // Handle header-only query (for HEADER_ONLY strategy)
    // Fast path: no scene import needed
    virtual std::string HandleHeaderOnly(FbxImporter* importer,
                                        const std::vector<std::string>& pathSegments) const {
        return "{\"error\": \"Header-only not supported for this resource\"}";
    }

    // Handle full scene query (for MINIMAL, SELECTIVE, FULL strategies)
    // Scene has been imported with configured settings
    virtual std::string HandleScene(FbxScene* scene,
                                   const std::vector<std::string>& pathSegments) const = 0;
};

#endif
