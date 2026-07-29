#include "command.h"
#include <fbxsdk.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

class ResampleCommand : public Command {
private:
    struct CurveRef {
        FbxAnimCurve* curve;
    };

    static void CollectPropertyCurves(FbxPropertyT<FbxDouble3>& property,
                                      FbxAnimLayer* layer,
                                      std::vector<CurveRef>& curves) {
        const char* components[] = {
            FBXSDK_CURVENODE_COMPONENT_X,
            FBXSDK_CURVENODE_COMPONENT_Y,
            FBXSDK_CURVENODE_COMPONENT_Z
        };
        for (const char* component : components) {
            if (FbxAnimCurve* curve = property.GetCurve(layer, component, false)) {
                curves.push_back({curve});
            }
        }
    }

    static void CollectNodeCurves(FbxNode* node,
                                  FbxAnimLayer* layer,
                                  std::vector<CurveRef>& curves) {
        if (!node) return;
        CollectPropertyCurves(node->LclTranslation, layer, curves);
        CollectPropertyCurves(node->LclRotation, layer, curves);
        CollectPropertyCurves(node->LclScaling, layer, curves);
        for (int i = 0; i < node->GetChildCount(); ++i) {
            CollectNodeCurves(node->GetChild(i), layer, curves);
        }
    }

    static FbxTime::EMode TimeModeForFps(double fps) {
        struct StandardRate { double fps; FbxTime::EMode mode; };
        const StandardRate rates[] = {
            {120.0, FbxTime::eFrames120}, {100.0, FbxTime::eFrames100},
            {60.0, FbxTime::eFrames60},   {50.0, FbxTime::eFrames50},
            {48.0, FbxTime::eFrames48},   {30.0, FbxTime::eFrames30},
            {25.0, FbxTime::ePAL},        {24.0, FbxTime::eFrames24},
            {96.0, FbxTime::eFrames96},   {72.0, FbxTime::eFrames72},
            {59.94, FbxTime::eFrames59dot94},
            {119.88, FbxTime::eFrames119dot88}
        };
        for (const auto& rate : rates) {
            if (std::abs(fps - rate.fps) < 0.0001) return rate.mode;
        }
        return FbxTime::eCustom;
    }

    static FbxTime Seconds(double seconds) {
        FbxTime time;
        time.SetSecondDouble(seconds);
        return time;
    }

public:
    const char* GetName() const override { return "resample"; }
    const char* GetDescription() const override {
        return "Evaluate and bake transform curves at a new FPS; preserves duration and playback speed but changes key count";
    }
    const char* GetUsage() const override {
        return "resample <input.fbx> <output.fbx> <fps> [--zero-start]";
    }

