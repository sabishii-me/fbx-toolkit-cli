#include "command.h"
#include <fbxsdk.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <functional>

class SplitSkeletonCommand : public Command {
private:
    // Find all skeleton roots in the scene (including parent transform nodes)
    std::vector<FbxNode*> FindSkeletonRoots(FbxNode* node) {
        std::vector<FbxNode*> roots;
        std::function<void(FbxNode*)> traverse = [&](FbxNode* n) {
            if (!n) return;
            FbxSkeleton* skel = n->GetSkeleton();
            if (skel && skel->IsSkeletonRoot()) {
                // Check if parent is a transform node (not skeleton, not mesh)
                FbxNode* parent = n->GetParent();
                if (parent && parent != n->GetScene()->GetRootNode()) {
                    FbxNodeAttribute* parentAttr = parent->GetNodeAttribute();
                    // If parent has no attribute or is just a Null/Transform, use parent as root
                    if (!parentAttr || parentAttr->GetAttributeType() == FbxNodeAttribute::eNull) {
                        roots.push_back(parent);
                    } else {
                        roots.push_back(n);
                    }
                } else {
                    roots.push_back(n);
                }
            }
            for (int i = 0; i < n->GetChildCount(); i++) {
                traverse(n->GetChild(i));
            }
        };
        traverse(node);
        return roots;
    }

    // Recursively clone skeleton hierarchy
    FbxNode* CloneSkeleton(FbxNode* sourceNode, FbxScene* targetScene, FbxNode* targetParent,
                           std::map<FbxNode*, FbxNode*>& boneMap) {
        FbxNode* targetNode = FbxNode::Create(targetScene, sourceNode->GetName());

        // Clone node attribute (skeleton or null/transform)
        FbxNodeAttribute* sourceAttr = sourceNode->GetNodeAttribute();
        if (sourceAttr) {
            FbxSkeleton* sourceSkel = sourceNode->GetSkeleton();
            if (sourceSkel) {
                FbxSkeleton* targetSkel = FbxSkeleton::Create(targetScene, sourceNode->GetName());
                targetSkel->SetSkeletonType(sourceSkel->GetSkeletonType());
                targetSkel->Size.Set(sourceSkel->Size.Get());
                targetNode->SetNodeAttribute(targetSkel);
            } else if (sourceAttr->GetAttributeType() == FbxNodeAttribute::eNull) {
                FbxNull* targetNull = FbxNull::Create(targetScene, sourceNode->GetName());
                targetNode->SetNodeAttribute(targetNull);
            }
        }

        // Copy transforms
        targetNode->LclTranslation.Set(sourceNode->LclTranslation.Get());
        targetNode->LclRotation.Set(sourceNode->LclRotation.Get());
        targetNode->LclScaling.Set(sourceNode->LclScaling.Get());

        // Add to parent or root
        if (targetParent) {
            targetParent->AddChild(targetNode);
        } else {
            targetScene->GetRootNode()->AddChild(targetNode);
        }

        // Store mapping
        boneMap[sourceNode] = targetNode;

        // Recursively clone children (skeleton bones and intermediate transforms)
        for (int i = 0; i < sourceNode->GetChildCount(); i++) {
            FbxNode* child = sourceNode->GetChild(i);
            FbxNodeAttribute* childAttr = child->GetNodeAttribute();
            // Clone if it's a skeleton bone or a transform/null node (intermediate parent)
            if (child->GetSkeleton() || !childAttr || childAttr->GetAttributeType() == FbxNodeAttribute::eNull) {
                CloneSkeleton(child, targetScene, targetNode, boneMap);
            }
        }

        return targetNode;
    }

    // Find all meshes skinned to this skeleton
    std::vector<FbxMesh*> FindSkinnedMeshes(FbxScene* scene, FbxNode* skeletonRoot) {
        std::vector<FbxMesh*> meshes;
        std::set<FbxNode*> skeletonBones;

        // Collect all bones in this skeleton
        std::function<void(FbxNode*)> collectBones = [&](FbxNode* node) {
            if (!node) return;
            if (node->GetSkeleton()) {
                skeletonBones.insert(node);
            }
            for (int i = 0; i < node->GetChildCount(); i++) {
                collectBones(node->GetChild(i));
            }
        };
        collectBones(skeletonRoot);

        // Find meshes that reference these bones
        int meshCount = scene->GetSrcObjectCount<FbxMesh>();
        for (int i = 0; i < meshCount; i++) {
            FbxMesh* mesh = scene->GetSrcObject<FbxMesh>(i);
            int deformerCount = mesh->GetDeformerCount(FbxDeformer::eSkin);

            for (int j = 0; j < deformerCount; j++) {
                FbxSkin* skin = (FbxSkin*)mesh->GetDeformer(j, FbxDeformer::eSkin);
                int clusterCount = skin->GetClusterCount();

                for (int k = 0; k < clusterCount; k++) {
                    FbxCluster* cluster = skin->GetCluster(k);
                    FbxNode* linkNode = cluster->GetLink();
                    if (linkNode && skeletonBones.find(linkNode) != skeletonBones.end()) {
                        meshes.push_back(mesh);
                        goto next_mesh;  // Found a match, move to next mesh
                    }
                }
            }
            next_mesh:;
        }

        return meshes;
    }

