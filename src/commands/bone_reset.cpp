#include "command.h"
#include <fbxsdk.h>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace fs = std::filesystem;

class BoneResetCommand : public Command {
private:
    bool fixedRightHandThumb1 = false;
    bool fixedLeftHandThumb1 = false;
    bool fixedHips = false;

    std::string GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        struct tm timeInfo;
#ifdef _WIN32
        localtime_s(&timeInfo, &time);
#else
        localtime_r(&time, &timeInfo);
#endif

        std::stringstream ss;
        ss << std::put_time(&timeInfo, "%Y-%m-%d_%H-%M-%S");
        return ss.str();
    }

    void ResetBoneRotations(FbxNode* node, FbxAnimLayer* animLayer, FbxTime& startTime) {
        if (!node) return;

        FbxVector4 resetRotationValue = FbxVector4(0.0, 0.0, 0.0);

        FbxAnimEvaluator* evaluator = node->GetScene()->GetAnimationEvaluator();
        FbxAMatrix localTransform = evaluator->GetNodeLocalTransform(node, startTime);
        localTransform.SetR(resetRotationValue);

        if(strcmp(node->GetName(),"RightHandThumb1") == 0) {
            fixedRightHandThumb1 = true;
            resetRotationValue = FbxVector4(45.0, 45.0, 0.0);
        } else if (strcmp(node->GetName(),"LeftHandThumb1") == 0) {
            fixedLeftHandThumb1 = true;
            resetRotationValue = FbxVector4(45.0, -45.0, 0.0);
        }
        node->LclRotation = resetRotationValue;

        FbxAnimCurve* curve = node->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
        if (curve) {
            curve->KeyModifyBegin();
            FbxAnimCurveKey key;
            key.Set(startTime, (float)node->LclRotation.Get()[0]);
            curve->KeyAdd(startTime, key);
            curve->KeyModifyEnd();
        }

        curve = node->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
        if (curve) {
            curve->KeyModifyBegin();
            FbxAnimCurveKey key;
            key.Set(startTime, (float)node->LclRotation.Get()[1]);
            curve->KeyAdd(startTime, key);
            curve->KeyModifyEnd();
        }

        curve = node->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
        if (curve) {
            curve->KeyModifyBegin();
            FbxAnimCurveKey key;
            key.Set(startTime, (float)node->LclRotation.Get()[2]);
            curve->KeyAdd(startTime, key);
            curve->KeyModifyEnd();
        }

        if(strcmp(node->GetName(), "Hips") == 0) {
            FbxVector4 localT = localTransform.GetT();
            localT[0] = 0.0;
            localT[2] = 0.0;
            localTransform.SetT(localT);
            node->LclTranslation = localT;

            curve = node->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
            if (curve) {
                curve->KeyModifyBegin();
                FbxAnimCurveKey key;
                key.Set(startTime, 0.0f);
                curve->KeyAdd(startTime, key);
                curve->KeyModifyEnd();
            }

            curve = node->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
            if (curve) {
                curve->KeyModifyBegin();
                FbxAnimCurveKey key;
                key.Set(startTime, 0.0f);
                curve->KeyAdd(startTime, key);
                curve->KeyModifyEnd();
            }
            fixedHips = true;
        }

        for (int i = 0; i < node->GetChildCount(); ++i) {
            ResetBoneRotations(node->GetChild(i), animLayer, startTime);
        }
    }

    bool ProcessFBX(const std::string& filePath) {
        fixedRightHandThumb1 = false;
        fixedLeftHandThumb1 = false;
        fixedHips = false;

        FbxManager* sdkManager = FbxManager::Create();
        FbxIOSettings* ioSettings = FbxIOSettings::Create(sdkManager, IOSROOT);
        sdkManager->SetIOSettings(ioSettings);

        FbxImporter* importer = FbxImporter::Create(sdkManager, "");
        if (!importer->Initialize(filePath.c_str(), -1, sdkManager->GetIOSettings())) {
            printf("Failed to initialize importer for file: %s\n", filePath.c_str());
            sdkManager->Destroy();
            return false;
        }

        FbxScene* scene = FbxScene::Create(sdkManager, "Scene");
        importer->Import(scene);
        importer->Destroy();

        FbxTime startTime;
        FbxAnimStack* animStack = nullptr;
        FbxAnimLayer* animLayer = nullptr;

        int animStackCount = scene->GetSrcObjectCount<FbxAnimStack>();
        for (int i = 0; i < animStackCount; ++i) {
            animStack = FbxCast<FbxAnimStack>(scene->GetSrcObject<FbxAnimStack>(i));
            scene->SetCurrentAnimationStack(animStack);

            animLayer = animStack->GetMember<FbxAnimLayer>();
            if (!animLayer) continue;

            startTime = animStack->GetLocalTimeSpan().GetStart();
            ResetBoneRotations(scene->GetRootNode(), animLayer, startTime);
        }

        FbxExporter* exporter = FbxExporter::Create(sdkManager, "");
        if (!exporter->Initialize(filePath.c_str(), -1, sdkManager->GetIOSettings())) {
            printf("Failed to initialize exporter for file:  %s\n", filePath.c_str());
            sdkManager->Destroy();
            return false;
        }

        if (!fixedHips || !fixedLeftHandThumb1 || !fixedRightHandThumb1 || !exporter->Export(scene)) {
            printf("Failed to fix file:  %s\n", filePath.c_str());
        } else {
            printf("Finished to fix file:  %s\n", filePath.c_str());
        }

        exporter->Destroy();
        sdkManager->Destroy();
        return true;
    }

    void ProcessDirectory(const std::string& dirPath) {
        for (const auto& entry : fs::directory_iterator(dirPath)) {
            if (entry.path().extension() == ".fbx") {
                std::cout << "Processing: " << entry.path().string() << std::endl;
                if (!ProcessFBX(entry.path().string())) {
                    std::cerr << "Failed to process: " << entry.path().string() << std::endl;
                }
            }
        }
    }

