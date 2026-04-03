#include "command.h"
#include <fbxsdk.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <functional>

// Forward declarations for resource handlers
std::string GetSceneInfo(FbxScene* scene);
std::string GetNodes(FbxScene* scene);
std::string GetSkeletonBrief(FbxScene* scene);
std::string GetSkeleton(FbxScene* scene);
std::string GetMeshes(FbxScene* scene);
std::string GetMaterials(FbxScene* scene);
std::string GetAnimations(FbxScene* scene);

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
            std::cout << "Usage: " << GetUsage() << "\n\n";
            std::cout << "Examples:\n";
            std::cout << "  resources character.fbx/scene\n";
            std::cout << "  resources character.fbx/skeleton               # Brief list\n";
            std::cout << "  resources character.fbx/skeleton/0             # Specific skeleton\n";
            std::cout << "  resources character.fbx/skeleton/0/**          # All bones (names)\n";
            std::cout << "  resources character.fbx/skeleton/0/** -t TRS   # With transforms\n";
            std::cout << "  resources character.fbx/skeleton/0/Hips/Spine  # Specific bone\n";
            std::cout << "\nFlags:\n";
            std::cout << "  -t T,R,S,TRS   Transform attributes (Translation, Rotation, Scale)\n";
            std::cout << "  -o FILE        Output to file\n";
            std::cout << "  --tree         Hierarchical structure\n";
            std::cout << "  --flat         Flat list\n";
            return 1;
        }

        // Parse URI: file.fbx/resource/path
        std::string uri = args[0];
        size_t fbxPos = uri.find(".fbx/");
        if (fbxPos == std::string::npos) fbxPos = uri.find(".FBX/");

        if (fbxPos == std::string::npos) {
            std::cerr << "Error: Invalid URI format. Expected: <path/to/file.fbx>/resource\n";
            return 1;
        }

        std::string filePath = uri.substr(0, fbxPos + 4);
        std::string resourcePath = uri.substr(fbxPos + 5);

        // Parse flags from remaining args
        std::string transformFlags = "";
        std::string outputFile = "";
        bool treeMode = false;
        bool flatMode = false;

        for (size_t i = 1; i < args.size(); i++) {
            if (args[i] == "-t" && i + 1 < args.size()) {
                transformFlags = args[++i];
            } else if (args[i] == "-o" && i + 1 < args.size()) {
                outputFile = args[++i];
            } else if (args[i] == "--tree") {
                treeMode = true;
            } else if (args[i] == "--flat") {
                flatMode = true;
            }
        }

        // Split resource path into segments (skeleton/0/Hips/Spine)
        std::vector<std::string> pathSegments;
        std::string segment;
        std::istringstream pathStream(resourcePath);
        while (std::getline(pathStream, segment, '/')) {
            if (!segment.empty()) pathSegments.push_back(segment);
        }

        // Initialize FBX SDK
        FbxManager* sdkManager = FbxManager::Create();
        if (!sdkManager) {
            std::cerr << "Error: Unable to create FBX Manager\n";
            return 1;
        }

        FbxIOSettings* ios = FbxIOSettings::Create(sdkManager, IOSROOT);
        sdkManager->SetIOSettings(ios);

        // Create importer
        FbxImporter* importer = FbxImporter::Create(sdkManager, "");
        if (!importer->Initialize(filePath.c_str(), -1, sdkManager->GetIOSettings())) {
            std::cerr << "Error: Failed to open FBX file: " << filePath << "\n";
            std::cerr << "Error: " << importer->GetStatus().GetErrorString() << "\n";
            importer->Destroy();
            sdkManager->Destroy();
            return 1;
        }

        // Create scene
        FbxScene* scene = FbxScene::Create(sdkManager, "scene");
        if (!importer->Import(scene)) {
            std::cerr << "Error: Failed to import scene\n";
            importer->Destroy();
            scene->Destroy();
            sdkManager->Destroy();
            return 1;
        }

        importer->Destroy();

        // Route to appropriate resource handler
        std::string output;
        std::string baseResource = pathSegments.empty() ? "" : pathSegments[0];

        if (baseResource == "scene") {
            output = GetSceneInfo(scene);
        } else if (baseResource == "nodes") {
            output = GetNodes(scene);
        } else if (baseResource == "skeleton") {
            // Check if requesting full tree (dangerous for agents!)
            bool isRecursive = (pathSegments.size() > 1 && pathSegments.back() == "**") ||
                               transformFlags.length() > 0;

            if (isRecursive || pathSegments.size() > 1) {
                output = GetSkeleton(scene);  // Full tree
                // Add warning for large output
                std::cerr << "Warning: Large output (" << output.length() << " bytes). Consider using grep or -o file.json\n";
            } else {
                output = GetSkeletonBrief(scene);  // Brief list (safe for agents)
            }
        } else if (baseResource == "meshes") {
            output = GetMeshes(scene);
        } else if (resourcePath == "materials") {
            output = GetMaterials(scene);
        } else if (resourcePath == "animations") {
            output = GetAnimations(scene);
        } else {
            std::cerr << "Error: Unknown resource: " << resourcePath << "\n";
            scene->Destroy();
            sdkManager->Destroy();
            return 1;
        }

        std::cout << output << std::endl;

        // Cleanup
        scene->Destroy();
        sdkManager->Destroy();

        return 0;
    }
};