    // Clone mesh with skin deformer remapping
    void CloneMeshWithSkin(FbxMesh* sourceMesh, FbxScene* targetScene,
                          std::map<FbxNode*, FbxNode*>& boneMap) {
        // Create mesh node
        FbxNode* meshNode = FbxNode::Create(targetScene, sourceMesh->GetNode()->GetName());
        targetScene->GetRootNode()->AddChild(meshNode);

        // Clone mesh geometry
        FbxMesh* targetMesh = FbxMesh::Create(targetScene, sourceMesh->GetName());
        meshNode->SetNodeAttribute(targetMesh);

        // Copy control points
        int controlPointCount = sourceMesh->GetControlPointsCount();
        targetMesh->InitControlPoints(controlPointCount);
        FbxVector4* srcPoints = sourceMesh->GetControlPoints();
        FbxVector4* dstPoints = targetMesh->GetControlPoints();
        for (int i = 0; i < controlPointCount; i++) {
            dstPoints[i] = srcPoints[i];
        }

        // Copy polygons
        int polyCount = sourceMesh->GetPolygonCount();
        for (int i = 0; i < polyCount; i++) {
            int polySize = sourceMesh->GetPolygonSize(i);
            targetMesh->BeginPolygon(-1, -1, false);
            for (int j = 0; j < polySize; j++) {
                targetMesh->AddPolygon(sourceMesh->GetPolygonVertex(i, j));
            }
            targetMesh->EndPolygon();
        }

        // Copy normals
        FbxGeometryElementNormal* srcNormals = sourceMesh->GetElementNormal();
        if (srcNormals) {
            FbxGeometryElementNormal* dstNormals = targetMesh->CreateElementNormal();
            dstNormals->SetMappingMode(srcNormals->GetMappingMode());
            dstNormals->SetReferenceMode(srcNormals->GetReferenceMode());
            if (srcNormals->GetReferenceMode() == FbxGeometryElement::eDirect) {
                for (int i = 0; i < srcNormals->GetDirectArray().GetCount(); i++) {
                    dstNormals->GetDirectArray().Add(srcNormals->GetDirectArray().GetAt(i));
                }
            }
        }

        // Copy UVs
        for (int i = 0; i < sourceMesh->GetElementUVCount(); i++) {
            FbxGeometryElementUV* srcUV = sourceMesh->GetElementUV(i);
            FbxGeometryElementUV* dstUV = targetMesh->CreateElementUV(srcUV->GetName());
            dstUV->SetMappingMode(srcUV->GetMappingMode());
            dstUV->SetReferenceMode(srcUV->GetReferenceMode());

            for (int j = 0; j < srcUV->GetDirectArray().GetCount(); j++) {
                dstUV->GetDirectArray().Add(srcUV->GetDirectArray().GetAt(j));
            }
            for (int j = 0; j < srcUV->GetIndexArray().GetCount(); j++) {
                dstUV->GetIndexArray().Add(srcUV->GetIndexArray().GetAt(j));
            }
        }

        // Copy materials
        FbxNode* srcNode = sourceMesh->GetNode();
        if (srcNode) {
            for (int i = 0; i < srcNode->GetMaterialCount(); i++) {
                meshNode->AddMaterial(srcNode->GetMaterial(i));
            }
        }

        // Clone skin deformers with remapped bones
        int deformerCount = sourceMesh->GetDeformerCount(FbxDeformer::eSkin);
        for (int i = 0; i < deformerCount; i++) {
            FbxSkin* srcSkin = (FbxSkin*)sourceMesh->GetDeformer(i, FbxDeformer::eSkin);
            FbxSkin* dstSkin = FbxSkin::Create(targetScene, srcSkin->GetName());

            int clusterCount = srcSkin->GetClusterCount();
            for (int j = 0; j < clusterCount; j++) {
                FbxCluster* srcCluster = srcSkin->GetCluster(j);
                FbxNode* srcLink = srcCluster->GetLink();

                // Find corresponding bone in target scene
                auto it = boneMap.find(srcLink);
                if (it == boneMap.end()) continue;  // Bone not in this skeleton, skip

                FbxNode* dstLink = it->second;
                FbxCluster* dstCluster = FbxCluster::Create(targetScene, srcCluster->GetName());
                dstCluster->SetLink(dstLink);
                dstCluster->SetLinkMode(srcCluster->GetLinkMode());

                // Copy weights
                int* indices = srcCluster->GetControlPointIndices();
                double* weights = srcCluster->GetControlPointWeights();
                int indexCount = srcCluster->GetControlPointIndicesCount();
                for (int k = 0; k < indexCount; k++) {
                    dstCluster->AddControlPointIndex(indices[k], weights[k]);
                }

                // Copy transform matrices
                FbxAMatrix transform, transformLink;
                srcCluster->GetTransformMatrix(transform);
                srcCluster->GetTransformLinkMatrix(transformLink);
                dstCluster->SetTransformMatrix(transform);
                dstCluster->SetTransformLinkMatrix(transformLink);

                dstSkin->AddCluster(dstCluster);
            }

            if (dstSkin->GetClusterCount() > 0) {
                targetMesh->AddDeformer(dstSkin);
            }
        }
    }

