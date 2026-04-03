#include <fbxsdk.h>
#include <iostream>

int main() {
    FbxManager* manager = FbxManager::Create();
    FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(ios);
    
    FbxImporter* importer = FbxImporter::Create(manager, "");
    if (!importer->Initialize("test_files/PiaMeraleo_1.0.27.fbx", -1, ios)) {
        std::cerr << "Failed to open\n";
        return 1;
    }
    
    FbxScene* scene = FbxScene::Create(manager, "");
    
    // Check pose count BEFORE import
    std::cout << "Pose count before import: " << scene->GetPoseCount() << "\n";
    
    importer->Import(scene);
    
    // Check pose count AFTER import
    std::cout << "Pose count after import: " << scene->GetPoseCount() << "\n";
    
    // List all poses
    for (int i = 0; i < scene->GetPoseCount(); i++) {
        FbxPose* pose = scene->GetPose(i);
        std::cout << "  Pose " << i << ": " << pose->GetName() 
                  << " (BindPose=" << pose->IsBindPose() 
                  << ", RestPose=" << pose->IsRestPose() 
                  << ", Nodes=" << pose->GetCount() << ")\n";
    }
    
    manager->Destroy();
    return 0;
}