// Resource handler implementations

std::string GetSceneInfo(FbxScene* scene) {
    FbxGlobalSettings& settings = scene->GetGlobalSettings();
    FbxSystemUnit unit = settings.GetSystemUnit();

    std::ostringstream json;
    json << "{\n";
    json << "  \"resource_type\": \"scene\",\n";
    json << "  \"format_version\": \"1.0\",\n";

    // Scene info
    FbxDocumentInfo* info = scene->GetSceneInfo();
    if (info) {
        json << "  \"title\": \"" << (info->mTitle.IsEmpty() ? "Untitled" : info->mTitle.Buffer()) << "\",\n";
        json << "  \"author\": \"" << (info->mAuthor.IsEmpty() ? "" : info->mAuthor.Buffer()) << "\",\n";
    }

    // Coordinate system
    FbxAxisSystem axisSystem = settings.GetAxisSystem();
    int upSign;
    FbxAxisSystem::EUpVector upVector = axisSystem.GetUpVector(upSign);
    json << "  \"coordinate_system\": {\n";
    json << "    \"up_axis\": \"" << (upVector == FbxAxisSystem::eYAxis ? "Y" : (upVector == FbxAxisSystem::eZAxis ? "Z" : "X")) << "\",\n";
    json << "    \"coord_system\": \"" << (axisSystem.GetCoorSystem() == FbxAxisSystem::eRightHanded ? "RightHanded" : "LeftHanded") << "\"\n";
    json << "  },\n";

    // Units
    json << "  \"units\": {\n";
    json << "    \"scale_factor\": " << unit.GetScaleFactor() << ",\n";
    json << "    \"name\": \"" << unit.GetScaleFactorAsString().Buffer() << "\"\n";
    json << "  },\n";

    // Time mode
    FbxTime::EMode timeMode = settings.GetTimeMode();
    json << "  \"time\": {\n";
    json << "    \"mode\": " << static_cast<int>(timeMode) << ",\n";
    json << "    \"fps\": " << FbxTime::GetFrameRate(timeMode) << "\n";
    json << "  },\n";

    // Statistics
    int nodeCount = scene->GetNodeCount();
    int meshCount = scene->GetSrcObjectCount<FbxMesh>();
    int materialCount = scene->GetMaterialCount();
    int textureCount = scene->GetTextureCount();

    json << "  \"statistics\": {\n";
    json << "    \"nodes\": " << nodeCount << ",\n";
    json << "    \"meshes\": " << meshCount << ",\n";
    json << "    \"materials\": " << materialCount << ",\n";
    json << "    \"textures\": " << textureCount << "\n";
    json << "  }\n";
    json << "}";

    return json.str();
}

