#include "command.h"
#include <fbxsdk.h>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

class AxisMenderCommand : public Command {
private:
    FbxNode* FindRootBone(FbxNode* pNode) {
        if (pNode->GetNodeAttribute() && pNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton) {
            return pNode;
        }
        for (int i = 0; i < pNode->GetChildCount(); ++i) {
            FbxNode* found = FindRootBone(pNode->GetChild(i));
            if (found) {
                return found;
            }
        }
        return nullptr;
    }

    void CreateTargetSkeleton(FbxNode* pSourceNode, FbxScene* pTargetScene, FbxNode* pTargetParent) {
        FbxNode* targetNode = FbxNode::Create(pTargetScene, pSourceNode->GetName());
        FbxSkeleton* skeletonAttribute = FbxSkeleton::Create(pTargetScene, pSourceNode->GetName());
        targetNode->SetNodeAttribute(skeletonAttribute);

        if (pTargetParent) {
            pTargetParent->AddChild(targetNode);
        } else {
            pTargetScene->GetRootNode()->AddChild(targetNode);
        }

        targetNode->LclTranslation.Set(pSourceNode->LclTranslation.Get());
        targetNode->LclRotation.Set(pSourceNode->LclRotation.Get());
        targetNode->LclScaling.Set(pSourceNode->LclScaling.Get());

        FbxVector4 currentRot = targetNode->LclRotation.Get();
        currentRot[0] -= 90.0;
        targetNode->LclRotation.Set(currentRot);

        for (int i = 0; i < pSourceNode->GetChildCount(); ++i) {
            CreateTargetSkeleton(pSourceNode->GetChild(i), pTargetScene, targetNode);
        }
    }

    void SetKeyframe(FbxAnimLayer* pAnimLayer, FbxNode* pNode, FbxProperty pProperty, FbxTime pTime, const FbxVector4& pValue) {
        FbxAnimCurve* curve;
        int keyIndex;

        curve = pProperty.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
        if (curve) {
            keyIndex = curve->KeyAdd(pTime);
            curve->KeySetValue(keyIndex, static_cast<float>(pValue[0]));
            curve->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationLinear);
        }

        curve = pProperty.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
        if (curve) {
            keyIndex = curve->KeyAdd(pTime);
            curve->KeySetValue(keyIndex, static_cast<float>(pValue[1]));
            curve->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationLinear);
        }

        curve = pProperty.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
        if (curve) {
            keyIndex = curve->KeyAdd(pTime);
            curve->KeySetValue(keyIndex, static_cast<float>(pValue[2]));
            curve->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationLinear);
        }
    }

