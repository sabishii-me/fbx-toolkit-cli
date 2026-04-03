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
    importer->Import(scene);
    
    std::cout << "Cameras: " << scene->GetSrcObjectCount<FbxCamera>() << "\n";
    std::cout << "Lights: " << scene->GetSrcObjectCount<FbxLight>() << "\n";
    std::cout << "Poses: " << scene->GetPoseCount() << "\n";
    std::cout << "Audio: " << scene->GetSrcObjectCount<FbxAudio>() << "\n";
    std::cout << "Constraints: " << scene->GetSrcObjectCount<FbxConstraint>() << "\n";
    std::cout << "Display Layers: " << scene->GetMemberCount<FbxDisplayLayer>() << "\n";
    
    manager->Destroy();
    return 0;
}