    // Copy animations for skeleton bones
    void CopyAnimations(FbxScene* sourceScene, FbxScene* targetScene,
                       std::map<FbxNode*, FbxNode*>& boneMap) {
        int animStackCount = sourceScene->GetSrcObjectCount<FbxAnimStack>();
        if (animStackCount == 0) return;

        for (int i = 0; i < animStackCount; i++) {
            FbxAnimStack* srcStack = sourceScene->GetSrcObject<FbxAnimStack>(i);
            FbxAnimStack* dstStack = FbxAnimStack::Create(targetScene, srcStack->GetName());

            int layerCount = srcStack->GetMemberCount<FbxAnimLayer>();
            for (int j = 0; j < layerCount; j++) {
                FbxAnimLayer* srcLayer = srcStack->GetMember<FbxAnimLayer>(j);
                FbxAnimLayer* dstLayer = FbxAnimLayer::Create(targetScene, srcLayer->GetName());
                dstStack->AddMember(dstLayer);

                // Copy animation for each bone in the mapping
                for (auto& pair : boneMap) {
                    FbxNode* srcNode = pair.first;
                    FbxNode* dstNode = pair.second;

                    // Copy translation curves
                    CopyCurves(srcNode->LclTranslation, dstNode->LclTranslation, srcLayer, dstLayer);
                    CopyCurves(srcNode->LclRotation, dstNode->LclRotation, srcLayer, dstLayer);
                    CopyCurves(srcNode->LclScaling, dstNode->LclScaling, srcLayer, dstLayer);
                }
            }
        }
    }

    void CopyCurves(FbxProperty& srcProp, FbxProperty& dstProp, FbxAnimLayer* srcLayer, FbxAnimLayer* dstLayer) {
        const char* components[] = {FBXSDK_CURVENODE_COMPONENT_X, FBXSDK_CURVENODE_COMPONENT_Y, FBXSDK_CURVENODE_COMPONENT_Z};

        for (int i = 0; i < 3; i++) {
            FbxAnimCurve* srcCurve = srcProp.GetCurve(srcLayer, components[i]);
            if (!srcCurve) continue;

            FbxAnimCurve* dstCurve = dstProp.GetCurve(dstLayer, components[i], true);
            if (!dstCurve) continue;

            // Copy all keyframes
            int keyCount = srcCurve->KeyGetCount();
            for (int k = 0; k < keyCount; k++) {
                FbxTime time = srcCurve->KeyGetTime(k);
                float value = srcCurve->KeyGetValue(k);
                int keyIndex = dstCurve->KeyAdd(time);
                dstCurve->KeySetValue(keyIndex, value);
                dstCurve->KeySetInterpolation(keyIndex, srcCurve->KeyGetInterpolation(k));
            }
        }
    }

public:
    const char* GetName() const override {
        return "split-skeleton";
    }

    const char* GetDescription() const override {
        return "Split multi-skeleton FBX into separate files (one per skeleton)";
    }

    const char* GetUsage() const override {
        return "split-skeleton <input.fbx> [output_dir]";
    }

