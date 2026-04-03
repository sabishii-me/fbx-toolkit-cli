#ifndef FBX_LOADER_H
#define FBX_LOADER_H

#include <fbxsdk.h>
#include <string>
#include "../resources/resource_handler.h"

// Smart FBX file loader with lazy loading support
// Handles SDK initialization, import configuration, and cleanup
class FbxLoader {
private:
    FbxManager* manager_;
    FbxIOSettings* ios_;
    FbxImporter* importer_;
    FbxScene* scene_;

public:
    FbxLoader() : manager_(nullptr), ios_(nullptr), importer_(nullptr), scene_(nullptr) {}

    ~FbxLoader() {
        Cleanup();
    }

    // Initialize and open FBX file
    bool Initialize(const std::string& filePath) {
        manager_ = FbxManager::Create();
        if (!manager_) {
            return false;
        }

        ios_ = FbxIOSettings::Create(manager_, IOSROOT);
        manager_->SetIOSettings(ios_);

        importer_ = FbxImporter::Create(manager_, "");
        if (!importer_->Initialize(filePath.c_str(), -1, ios_)) {
            return false;
        }

        return true;
    }

    // Configure import settings (call before ImportScene)
    void ConfigureImport(const ResourceHandler* handler, const std::vector<std::string>& pathSegments) {
        if (handler) {
            handler->ConfigureImport(ios_, pathSegments);
        }
    }

    // Import scene with configured settings
    bool ImportScene() {
        if (!importer_) {
            return false;
        }

        scene_ = FbxScene::Create(manager_, "scene");
        if (!importer_->Import(scene_)) {
            return false;
        }

        return true;
    }

    // Get importer (for header-only reads)
    FbxImporter* GetImporter() const { return importer_; }

    // Get loaded scene (after ImportScene)
    FbxScene* GetScene() const { return scene_; }

    // Get IO settings (for manual configuration)
    FbxIOSettings* GetIOSettings() const { return ios_; }

private:
    void Cleanup() {
        if (importer_) {
            importer_->Destroy();
            importer_ = nullptr;
        }
        if (scene_) {
            scene_->Destroy();
            scene_ = nullptr;
        }
        if (manager_) {
            manager_->Destroy();
            manager_ = nullptr;
        }
    }
};

#endif