std::string GetNodes(FbxScene* scene) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"resource_type\": \"nodes\",\n";
    json << "  \"format_version\": \"1.0\",\n";
    json << "  \"nodes\": [\n";

    FbxNode* rootNode = scene->GetRootNode();
    bool first = true;

    std::function<void(FbxNode*, int)> printNode = [&](FbxNode* node, int depth) {
        if (!node) return;

        if (!first) json << ",\n";
        first = false;

        json << "    {\n";
        json << "      \"name\": \"" << node->GetName() << "\",\n";
        json << "      \"type\": \"" << node->GetTypeName() << "\",\n";

        FbxDouble3 translation = node->LclTranslation.Get();
        FbxDouble3 rotation = node->LclRotation.Get();
        FbxDouble3 scale = node->LclScaling.Get();

        json << "      \"transform\": {\n";
        json << "        \"translation\": [" << translation[0] << ", " << translation[1] << ", " << translation[2] << "],\n";
        json << "        \"rotation\": [" << rotation[0] << ", " << rotation[1] << ", " << rotation[2] << "],\n";
        json << "        \"scale\": [" << scale[0] << ", " << scale[1] << ", " << scale[2] << "]\n";
        json << "      },\n";

        json << "      \"children\": [";
        for (int i = 0; i < node->GetChildCount(); i++) {
            if (i > 0) json << ", ";
            json << "\"" << node->GetChild(i)->GetName() << "\"";
        }
        json << "]\n";
        json << "    }";

        for (int i = 0; i < node->GetChildCount(); i++) {
            printNode(node->GetChild(i), depth + 1);
        }
    };

    if (rootNode) {
        for (int i = 0; i < rootNode->GetChildCount(); i++) {
            printNode(rootNode->GetChild(i), 0);
        }
    }

    json << "\n  ]\n";
    json << "}";

    return json.str();
}

std::string GetSkeletonBrief(FbxScene* scene) {
    std::ostringstream json;
    FbxNode* rootNode = scene->GetRootNode();

    // Count skeletons and bones
    int skeletonCount = 0;
    int totalBones = 0;
    std::vector<std::string> skeletonNames;

    std::function<void(FbxNode*)> findSkeletons = [&](FbxNode* node) {
        if (!node) return;
        FbxSkeleton* skeleton = node->GetSkeleton();
        if (skeleton) {
            totalBones++;
            if (skeleton->IsSkeletonRoot()) {
                skeletonCount++;
                skeletonNames.push_back(node->GetName());
            }
        }
        for (int i = 0; i < node->GetChildCount(); i++) {
            findSkeletons(node->GetChild(i));
        }
    };

    if (rootNode) findSkeletons(rootNode);

    // Brief output: just counts and names
    json << "{\n";
    json << "  \"resource_type\": \"skeleton\",\n";
    json << "  \"format_version\": \"1.0\",\n";
    json << "  \"skeleton_count\": " << skeletonCount << ",\n";
    json << "  \"total_bones\": " << totalBones << ",\n";
    json << "  \"skeletons\": [";
    for (size_t i = 0; i < skeletonNames.size(); i++) {
        if (i > 0) json << ", ";
        json << "\"" << skeletonNames[i] << "\"";
    }
    json << "],\n";
    json << "  \"hint\": \"Use skeleton/0/** or -r for full tree (large output)\"\n";
    json << "}";

    return json.str();
}

std::string GetSkeleton(FbxScene* scene) {
    std::ostringstream json;
    FbxNode* rootNode = scene->GetRootNode();

    // Count skeletons and bones
    int skeletonCount = 0;
    int totalBones = 0;
    std::function<void(FbxNode*)> countSkeletons = [&](FbxNode* node) {
        if (!node) return;
        FbxSkeleton* skeleton = node->GetSkeleton();
        if (skeleton) {
            totalBones++;
            if (skeleton->IsSkeletonRoot()) skeletonCount++;
        }
        for (int i = 0; i < node->GetChildCount(); i++) {
            countSkeletons(node->GetChild(i));
        }
    };
    if (rootNode) countSkeletons(rootNode);

    // Recursive function to print bone tree
    std::function<void(FbxNode*, int)> printBoneTree = [&](FbxNode* node, int depth) {
        if (!node) return;

        FbxSkeleton* skeleton = node->GetSkeleton();
        if (skeleton) {
            FbxDouble3 translation = node->LclTranslation.Get();
            FbxDouble3 rotation = node->LclRotation.Get();

            json << "{\n";
            json << "      \"name\": \"" << node->GetName() << "\",\n";
            json << "      \"type\": \"" << (skeleton->IsSkeletonRoot() ? "Root" : "Bone") << "\",\n";
            json << "      \"transform\": {\n";
            json << "        \"translation\": [" << translation[0] << ", " << translation[1] << ", " << translation[2] << "],\n";
            json << "        \"rotation\": [" << rotation[0] << ", " << rotation[1] << ", " << rotation[2] << "]\n";
            json << "      },\n";
            json << "      \"children\": [";

            // Print children
            bool firstChild = true;
            for (int i = 0; i < node->GetChildCount(); i++) {
                FbxNode* child = node->GetChild(i);
                if (child && child->GetSkeleton()) {
                    if (!firstChild) json << ",";
                    firstChild = false;
                    json << "\n        ";
                    printBoneTree(child, depth + 1);
                }
            }

            json << "\n      ]\n";
            json << "    }";
        }
    };

    // Output JSON
    json << "{\n";
    json << "  \"resource_type\": \"skeleton\",\n";
    json << "  \"format_version\": \"1.0\",\n";
    json << "  \"skeleton_count\": " << skeletonCount << ",\n";
    json << "  \"total_bones\": " << totalBones << ",\n";
    json << "  \"skeletons\": [\n";

    // Find and print all skeleton roots
    bool firstSkeleton = true;
    std::function<void(FbxNode*)> findSkeletonRoots = [&](FbxNode* node) {
        if (!node) return;
        FbxSkeleton* skeleton = node->GetSkeleton();
        if (skeleton && skeleton->IsSkeletonRoot()) {
            if (!firstSkeleton) json << ",\n";
            firstSkeleton = false;
            json << "    ";
            printBoneTree(node, 0);
        }
        for (int i = 0; i < node->GetChildCount(); i++) {
            findSkeletonRoots(node->GetChild(i));
        }
    };

    if (rootNode) {
        findSkeletonRoots(rootNode);
    }

    json << "\n  ]\n";
    json << "}";

    return json.str();
}

