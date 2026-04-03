#include "resource_handler.h"
#include "resource_registry.h"
#include <sstream>

class AnimationsHandler : public ResourceHandler {
public:
    const char* GetResourceName() const override {
        return "animations";
    }

    LoadStrategy GetLoadStrategy(const std::vector<std::string>& pathSegments) const override {
        // If querying a specific animation with full details, need to import
        // Path: animations/AnimName/** (pathSegments[0] = "animations", [1] = name, [2] = "**")
        if (pathSegments.size() >= 3 && pathSegments[2] == "**") {
            return LoadStrategy::SELECTIVE;
        }
        // Use header-only mode to read take info without loading curves!
        return LoadStrategy::HEADER_ONLY;
    }

    void ConfigureImport(FbxIOSettings* ios, const std::vector<std::string>& pathSegments) const override {
        // Load only animation data
        ios->SetBoolProp(IMP_FBX_TEXTURE, false);
        ios->SetBoolProp(IMP_FBX_MATERIAL, false);
        ios->SetBoolProp(IMP_FBX_MODEL, false);
        ios->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, false);
        // Keep animations enabled (default)
    }

    std::string HandleHeaderOnly(FbxImporter* importer,
                                const std::vector<std::string>& pathSegments) const override {
        int animStackCount = importer->GetAnimStackCount();

        // Check if querying specific animation by name
        if (pathSegments.size() >= 2) {
            std::string animName = pathSegments[1];

            // Find the animation
            for (int i = 0; i < animStackCount; i++) {
                FbxTakeInfo* takeInfo = importer->GetTakeInfo(i);
                if (std::string(takeInfo->mName.Buffer()) == animName) {
                    // Return single animation brief
                    std::ostringstream json;
                    json << "{\n";
                    json << "  \"resource_type\": \"animation\",\n";
                    json << "  \"format_version\": \"1.0\",\n";
                    json << "  \"name\": \"" << takeInfo->mName.Buffer() << "\",\n";
                    json << "  \"start_time\": " << takeInfo->mLocalTimeSpan.GetStart().GetSecondDouble() << ",\n";
                    json << "  \"stop_time\": " << takeInfo->mLocalTimeSpan.GetStop().GetSecondDouble() << ",\n";
                    json << "  \"duration\": " << takeInfo->mLocalTimeSpan.GetDuration().GetSecondDouble() << "\n";
                    json << "}";
                    return json.str();
                }
            }

            return "{\"error\": \"Animation not found\"}";
        }

        // Return all animations (brief list)
        std::ostringstream json;
        json << "{\n";
        json << "  \"resource_type\": \"animations\",\n";
        json << "  \"format_version\": \"1.0\",\n";
        json << "  \"animations\": [\n";

        for (int i = 0; i < animStackCount; i++) {
            FbxTakeInfo* takeInfo = importer->GetTakeInfo(i);

            if (i > 0) json << ",\n";
            json << "    {\n";
            json << "      \"name\": \"" << takeInfo->mName.Buffer() << "\",\n";
            json << "      \"start_time\": " << takeInfo->mLocalTimeSpan.GetStart().GetSecondDouble() << ",\n";
            json << "      \"stop_time\": " << takeInfo->mLocalTimeSpan.GetStop().GetSecondDouble() << ",\n";
            json << "      \"duration\": " << takeInfo->mLocalTimeSpan.GetDuration().GetSecondDouble() << "\n";
            json << "    }";
        }

        json << "\n  ]\n";
        json << "}";

        return json.str();
    }

    std::string HandleScene(FbxScene* scene,
                           const std::vector<std::string>& pathSegments) const override {
        // Full animation data with curves
        if (pathSegments.size() < 2) {
            return "{\"error\": \"Animation name required for full data\"}";
        }

        std::string animName = pathSegments[1];

        // Find and activate the animation stack
        int stackCount = scene->GetSrcObjectCount<FbxAnimStack>();
        FbxAnimStack* animStack = nullptr;

        for (int i = 0; i < stackCount; i++) {
            FbxAnimStack* stack = scene->GetSrcObject<FbxAnimStack>(i);
            if (std::string(stack->GetName()) == animName) {
                animStack = stack;
                break;
            }
        }

        if (!animStack) {
            return "{\"error\": \"Animation not found\"}";
        }

        scene->SetCurrentAnimationStack(animStack);

        std::ostringstream json;
        json << "{\n";
        json << "  \"resource_type\": \"animation_full\",\n";
        json << "  \"format_version\": \"1.0\",\n";
        json << "  \"name\": \"" << animStack->GetName() << "\",\n";

        // Get time span
        FbxTimeSpan timeSpan = animStack->GetLocalTimeSpan();
        json << "  \"start_time\": " << timeSpan.GetStart().GetSecondDouble() << ",\n";
        json << "  \"stop_time\": " << timeSpan.GetStop().GetSecondDouble() << ",\n";
        json << "  \"duration\": " << timeSpan.GetDuration().GetSecondDouble() << ",\n";

        // Get animation layers
        int layerCount = animStack->GetMemberCount<FbxAnimLayer>();
        json << "  \"layers\": " << layerCount << ",\n";
        json << "  \"animated_nodes\": [\n";

        // Collect all animated nodes
        bool firstNode = true;
        std::function<void(FbxNode*)> processNode = [&](FbxNode* node) {
            if (!node) return;

            // Check if this node has animation curves
            bool hasAnimation = false;
            for (int l = 0; l < layerCount; l++) {
                FbxAnimLayer* layer = animStack->GetMember<FbxAnimLayer>(l);

                FbxAnimCurve* curveX = node->LclTranslation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_X);
                FbxAnimCurve* curveY = node->LclTranslation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Y);
                FbxAnimCurve* curveZ = node->LclTranslation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Z);

                FbxAnimCurve* rotX = node->LclRotation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_X);
                FbxAnimCurve* rotY = node->LclRotation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Y);
                FbxAnimCurve* rotZ = node->LclRotation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Z);

                FbxAnimCurve* scaleX = node->LclScaling.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_X);
                FbxAnimCurve* scaleY = node->LclScaling.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Y);
                FbxAnimCurve* scaleZ = node->LclScaling.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Z);

                if (curveX || curveY || curveZ || rotX || rotY || rotZ || scaleX || scaleY || scaleZ) {
                    hasAnimation = true;
                    break;
                }
            }

            if (hasAnimation) {
                if (!firstNode) json << ",\n";
                firstNode = false;

                json << "    {\n";
                json << "      \"name\": \"" << node->GetName() << "\",\n";
                json << "      \"properties\": [";

                bool firstProp = true;
                for (int l = 0; l < layerCount; l++) {
                    FbxAnimLayer* layer = animStack->GetMember<FbxAnimLayer>(l);

                    auto addCurveInfo = [&](const char* propName, FbxAnimCurve* curve) {
                        if (curve && curve->KeyGetCount() > 0) {
                            if (!firstProp) json << ", ";
                            firstProp = false;
                            json << "\"" << propName << "(" << curve->KeyGetCount() << " keys)\"";
                        }
                    };

                    addCurveInfo("T.X", node->LclTranslation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_X));
                    addCurveInfo("T.Y", node->LclTranslation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Y));
                    addCurveInfo("T.Z", node->LclTranslation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Z));
                    addCurveInfo("R.X", node->LclRotation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_X));
                    addCurveInfo("R.Y", node->LclRotation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Y));
                    addCurveInfo("R.Z", node->LclRotation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Z));
                    addCurveInfo("S.X", node->LclScaling.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_X));
                    addCurveInfo("S.Y", node->LclScaling.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Y));
                    addCurveInfo("S.Z", node->LclScaling.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Z));
                }

                json << "]\n";
                json << "    }";
            }

            for (int i = 0; i < node->GetChildCount(); i++) {
                processNode(node->GetChild(i));
            }
        };

        processNode(scene->GetRootNode());

        json << "\n  ]\n";
        json << "}";

        return json.str();
    }
};

REGISTER_RESOURCE_HANDLER(AnimationsHandler)