    int Execute(const std::vector<std::string>& args) override {
        if (args.size() < 3 || args.size() > 4) {
            std::cerr << "Usage: tools " << GetUsage() << "\n";
            std::cerr << "Properties:\n"
                      << "  - Animation duration and playback speed are preserved.\n"
                      << "  - Transform curves are evaluated and baked on the target FPS grid.\n"
                      << "  - Key count and file size can increase or decrease.\n"
                      << "  - --zero-start moves the first animation time to 00:00:00.\n";
            return 1;
        }

        const std::string inputPath = args[0];
        const std::string outputPath = args[1];
        char* parseEnd = nullptr;
        const double targetFps = std::strtod(args[2].c_str(), &parseEnd);
        if (!parseEnd || *parseEnd != '\0' || !std::isfinite(targetFps) || targetFps <= 0.0 || targetFps > 1000.0) {
            std::cerr << "Error: fps must be a number greater than 0 and no greater than 1000.\n";
            return 1;
        }
        if (TimeModeForFps(targetFps) == FbxTime::eCustom && !FbxIsValidCustomFrameRate(targetFps)) {
            double nearestFps = 0.0;
            FbxGetNearestCustomFrameRate(targetFps, nearestFps);
            std::cerr << "Error: " << targetFps << " FPS cannot be represented exactly by the FBX time base. "
                      << "Nearest supported custom rate: " << nearestFps << " FPS.\n";
            return 1;
        }
        const bool zeroStart = args.size() == 4 && args[3] == "--zero-start";
        if (args.size() == 4 && !zeroStart) {
            std::cerr << "Error: unknown option: " << args[3] << "\n";
            return 1;
        }
        if (inputPath == outputPath) {
            std::cerr << "Error: input and output paths must differ.\n";
            return 1;
        }

        FbxManager* manager = FbxManager::Create();
        if (!manager) {
            std::cerr << "Error: unable to create FBX manager.\n";
            return 1;
        }
        FbxIOSettings* io = FbxIOSettings::Create(manager, IOSROOT);
        manager->SetIOSettings(io);

        FbxImporter* importer = FbxImporter::Create(manager, "");
        if (!importer->Initialize(inputPath.c_str(), -1, io)) {
            std::cerr << "Error loading file: " << importer->GetStatus().GetErrorString() << "\n";
            importer->Destroy();
            manager->Destroy();
            return 1;
        }
        FbxScene* scene = FbxScene::Create(manager, "scene");
        if (!importer->Import(scene)) {
            std::cerr << "Error importing file: " << importer->GetStatus().GetErrorString() << "\n";
            importer->Destroy();
            manager->Destroy();
            return 1;
        }
        importer->Destroy();

        const FbxTime::EMode sourceMode = scene->GetGlobalSettings().GetTimeMode();
        const double sourceFps = FbxTime::GetFrameRate(sourceMode);
        const double sourceDeclaredFps = sourceMode == FbxTime::eCustom
            ? scene->GetGlobalSettings().GetCustomFrameRate()
            : sourceFps;
        const int stackCount = scene->GetSrcObjectCount<FbxAnimStack>();
        if (stackCount == 0) {
            std::cerr << "Error: no animation stacks found.\n";
            manager->Destroy();
            return 1;
        }

        long long totalOldKeys = 0;
        long long totalNewKeys = 0;
        int totalCurves = 0;
        double firstSourceStart = 0.0;
        double firstSourceStop = 0.0;
        double firstOutputStart = 0.0;
        double firstOutputStop = 0.0;

        for (int stackIndex = 0; stackIndex < stackCount; ++stackIndex) {
            FbxAnimStack* stack = scene->GetSrcObject<FbxAnimStack>(stackIndex);
            scene->SetCurrentAnimationStack(stack);
            const FbxTimeSpan sourceSpan = stack->GetLocalTimeSpan();
            const FbxTime sourceStart = sourceSpan.GetStart();
            const FbxTime sourceStop = sourceSpan.GetStop();
            const double duration = sourceStop.GetSecondDouble() - sourceStart.GetSecondDouble();
            if (duration < 0.0) {
                std::cerr << "Error: animation stack has a negative duration: " << stack->GetName() << "\n";
                manager->Destroy();
                return 1;
            }

            const long long intervals = std::max(0LL, static_cast<long long>(std::llround(duration * targetFps)));
            const FbxTime outputStart = zeroStart ? Seconds(0.0) : sourceStart;
            const FbxTime outputStop = outputStart + Seconds(static_cast<double>(intervals) / targetFps);

            std::vector<FbxTime> sourceTimes;
            std::vector<FbxTime> outputTimes;
            sourceTimes.reserve(static_cast<size_t>(intervals + 1));
            outputTimes.reserve(static_cast<size_t>(intervals + 1));
            for (long long frame = 0; frame <= intervals; ++frame) {
                FbxTime sourceTime = sourceStart + Seconds(static_cast<double>(frame) / targetFps);
                if (sourceTime > sourceStop) sourceTime = sourceStop;
                sourceTimes.push_back(sourceTime);
                outputTimes.push_back(outputStart + Seconds(static_cast<double>(frame) / targetFps));
            }

            const int layerCount = stack->GetMemberCount<FbxAnimLayer>();
            for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex) {
                FbxAnimLayer* layer = stack->GetMember<FbxAnimLayer>(layerIndex);
                std::vector<CurveRef> curves;
                CollectNodeCurves(scene->GetRootNode(), layer, curves);
                totalCurves += static_cast<int>(curves.size());

                for (const CurveRef& ref : curves) {
                    FbxAnimCurve* curve = ref.curve;
                    totalOldKeys += curve->KeyGetCount();
                    std::vector<float> values;
                    values.reserve(sourceTimes.size());
                    for (const FbxTime& time : sourceTimes) {
                        values.push_back(curve->Evaluate(time));
                    }

                    curve->KeyModifyBegin();
                    curve->KeyClear();
                    for (size_t i = 0; i < outputTimes.size(); ++i) {
                        const int keyIndex = curve->KeyAdd(outputTimes[i]);
                        curve->KeySetValue(keyIndex, values[i]);
                        curve->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationLinear);
                    }
                    curve->KeyModifyEnd();
                    totalNewKeys += static_cast<long long>(outputTimes.size());
                }
            }