std::string GetMeshes(FbxScene* scene) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"resource_type\": \"meshes\",\n";
    json << "  \"format_version\": \"1.0\",\n";
    json << "  \"meshes\": [\n";

    int meshCount = scene->GetSrcObjectCount<FbxMesh>();
    for (int i = 0; i < meshCount; i++) {
        FbxMesh* mesh = scene->GetSrcObject<FbxMesh>(i);

        if (i > 0) json << ",\n";
        json << "    {\n";
        json << "      \"name\": \"" << mesh->GetName() << "\",\n";
        json << "      \"vertices\": " << mesh->GetControlPointsCount() << ",\n";
        json << "      \"polygons\": " << mesh->GetPolygonCount() << ",\n";
        json << "      \"has_normals\": " << (mesh->GetElementNormalCount() > 0 ? "true" : "false") << ",\n";
        json << "      \"has_uvs\": " << (mesh->GetElementUVCount() > 0 ? "true" : "false") << ",\n";
        json << "      \"has_vertex_colors\": " << (mesh->GetElementVertexColorCount() > 0 ? "true" : "false") << "\n";
        json << "    }";
    }

    json << "\n  ]\n";
    json << "}";

    return json.str();
}

std::string GetMaterials(FbxScene* scene) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"resource_type\": \"materials\",\n";
    json << "  \"format_version\": \"1.0\",\n";
    json << "  \"materials\": [\n";

    int materialCount = scene->GetMaterialCount();
    for (int i = 0; i < materialCount; i++) {
        FbxSurfaceMaterial* material = scene->GetMaterial(i);

        if (i > 0) json << ",\n";
        json << "    {\n";
        json << "      \"name\": \"" << material->GetName() << "\",\n";
        json << "      \"shading_model\": \"" << material->ShadingModel.Get().Buffer() << "\"\n";
        json << "    }";
    }

    json << "\n  ]\n";
    json << "}";

    return json.str();
}

std::string GetAnimations(FbxScene* scene) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"resource_type\": \"animations\",\n";
    json << "  \"format_version\": \"1.0\",\n";
    json << "  \"animations\": [\n";

    int animStackCount = scene->GetSrcObjectCount<FbxAnimStack>();
    for (int i = 0; i < animStackCount; i++) {
        FbxAnimStack* animStack = scene->GetSrcObject<FbxAnimStack>(i);
        FbxTimeSpan timeSpan = animStack->GetLocalTimeSpan();

        if (i > 0) json << ",\n";
        json << "    {\n";
        json << "      \"name\": \"" << animStack->GetName() << "\",\n";
        json << "      \"start_time\": " << timeSpan.GetStart().GetSecondDouble() << ",\n";
        json << "      \"stop_time\": " << timeSpan.GetStop().GetSecondDouble() << ",\n";
        json << "      \"duration\": " << timeSpan.GetDuration().GetSecondDouble() << "\n";
        json << "    }";
    }

    json << "\n  ]\n";
    json << "}";

    return json.str();
}