public:
    const char* GetName() const override {
        return "bone-reset";
    }

    const char* GetDescription() const override {
        return "Reset bone rotations and fix hand/hip positions in FBX files";
    }

    const char* GetUsage() const override {
        return "bone-reset <file_or_directory>";
    }

    int Execute(const std::vector<std::string>& args) override {
        if (args.empty()) {
            std::cerr << "Usage: " << GetUsage() << std::endl;
            return -1;
        }

        std::string targetPath = args[0];
        std::string timestamp = GetTimestamp();

        fs::path reportPath;
        if (fs::is_directory(targetPath)) {
            reportPath = fs::path(targetPath) / ("report_" + timestamp + ".txt");
        } else if (fs::is_regular_file(targetPath)) {
            reportPath = fs::path(targetPath).parent_path() / ("report_" + timestamp + ".txt");
        } else {
            std::cerr << "Invalid input. Provide either a directory containing FBX files or a single FBX file." << std::endl;
            return -1;
        }

        FILE* reportFile = nullptr;
#ifdef _WIN32
        errno_t err = freopen_s(&reportFile, reportPath.string().c_str(), "w", stdout);
        if (err != 0) {
            printf("Failed to open report file at %s.\n", reportPath.string().c_str());
            return -1;
        }
#else
        reportFile = freopen(reportPath.string().c_str(), "w", stdout);
        if (!reportFile) {
            printf("Failed to open report file at %s.\n", reportPath.string().c_str());
            return -1;
        }
#endif

        if (fs::is_directory(targetPath)) {
            ProcessDirectory(targetPath);
        } else if (fs::is_regular_file(targetPath) && targetPath.substr(targetPath.find_last_of(".") + 1) == "fbx") {
            std::cout << "Processing single file: " << targetPath << std::endl;
            if (!ProcessFBX(targetPath)) {
                std::cerr << "Failed to process file: " << targetPath << std::endl;
            }
        }

        fclose(reportFile);
#ifdef _WIN32
        freopen_s(&reportFile, "CON", "w", stdout);
#else
        freopen("/dev/tty", "w", stdout);
#endif
        return 0;
    }
};