public:
    const char* GetName() const override {
        return "axis-mender";
    }

    const char* GetDescription() const override {
        return "Retarget FBX animations with coordinate system adjustments";
    }

    const char* GetUsage() const override {
        return "axis-mender <input_fbx_file>";
    }

    int Execute(const std::vector<std::string>& args) override {
        if (args.empty()) {
            std::cerr << "Usage: " << GetUsage() << std::endl;
            return 1;
        }

        const char* inputFilePath = args[0].c_str();
        std::string outputFilePath = inputFilePath;
        size_t pos = outputFilePath.find_last_of(".");
        if (pos != std::string::npos) {
            outputFilePath = outputFilePath.substr(0, pos) + "_retargeted.fbx";
        } else {
            outputFilePath += "_retargeted.fbx";
        }

        FbxManager* sdkManager = FbxManager::Create();
        if (!sdkManager) {
            std::cerr << "Error: Unable to create FBX Manager." << std::endl;
            return 1;
        }
        FbxIOSettings* ioSettings = FbxIOSettings::Create(sdkManager, IOSROOT);
        sdkManager->SetIOSettings(ioSettings);

        FbxImporter* importer = FbxImporter::Create(sdkManager, "");
        if (!importer->Initialize(inputFilePath, -1, sdkManager->GetIOSettings())) {
            std::cerr << "Error loading file: " << inputFilePath << " - " << importer->GetStatus().GetErrorString() << std::endl;
            sdkManager->Destroy();
            return 1;
        }

        FbxScene* sourceScene = FbxScene::Create(sdkManager, "sourceScene");
        importer->Import(sourceScene);
        importer->Destroy();

        FbxScene* targetScene = FbxScene::Create(sdkManager, "targetScene");
        targetScene->GetGlobalSettings().SetAxisSystem(sourceScene->GetGlobalSettings().GetAxisSystem());

        FbxNode* sourceRootBone = FindRootBone(sourceScene->GetRootNode());
        if (!sourceRootBone) {
            std::cerr << "Error: No skeleton found in the source file." << std::endl;
            sdkManager->Destroy();
            return 1;
        }

        CreateTargetSkeleton(sourceRootBone, targetScene, nullptr);
        FbxNode* targetRootBone = targetScene->GetNode(0);

        std::unordered_map<std::string, FbxNode*> targetBoneMap;
        std::vector<FbxNode*> nodeStack;
        nodeStack.push_back(targetRootBone);
        while (!nodeStack.empty()) {
            FbxNode* node = nodeStack.back();
            nodeStack.pop_back();
            targetBoneMap[node->GetName()] = node;
            for (int i = 0; i < node->GetChildCount(); ++i) {
                nodeStack.push_back(node->GetChild(i));
            }
        }

        FbxAnimStack* sourceAnimStack = sourceScene->GetSrcObject<FbxAnimStack>(0);
        if (!sourceAnimStack) {
            std::cout << "Warning: No animation found in the source file. Saving skeleton only." << std::endl;
        } else {
            std::cout << "Animation stack found. Retargeting..." << std::endl;

            FbxAnimStack* targetAnimStack = FbxAnimStack::Create(targetScene, sourceAnimStack->GetName());
            FbxAnimLayer* targetAnimLayer = FbxAnimLayer::Create(targetScene, "Base Layer");
            targetAnimStack->AddMember(targetAnimLayer);

            FbxTimeSpan localTimeSpan = sourceAnimStack->GetLocalTimeSpan();
            FbxTime startTime = localTimeSpan.GetStart();
            FbxTime endTime = localTimeSpan.GetStop();
            FbxTime frameDuration = FbxTime::GetOneFrameValue(sourceScene->GetGlobalSettings().GetTimeMode());

            for (FbxTime currentTime = startTime; currentTime <= endTime; currentTime += frameDuration) {
                nodeStack.clear();
                nodeStack.push_back(sourceRootBone);
                while(!nodeStack.empty()){
                    FbxNode* sourceNode = nodeStack.back();
                    nodeStack.pop_back();

                    for(int i = 0; i < sourceNode->GetChildCount(); ++i){
                        nodeStack.push_back(sourceNode->GetChild(i));
                    }

                    auto it = targetBoneMap.find(sourceNode->GetName());
                    if (it == targetBoneMap.end()) continue;
                    FbxNode* targetNode = it->second;

                    FbxAMatrix sourceGlobalTransform = sourceNode->EvaluateGlobalTransform(currentTime);

                    FbxAMatrix targetParentGlobalTransform;
                    if (targetNode->GetParent()) {
                        targetParentGlobalTransform = targetNode->GetParent()->EvaluateGlobalTransform(currentTime);
                    } else {
                        targetParentGlobalTransform.SetIdentity();
                    }

                    FbxAMatrix targetLocalTransform = targetParentGlobalTransform.Inverse() * sourceGlobalTransform;

                    FbxVector4 localT = targetLocalTransform.GetT();
                    FbxVector4 localR = targetLocalTransform.GetR();
                    FbxVector4 localS = targetLocalTransform.GetS();

                    SetKeyframe(targetAnimLayer, targetNode, targetNode->LclTranslation, currentTime, localT);
                    SetKeyframe(targetAnimLayer, targetNode, targetNode->LclRotation, currentTime, localR);
                    SetKeyframe(targetAnimLayer, targetNode, targetNode->LclScaling, currentTime, localS);
                }
            }
        }

        FbxExporter* exporter = FbxExporter::Create(sdkManager, "");
        if (!exporter->Initialize(outputFilePath.c_str(), -1, sdkManager->GetIOSettings())) {
            std::cerr << "Error saving file: " << outputFilePath << " - " << exporter->GetStatus().GetErrorString() << std::endl;
            sdkManager->Destroy();
            return 1;
        }
        exporter->Export(targetScene);
        exporter->Destroy();

        sdkManager->Destroy();
        std::cout << "Retargeting complete. Output saved to: " << outputFilePath << std::endl;
        return 0;
    }
};