    int Execute(const std::vector<std::string>& args) override {
        if (args.empty()) {
            std::cerr << "Usage: " << GetUsage() << "\n";
            std::cerr << "Example: split-skeleton mocap.fbx output/\n";
            return 1;
        }

        std::string inputPath = args[0];
        std::string outputDir = args.size() > 1 ? args[1] : "";

        // Initialize FBX SDK
        FbxManager* sdkManager = FbxManager::Create();
        if (!sdkManager) {
            std::cerr << "Error: Unable to create FBX Manager\n";
            return 1;
        }

        FbxIOSettings* ios = FbxIOSettings::Create(sdkManager, IOSROOT);
        sdkManager->SetIOSettings(ios);

        // Load source scene
        FbxImporter* importer = FbxImporter::Create(sdkManager, "");
        if (!importer->Initialize(inputPath.c_str(), -1, ios)) {
            std::cerr << "Error: Failed to open file: " << inputPath << "\n";
            std::cerr << importer->GetStatus().GetErrorString() << "\n";
            sdkManager->Destroy();
            return 1;
        }

        FbxScene* sourceScene = FbxScene::Create(sdkManager, "sourceScene");
        if (!importer->Import(sourceScene)) {
            std::cerr << "Error: Failed to import scene\n";
            importer->Destroy();
            sdkManager->Destroy();
            return 1;
        }
        importer->Destroy();

        // Find all skeleton roots
        std::vector<FbxNode*> skeletons = FindSkeletonRoots(sourceScene->GetRootNode());
        if (skeletons.empty()) {
            std::cerr << "Error: No skeletons found in file\n";
            sourceScene->Destroy();
            sdkManager->Destroy();
            return 1;
        }

        std::cout << "Found " << skeletons.size() << " skeleton(s) in file\n";

        // Extract base filename
        std::string baseName = inputPath;
        size_t lastSlash = baseName.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            baseName = baseName.substr(lastSlash + 1);
        }
        size_t lastDot = baseName.find_last_of(".");
        if (lastDot != std::string::npos) {
            baseName = baseName.substr(0, lastDot);
        }

        // Process each skeleton
        for (size_t i = 0; i < skeletons.size(); i++) {
            FbxNode* skelRoot = skeletons[i];
            std::string skelName = skelRoot->GetName();

            std::cout << "\nProcessing skeleton " << (i + 1) << "/" << skeletons.size()
                      << ": " << skelName << "\n";

            // Create new scene for this skeleton
            FbxScene* targetScene = FbxScene::Create(sdkManager, "targetScene");
            targetScene->GetGlobalSettings().SetAxisSystem(sourceScene->GetGlobalSettings().GetAxisSystem());
            targetScene->GetGlobalSettings().SetSystemUnit(sourceScene->GetGlobalSettings().GetSystemUnit());
            targetScene->GetGlobalSettings().SetTimeMode(sourceScene->GetGlobalSettings().GetTimeMode());

            // Clone skeleton with bone mapping
            std::map<FbxNode*, FbxNode*> boneMap;
            CloneSkeleton(skelRoot, targetScene, nullptr, boneMap);
            std::cout << "  Cloned " << boneMap.size() << " bones\n";

            // Find and clone skinned meshes
            std::vector<FbxMesh*> meshes = FindSkinnedMeshes(sourceScene, skelRoot);
            std::cout << "  Found " << meshes.size() << " skinned mesh(es)\n";
            for (FbxMesh* mesh : meshes) {
                CloneMeshWithSkin(mesh, targetScene, boneMap);
            }

            // Copy animations
            CopyAnimations(sourceScene, targetScene, boneMap);
            int animCount = targetScene->GetSrcObjectCount<FbxAnimStack>();
            std::cout << "  Copied " << animCount << " animation(s)\n";

            // Generate output filename
            std::string outputPath = outputDir;
            if (!outputPath.empty() && outputPath.back() != '/' && outputPath.back() != '\\') {
                outputPath += "/";
            }
            outputPath += baseName + "_" + skelName + "_" + std::to_string(i) + ".fbx";

            // Export
            FbxExporter* exporter = FbxExporter::Create(sdkManager, "");
            if (!exporter->Initialize(outputPath.c_str(), -1, ios)) {
                std::cerr << "  Error: Failed to initialize exporter for " << outputPath << "\n";
                targetScene->Destroy();
                continue;
            }

            if (!exporter->Export(targetScene)) {
                std::cerr << "  Error: Failed to export " << outputPath << "\n";
            } else {
                std::cout << "  Saved: " << outputPath << "\n";
            }

            exporter->Destroy();
            targetScene->Destroy();
        }

        sourceScene->Destroy();
        sdkManager->Destroy();

        std::cout << "\nSplit complete! Created " << skeletons.size() << " file(s)\n";
        return 0;
    }
};