            FbxTimeSpan outputSpan(outputStart, outputStop);
            stack->SetLocalTimeSpan(outputSpan);
            if (stackIndex == 0) {
                firstSourceStart = sourceStart.GetSecondDouble();
                firstSourceStop = sourceStop.GetSecondDouble();
                firstOutputStart = outputStart.GetSecondDouble();
                firstOutputStop = outputStop.GetSecondDouble();
            }
        }

        const FbxTime::EMode targetMode = TimeModeForFps(targetFps);
        FbxTime::SetGlobalTimeMode(targetMode, targetMode == FbxTime::eCustom ? targetFps : 0.0);
        scene->GetGlobalSettings().SetTimeMode(targetMode);
        if (targetMode == FbxTime::eCustom) {
            scene->GetGlobalSettings().SetCustomFrameRate(targetFps);
            FbxProperty customRate = scene->GetGlobalSettings().FindProperty("CustomFrameRate");
            if (customRate.IsValid()) customRate.Set<FbxDouble>(targetFps);
        }
        scene->GetGlobalSettings().SetTimelineDefaultTimeSpan(
            FbxTimeSpan(Seconds(firstOutputStart), Seconds(firstOutputStop)));

        FbxExporter* exporter = FbxExporter::Create(manager, "");
        if (!exporter->Initialize(outputPath.c_str(), -1, io)) {
            std::cerr << "Error creating output: " << exporter->GetStatus().GetErrorString() << "\n";
            exporter->Destroy();
            manager->Destroy();
            return 1;
        }
        if (!exporter->Export(scene)) {
            std::cerr << "Error exporting output: " << exporter->GetStatus().GetErrorString() << "\n";
            exporter->Destroy();
            manager->Destroy();
            return 1;
        }
        exporter->Destroy();
        manager->Destroy();

        std::cout << std::fixed << std::setprecision(6)
                  << "{\n"
                  << "  \"tool\": \"resample\",\n"
                  << "  \"input\": \"" << inputPath << "\",\n"
                  << "  \"output\": \"" << outputPath << "\",\n"
                  << "  \"source_timeline_fps\": " << sourceFps << ",\n"
                  << "  \"source_declared_fps\": " << sourceDeclaredFps << ",\n"
                  << "  \"target_fps\": " << targetFps << ",\n"
                  << "  \"source_start_seconds\": " << firstSourceStart << ",\n"
                  << "  \"source_stop_seconds\": " << firstSourceStop << ",\n"
                  << "  \"output_start_seconds\": " << firstOutputStart << ",\n"
                  << "  \"output_stop_seconds\": " << firstOutputStop << ",\n"
                  << "  \"animation_stacks\": " << stackCount << ",\n"
                  << "  \"transform_curves\": " << totalCurves << ",\n"
                  << "  \"old_keys\": " << totalOldKeys << ",\n"
                  << "  \"new_keys\": " << totalNewKeys << "\n"
                  << "}\n";
        return 0;
    }
};
