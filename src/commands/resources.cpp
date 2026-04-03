#include "command.h"
#include "resources/resource_registry.h"
#include "fbx/fbx_loader.h"
#include <iostream>
#include <sstream>

class ResourcesCommand : public Command {
public:
    const char* GetName() const override {
        return "resources";
    }

    const char* GetDescription() const override {
        return "Read resources from FBX files (URI-based, stateless)";
    }

    const char* GetUsage() const override {
        return "resources <uri>";
    }

    int Execute(const std::vector<std::string>& args) override {
        if (args.size() < 1) {
            PrintUsage();
            return 1;
        }

        // Parse URI: file.fbx/resource/path
        auto [filePath, pathSegments] = ParseURI(args[0]);

        if (pathSegments.empty()) {
            std::cerr << "Error: No resource specified\n";
            return 1;
        }

        // Find handler for this resource
        std::string resourceName = pathSegments[0];
        const ResourceHandler* handler = ResourceRegistry::Instance().Get(resourceName);

        if (!handler) {
            std::cerr << "Error: Unknown resource: " << resourceName << "\n";
            std::cerr << "Available resources: ";
            for (const auto& name : ResourceRegistry::Instance().GetResourceNames()) {
                std::cerr << name << " ";
            }
            std::cerr << "\n";
            return 1;
        }

        // Initialize FBX loader
        FbxLoader loader;
        if (!loader.Initialize(filePath)) {
            std::cerr << "Error: Failed to open FBX file: " << filePath << "\n";
            return 1;
        }

        std::string output;
        LoadStrategy strategy = handler->GetLoadStrategy(pathSegments);

        // Execute handler based on strategy
        if (strategy == LoadStrategy::HEADER_ONLY) {
            // Fast path: no Import() needed
            output = handler->HandleHeaderOnly(loader.GetImporter(), pathSegments);
        } else {
            // Import scene with configured settings
            loader.ConfigureImport(handler, pathSegments);

            if (!loader.ImportScene()) {
                std::cerr << "Error: Failed to import scene\n";
                return 1;
            }

            output = handler->HandleScene(loader.GetScene(), pathSegments);
        }

        std::cout << output << std::endl;

        // Fast exit: Skip FBX SDK cleanup which can hang on large animation data
        // OS will reclaim memory instantly. For host mode, we'll need proper cleanup.
        std::exit(0);
    }

private:
    std::pair<std::string, std::vector<std::string>> ParseURI(const std::string& uri) {
        // Parse URI: file.fbx/resource/path
        size_t fbxPos = uri.find(".fbx/");
        if (fbxPos == std::string::npos) fbxPos = uri.find(".FBX/");

        if (fbxPos == std::string::npos) {
            return {"", {}};
        }

        std::string filePath = uri.substr(0, fbxPos + 4);
        std::string resourcePath = uri.substr(fbxPos + 5);

        // Split resource path into segments
        std::vector<std::string> pathSegments;
        std::string segment;
        std::istringstream pathStream(resourcePath);
        while (std::getline(pathStream, segment, '/')) {
            if (!segment.empty()) pathSegments.push_back(segment);
        }

        return {filePath, pathSegments};
    }

    void PrintUsage() {
        std::cout << "Usage: " << GetUsage() << "\n\n";
        std::cout << "Examples:\n";
        std::cout << "  resources character.fbx/scene\n";
        std::cout << "  resources character.fbx/skeleton               # Brief list\n";
        std::cout << "  resources character.fbx/skeleton/0/**          # Full tree\n";
        std::cout << "\nAvailable resources:\n";

        for (const auto& name : ResourceRegistry::Instance().GetResourceNames()) {
            std::cout << "  " << name << "\n";
        }
    }
};
